#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "AggregatedStatsCollection.h"
#include "arcdps_mock/CombatMock.h"
#include "Exports.h"
#include "Log.h"
#include "Options.h"
#include "../networking/Client.h"
#include "../networking/Server.h"

#include <utility>

bool operator==(const cbtevent& pLeft, const cbtevent& pRight)
{
	return memcmp(&pLeft, &pRight, sizeof(pLeft)) == 0;
}

void FillRandomData(void* pBuffer, size_t pBufferSize)
{
	for (size_t i = 0; i < pBufferSize; i++)
	{
		static_cast<byte*>(pBuffer)[i] = rand() % 256;
	}
}

// parameters are <max parallel callbacks, max fuzz width>
class SimpleNetworkTestFixture : public ::testing::Test
{
protected:
	void SetUp() override
	{
		uint64_t seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(static_cast<uint32_t>(seed));

		mServerThread = std::make_unique<std::thread>(evtc_rpc_server::ThreadStartServe, &Server);
	}

	void TearDown() override
	{
		for (auto& client : mClients)
		{
			client->Client->Shutdown();
		}
		for (auto& thread : mClientThreads)
		{
			thread.join();
		}
		Server.Shutdown();
		mServerThread->join();
	}

	struct ClientInstance
	{
		std::unique_ptr<evtc_rpc_client> Client;
		std::vector<cbtevent> ReceivedEvents; 

		evtc_rpc_client* operator->()
		{
			return Client.get();
		}
	};

	void FlushEvents()
	{
		for (auto& client : mClients)
		{
			client->Client->FlushEvents();
		}
	}

	ClientInstance& NewClient()
	{
		std::unique_ptr<ClientInstance>& newClient = mClients.emplace_back(std::make_unique<ClientInstance>());
		auto eventhandler = [client = newClient.get()](cbtevent* pEvent, uint16_t /*pInstanceId*/)
			{
				client->ReceivedEvents.push_back(*pEvent);
			};
		auto getEndpoint = []() -> std::string
			{
				return std::string{"localhost:50051"};
			};
		newClient->Client = std::make_unique<evtc_rpc_client>(std::move(getEndpoint), std::move(eventhandler));

		mClientThreads.emplace_back(evtc_rpc_client::ThreadStartServe, mClients.back()->Client.get());
		return *newClient.get();
	}

// A bug in grpc memory leak detection causes Server shutdown to detect leaks if there are non-destroyed clients
// active. Therefore we have to make sure that the clients are shutdown first (which leads to the strange
// private-public-private order)
private:
	std::vector<std::unique_ptr<ClientInstance>> mClients;

public:
	evtc_rpc_server Server{"localhost:50051"};

private:
	std::unique_ptr<std::thread> mServerThread;
	std::vector<std::thread> mClientThreads;
};

namespace
{
static EventProcessor* ACTIVE_PROCESSOR = nullptr;
static evtc_rpc_client* ACTIVE_CLIENT = nullptr;
uintptr_t network_test_mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	ACTIVE_PROCESSOR->AreaCombat(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	ACTIVE_CLIENT->ProcessAreaEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t network_test_mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	ACTIVE_PROCESSOR->LocalCombat(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	ACTIVE_CLIENT->ProcessLocalEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	return 0;
}
}; // Anonymous namespace

// parameters are <max parallel callbacks, max fuzz width>
class NetworkXevtcTestFixture : public ::testing::TestWithParam<uint32_t>
{
protected:
	void SetUp() override
	{
		uint64_t seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(static_cast<uint32_t>(seed));

		Server = std::make_unique<evtc_rpc_server>("localhost:50051");

		auto eventhandler = [this](cbtevent* /*pEvent*/, uint16_t /*pInstanceId*/)
		{
			// Do nothing
		};
		auto getEndpoint = []() -> std::string
		{
			return std::string{"localhost:50051"};
		};
		Client = std::make_unique<evtc_rpc_client>(std::function{getEndpoint}, std::move(eventhandler));

		auto eventhandler2 = [this](cbtevent* pEvent, uint16_t pInstanceId)
		{
			Processor.PeerCombat(pEvent, pInstanceId);
		};
		PeerClient = std::make_unique<evtc_rpc_client>(std::move(getEndpoint), std::move(eventhandler2));

		ACTIVE_PROCESSOR = &Processor;
		ACTIVE_CLIENT = Client.get();

		mExports.combat = network_test_mod_combat;
		mExports.combat_local = network_test_mod_combat_local;

		mServerThread = std::make_unique<std::thread>(evtc_rpc_server::ThreadStartServe, Server.get());
		mClientThread = std::make_unique<std::thread>(evtc_rpc_client::ThreadStartServe, Client.get());
		mPeerClientThread = std::make_unique<std::thread>(evtc_rpc_client::ThreadStartServe, PeerClient.get());
	}

	void TearDown() override
	{
		Client->Shutdown();
		PeerClient->Shutdown();
		mClientThread->join();
		mPeerClientThread->join();

		Server->Shutdown();
		mServerThread->join();

		ACTIVE_PROCESSOR = nullptr;
		ACTIVE_CLIENT = nullptr;
	}

	CombatMock Mock{&mExports};
	EventProcessor Processor;

	std::unique_ptr<evtc_rpc_client> PeerClient;
	std::unique_ptr<evtc_rpc_client> Client;
	std::unique_ptr<evtc_rpc_server> Server;

private:
	arcdps_exports mExports;

	std::unique_ptr<std::thread> mPeerClientThread;
	std::unique_ptr<std::thread> mClientThread;
	std::unique_ptr<std::thread> mServerThread;
};


TEST_F(SimpleNetworkTestFixture, RegisterSelf)
{
	ClientInstance& client1 = NewClient();

	for (uint16_t instid : std::array<uint16_t, 2>({13, 17}))
	{
		// Send a self agent registration event
		ag ag1{};
		ag ag2{};
		ag1.elite = 0;
		ag1.prof = static_cast<Prof>(1);
		ag2.self = 1;
		ag2.id = instid;
		ag2.name = "testagent.1234";
		client1->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

		FlushEvents();

		// Wait until the server sees the new agent and then verify the state
		auto start = std::chrono::system_clock::now();
		bool completed = false;
		while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(200))
		{
			{
				std::lock_guard lock(Server.mRegisteredAgentsLock);
				if (Server.mRegisteredAgents.size() >= 1)
				{
					auto iter = Server.mRegisteredAgents.find("testagent.1234");
					if (iter != Server.mRegisteredAgents.end() && iter->second->InstanceId == instid)
					{
						completed = true;
						break;
					}
				}
			}

			Sleep(1);
		}
		EXPECT_TRUE(completed);
		{
			std::lock_guard lock(Server.mRegisteredAgentsLock);

			EXPECT_EQ(Server.mRegisteredAgents.size(), 1);
			auto iter = Server.mRegisteredAgents.find("testagent.1234");
			ASSERT_NE(iter, Server.mRegisteredAgents.end());
			EXPECT_EQ(iter->first, "testagent.1234");
			EXPECT_EQ(iter->second->Iterator, iter);
			EXPECT_EQ(iter->second->InstanceId, instid);
			EXPECT_EQ(iter->second->Peers.size(), 0);
		}
	}
}

TEST_F(SimpleNetworkTestFixture, RegisterPeer)
{
	ClientInstance& client1 = NewClient();

	// Send a self agent registration event followed by a non-self agent registration event
	ag ag1{};
	ag ag2{};
	ag1.elite = 0;
	ag1.prof = static_cast<Prof>(1);
	ag2.self = 1;
	ag2.id = 10;
	ag2.name = "testagent.1234";
	client1->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	ag2.self = 0;
	ag2.id = 11;
	ag2.name = "testagent2.1234";
	client1->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	FlushEvents();

	// Wait until the server sees the new agents and then verify the state
	auto start = std::chrono::system_clock::now();
	bool completed = false;
	while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
	{
		{
			std::lock_guard lock(Server.mRegisteredAgentsLock);
			auto iter = Server.mRegisteredAgents.find("testagent.1234");
			if (iter != Server.mRegisteredAgents.end())
			{
				if (iter->second->Peers.size() > 0)
				{
					completed = true;
					break;
				}
			}
		}

		Sleep(1);
	}
	ASSERT_TRUE(completed);
	{
		std::lock_guard lock(Server.mRegisteredAgentsLock);

		EXPECT_EQ(Server.mRegisteredAgents.size(), 1);
		auto iter = Server.mRegisteredAgents.find("testagent.1234");
		ASSERT_NE(iter, Server.mRegisteredAgents.end());
		EXPECT_EQ(iter->first, "testagent.1234");
		EXPECT_EQ(iter->second->Iterator, iter);
		EXPECT_EQ(iter->second->InstanceId, 10);

		auto expected_map = std::map<std::string, uint16_t>({{"testagent2.1234", static_cast<uint16_t>(11)}});
		EXPECT_EQ(iter->second->Peers, expected_map);
	}

	// Deregister the non-self agent
	ag1.prof = static_cast<Prof>(0);
	client1->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	FlushEvents();

	// Wait until the server deregistered the peer and then verify the state
	start = std::chrono::system_clock::now();
	completed = false;
	while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
	{
		{
			std::lock_guard lock(Server.mRegisteredAgentsLock);
			auto iter = Server.mRegisteredAgents.find("testagent.1234");
			if (iter != Server.mRegisteredAgents.end())
			{
				if (iter->second->Peers.size() == 0)
				{
					completed = true;
					break;
				}
			}
		}

		Sleep(1);
	}
	ASSERT_TRUE(completed);
	{
		std::lock_guard lock(Server.mRegisteredAgentsLock);

		EXPECT_EQ(Server.mRegisteredAgents.size(), 1);
		auto iter = Server.mRegisteredAgents.find("testagent.1234");
		ASSERT_NE(iter, Server.mRegisteredAgents.end());
		EXPECT_EQ(iter->first, "testagent.1234");
		EXPECT_EQ(iter->second->Iterator, iter);
		EXPECT_EQ(iter->second->InstanceId, 10);

		auto expected_map = std::map<std::string, uint16_t>({});
		EXPECT_EQ(iter->second->Peers, expected_map);
	}
}

TEST_F(SimpleNetworkTestFixture, CombatEvent)
{
	ClientInstance& client1 = NewClient();

	// Send a self agent registration event followed by a non-self agent registration event followed by a combat event
	// This should not result in any events sent to peer
	ag ag1{};
	ag ag2{};
	ag1.elite = 0;
	ag1.prof = static_cast<Prof>(1);
	ag2.self = 1;
	ag2.id = 10;
	ag2.name = "testagent.1234";
	client1->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	ag2.self = 0;
	ag2.id = 11;
	ag2.name = "testagent2.1234";
	client1->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	cbtevent ev;
	FillRandomData(&ev, sizeof(ev));
	client1->ProcessLocalEvent(&ev, nullptr, nullptr, nullptr, 0, 0);

	FlushEvents();

	auto start = std::chrono::system_clock::now();
	bool completed = false;
	while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
	{
		{
			std::lock_guard lock(Server.mRegisteredAgentsLock);
			auto iter = Server.mRegisteredAgents.find("testagent.1234");
			if (iter != Server.mRegisteredAgents.end())
			{
				if (iter->second->Peers.size() > 0)
				{
					completed = true;
					break;
				}
			}
		}

		Sleep(1);
	}
	ASSERT_TRUE(completed);

	// Start a new client that registers itself and the first client as a peer. Then resend the same event again.
	// It should now be received
	ClientInstance& client2 = NewClient();

	ag2.self = 1;
	ag2.id = 11;
	ag2.name = "testagent2.1234";
	client2->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	ag2.self = 0;
	ag2.id = 10;
	ag2.name = "testagent.1234";
	client2->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	FlushEvents();

	start = std::chrono::system_clock::now();
	completed = false;
	while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
	{
		{
			std::lock_guard lock(Server.mRegisteredAgentsLock);
			auto iter = Server.mRegisteredAgents.find("testagent2.1234");
			if (iter != Server.mRegisteredAgents.end())
			{
				if (iter->second->Peers.size() > 0)
				{
					completed = true;
					break;
				}
			}
		}

		Sleep(1);
	}
	ASSERT_TRUE(completed);

	client1->ProcessLocalEvent(&ev, nullptr, nullptr, nullptr, 0, 0);

	FlushEvents();

	Sleep(100); // TODO: get rid of this ugly sleep

	std::vector<cbtevent> expectedEvents{ev};
	EXPECT_EQ(client2.ReceivedEvents, expectedEvents);
}

TEST_P(NetworkXevtcTestFixture, druid_MO)
{
	uint32_t parallelCallbacks = GetParam();
	LOG("Starting test, parallelCallbacks=%u", parallelCallbacks);

	ag ag1{};
	ag ag2{};
	ag1.elite = 0;
	ag1.prof = static_cast<Prof>(1);
	ag2.self = 1;
	ag2.id = 642;
	ag2.name = ":Spontanefix.2376";
	PeerClient->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	ag2.self = 0;
	ag2.id = 697;
	ag2.name = ":worshipperofnarnia.2689";
	PeerClient->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);

	uint32_t result = Mock.ExecuteFromXevtc("logs\\druid_MO.xevtc", parallelCallbacks, 0);
	ASSERT_EQ(result, 0);

	Client->FlushEvents();
	PeerClient->FlushEvents();
	Sleep(1000); // TODO: Fix ugly sleep :(

	HealTableOptions options;
	Processor.mSelfUniqueId.store(100000); // Fake self having a different id
	Processor.mAgentTable.AddAgent(100000, static_cast<uint16_t>(UINT16_MAX - 1), "Local Zarwae", static_cast<uint16_t>(1), false, true);
	auto [localId, states] = Processor.GetState();

	HealingStats* localState = &states[localId].second;
	HealingStats* peerState = &states[2000].second;

	ASSERT_EQ(states.size(), 2);
	ASSERT_EQ(localState->Events.size(), peerState->Events.size());
	for (size_t i = 0; i < localState->Events.size(); i++)
	{
		if (localState->Events[i] != peerState->Events[i])
		{
			LOG("Event %zu does not match - %llu %llu %llu %u vs %llu %llu %llu %u", i,
				localState->Events[i].Time, localState->Events[i].Size, localState->Events[i].AgentId, localState->Events[i].SkillId,
				peerState->Events[i].Time, peerState->Events[i].Size, peerState->Events[i].AgentId, peerState->Events[i].SkillId);
			GTEST_FAIL();
		}
	}

	for (HealingStats* rawStats : {localState, peerState})
	{
		LOG("Verifying %p (localStats=%p peerStats=%p)", rawStats, localState, peerState);

		// Use the "Combined" window
		AggregatedStats stats{std::move(*rawStats), options.Windows[9], false};

		EXPECT_FLOAT_EQ(std::floor(stats.GetCombatTime()), 95.0f);

		const AggregatedStatsEntry& totalEntry = stats.GetTotal();
		EXPECT_EQ(totalEntry.Healing, 304967);
		EXPECT_EQ(totalEntry.Hits, 727);

		AggregatedVector expectedTotals;
		expectedTotals.Add(0, "Group", 207634, 449, std::nullopt);
		expectedTotals.Add(0, "Squad", 304967, 727, std::nullopt);
		expectedTotals.Add(0, "All (Excluding Summons)", 304967, 727, std::nullopt);
		expectedTotals.Add(0, "All (Including Summons)", 409220, 1186, std::nullopt);

		const AggregatedVector& totals = stats.GetStats(DataSource::Totals);
		ASSERT_EQ(totals.Entries.size(), expectedTotals.Entries.size());
		EXPECT_EQ(totals.HighestHealing, expectedTotals.HighestHealing);
		for (uint32_t i = 0; i < expectedTotals.Entries.size(); i++)
		{
			EXPECT_EQ(totals.Entries[i].GetTie(), expectedTotals.Entries[i].GetTie());
		}

		AggregatedVector expectedAgents;
		expectedAgents.Add(2000, "Zarwae", 51011, 135, std::nullopt);
		expectedAgents.Add(3148, "Apocalypse Dawn", 47929, 89, std::nullopt);
		expectedAgents.Add(3150, "Waiana Sulis", 40005, 86, std::nullopt);
		expectedAgents.Add(3149, "And Avr Two L Q E A", 39603, 71, std::nullopt);
		expectedAgents.Add(3144, "Taya Celeste", 29086, 68, std::nullopt);
		expectedAgents.Add(3145, "Teivarus", 26490, 71, std::nullopt);
		expectedAgents.Add(3151, "Janna Larion", 21902, 71, std::nullopt);
		expectedAgents.Add(3137, "Lady Manyak", 20637, 52, std::nullopt);
		expectedAgents.Add(3146, "Akashi Vi Britannia", 20084, 55, std::nullopt);
		expectedAgents.Add(3147, u8"Moa Fhómhair", 8220, 29, std::nullopt);

		const AggregatedVector& agents = stats.GetStats(DataSource::Agents);
		ASSERT_EQ(agents.Entries.size(), expectedAgents.Entries.size());
		EXPECT_EQ(agents.HighestHealing, expectedAgents.HighestHealing);
		for (uint32_t i = 0; i < expectedAgents.Entries.size(); i++)
		{
			EXPECT_EQ(agents.Entries[i].GetTie(), expectedAgents.Entries[i].GetTie());
		}
	}

	// This is not really realistic - peer and local has same name, id, etc. But everything can handle it fine.
	auto [localId2, states2] = Processor.GetState();

	AggregatedStatsCollection stats{std::move(states2), localId2, options.Windows[9], false};
	const AggregatedStatsEntry& totalEntry = stats.GetTotal(DataSource::PeersOutgoing);
	EXPECT_EQ(totalEntry.Healing, 304967*2);
	EXPECT_EQ(totalEntry.Hits, 727*2);

	AggregatedVector expectedStats;
	expectedStats.Add(2000, "Zarwae", 304967, 727, std::nullopt);
	expectedStats.Add(100000, "Local Zarwae", 304967, 727, std::nullopt);
	
	const AggregatedVector& actualStats = stats.GetStats(DataSource::PeersOutgoing);
	ASSERT_EQ(actualStats.Entries.size(), expectedStats.Entries.size());
	EXPECT_EQ(actualStats.HighestHealing, expectedStats.HighestHealing);
	for (uint32_t i = 0; i < expectedStats.Entries.size(); i++)
	{
		EXPECT_EQ(actualStats.Entries[i].GetTie(), expectedStats.Entries[i].GetTie());
	}
}

INSTANTIATE_TEST_SUITE_P(
	Normal,
	NetworkXevtcTestFixture,
	::testing::Values(0u));
