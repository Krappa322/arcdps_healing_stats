#pragma warning(push, 0)
#pragma warning(disable : 4005)
#pragma warning(disable : 4389)
#pragma warning(disable : 26439)
#pragma warning(disable : 26495)
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

namespace
{
const static grpc::SslServerCredentialsOptions::PemKeyCertPair UNIT_TEST_CERT_PAIR
{
R"STRING_LITERAL(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----)STRING_LITERAL",
R"STRING_LITERAL(-----BEGIN CERTIFICATE-----
MIIFGDCCAwACAQEwDQYJKoZIhvcNAQELBQAwUDELMAkGA1UEBhMCU0UxETAPBgNV
BAgMCEJsZWtpbmdlMQ0wCwYDVQQKDARUZXN0MQ0wCwYDVQQLDARUZXN0MRAwDgYD
VQQDDAdSb290IENBMB4XDTIxMDUxODIxNDIwMloXDTMxMDUxNjIxNDIwMlowVDEL
MAkGA1UEBhMCU0UxETAPBgNVBAgMCEJsZWtpbmdlMQ0wCwYDVQQKDARUZXN0MQ8w
DQYDVQQLDAZTZXJ2ZXIxEjAQBgNVBAMMCWxvY2FsaG9zdDCCAiIwDQYJKoZIhvcN
AQEBBQADggIPADCCAgoCggIBAKHWzfohGkzLG5lywlSEW08xWEXJOZO8eZx///iP
Jhe8MUL74k1evwg5cpRFcZUNZay0JqGCKYc0JU/GIJhWMl6ma7ZgukzYw+aAsnPN
UxZoJYWZxCcJFqWVyRHlS6Bv+YgERAEiMgMAfhTdGOIMKliOxwuYk+6C+U5avXBL
xeFddho5QDk6NxXnMVE8uMM0aq1uUCyjpk06cAXU+PMQkBbPVH5Hh7MHD4L/+uo6
v0H/QdHvBgRg6IzQhCmCwQdd1JMcxfuEi1wjQnZh9MwNo57l34anV6381u4d6VAX
HfB+j7BppBIu4MxROzSr//Q9nkpcf3QFVJ/sPuKmBCoX2vEk99idUPzow/26VP8L
zzKkeDPhj/Uhu50A68f3lFng1Fltdk4DT4ufPWBKHUPCbsc4I68SkB0vdWMARjJg
NNqm3uuN2hdVOObfrKD7fPS0WTPl3gkrsNBMBS377tkliFjiEFPlBcbC2nlVklOI
aF6y1aST4zVrhg0vy5aRkDW55QnrCom3J5ZK/dPxDlPZSD5gTB75zl4OIUrnKkvg
BNPT5PfhNgADe/OrGQFQq+mqygYYHI1M01CNNLnsnbeQ64ofqdrOPvBf8dw5olZ4
p7zI4hiiP0Y0dnIsI0Sa8xzQEbGvwFAIqXbiUvGrZVvEap0Kmuul02Sv+KFzE/Ni
RxHTAgMBAAEwDQYJKoZIhvcNAQELBQADggIBAEm4w0XAB2p48sjjoRgBDWtrWzpB
6733Ba0ScIt4GaHunsSg+YUNltVWK2Pf1UmCxBIVq/cuu8vf7rvf6gAVxDMKkWVI
YVGkKoTCTBP83EuvqLOjI7ZggNNNbw+96bbl71khpLWVJ/WPgPg14Q8m/WVpdBvl
riyiuTXeTZo3C3luHALXU1VAeM1geuAY6/thWETmbzimqWcDr8AEkj4JXMV1FGmk
DsaeDoEP3v91MfRULDy7bUW5v2I6HFzxiHywz1wXyKYUL/sdYiYXnlbqqhbp89M+
nlm3ltqcGfwXy+ChF9m8pUc5zpjreEgP3LKT1hX1kAj28unCcr0ja8aDa7gNr0C/
lTNnt+LkO2MpTDdAKtrjAgz9fd1nDr5+hdRB3sL09gCxEioP4AU3D5fg3kD0KmCr
VcDmG0o97pyj3USkTadSE/wxgvQjo12jtCu1K8TYHgejqE//qcAOGykjzg0m9kr9
DOm27GEcYzBu2ip6vPo9YOpTQtMSTXvuw/cs3WfjMHFBLPy/gIZ06gQtm3Ue9E00
IQfrhEVkJfWjQWgrH3yIP6RXC08/hikNCyy91AvjbA+4xxDm1qr2yBdnNdKWWqJm
QvXw5hn+LXrbndRrxhvrnhilvmSEO7Ocbr2vtBnrpxHpycKyRn1We8hWkyIyzUPA
NOpvdk1FH2CajXCT
-----END CERTIFICATE-----)STRING_LITERAL"
};

const static std::string UNIT_TEST_CA =
R"STRING_LITERAL(-----BEGIN CERTIFICATE-----
MIIFgTCCA2mgAwIBAgIUJ6HGrj/fSWM/GT6DBXHnPV9EohkwDQYJKoZIhvcNAQEL
BQAwUDELMAkGA1UEBhMCU0UxETAPBgNVBAgMCEJsZWtpbmdlMQ0wCwYDVQQKDARU
ZXN0MQ0wCwYDVQQLDARUZXN0MRAwDgYDVQQDDAdSb290IENBMB4XDTIxMDUxODIx
NDIwMFoXDTMxMDUxNjIxNDIwMFowUDELMAkGA1UEBhMCU0UxETAPBgNVBAgMCEJs
ZWtpbmdlMQ0wCwYDVQQKDARUZXN0MQ0wCwYDVQQLDARUZXN0MRAwDgYDVQQDDAdS
b290IENBMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAuPFhEIMkRflv
M/o5RdAXD3j04uIE9DUskRjWEAPdZaWu0uIahfhrJXzO9ciiBixNa1NwGZyogXPC
DhYcs6whLJUkyGj44bLr5YokW/VTUsxP6happwSQQm5yQCQGFSs69cSQNnG3IzO2
HEotiMtVnpgC1AuQwJ8F4Vhg79fWxsO7LdceGdf/DRVpP2qTdonA7sMmpkRevkKO
0MYrChIiakui1xi33eh91DugdzisUOA8LUrspiX29djpWj9s7CadrXddie+RIHMH
HdzZ0Zap31DASoqELcAo9B1Yl3IQmqD7dCrKwEu5ZoS+TUdm3gtI9hZ+Fw1IjVyw
FeL3Gw4KzKyHEewEyJ1q/AsSdSzpXsezfCr94wP5NT3fmHYAxVaKwkgTo6cQEWi2
9T+0RA2WVhCRTtx6koqngWFO2/gwL4NgvIL8why/qbbT9SyUIVuzxkQu6GKSGh28
rE4LZ1pHKagtJgXdwic+Eh0czWRcmVV55K6gfYcewghGzvDbi0TtPoJcQCPT0Det
G1ezN0hl6ekAgisHx1YPTZYHRDBIiLoUWRHrNxQpHU7Whk1dFx4hRTu1LbU9LzU9
8jeOFSCq8YUQpNoyp7Vi3VhUVQYjrqOfNPitrl1YoBEKmIASKpdQJoKub/wnMI1i
+ArkAgzMICbah6R3oni9DU5X88tMxh8CAwEAAaNTMFEwHQYDVR0OBBYEFBCBNBlP
W/SedwkpRVipiWZg6lMKMB8GA1UdIwQYMBaAFBCBNBlPW/SedwkpRVipiWZg6lMK
MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggIBAESKEhlLNrqArH2+
w63mHtuqSWYpNG39btXdRuSNLZf6eOfXZGB2svaM0c07MjPq2z0i+jQ0VA0nT3uD
KmV+6lpsaWgws4m2CremW541y90RArT/Ca54ziBl27BW/qLnr/l+74o+Pw3VyM+k
flMPMZR23FFIcIFCRQvkA3ubSfFioRNMKdPHVBedOKU1nxlcIVMPxpaTGqeH034z
gvpZR+uCYdiUDb71lEGlUJTiU0WWshlYsWWQ3KTFCm3K7jiyOh7tkbtO9fvT9G81
n3/1cMfxvvCK4bKlO9t8s8bSZd0oM/eY7DYW4XlNub0/N7zjB6ptTRBQNdQ9G6Fl
jCQbW6lms+SlsYHkFyq1xmnVRCdsEz8g2rmMtYHEvr1HABbcsRCV+rZwvdDoQnl3
y9PmHEui2WD9ypkouapgXTWxLEY48sC1FDAEXv+tS8MNNXtGTcto3TUMpqjNyhu6
Gnb5sqGws3KCuW3ObPgoaDZMF+/Wc8AneLSiu4eXH4qJmTYFrPVrqqVHD7y4c5Uk
8d+uVXKol3GT53ghrhErIs/Zr3Chav9Mgu6i7VhrrgYsDUOijvkySBDSPuFzywQs
gW58G2yLyQQNZRjFJQMTkUdDo54/y+AgNfNf3xaBYRIGqcxSbWP/UTogSxD0HNAH
zUjXeAdwm/NFW/NuH1osYu9zny71
-----END CERTIFICATE-----)STRING_LITERAL";

void FillRandomData(void* pBuffer, size_t pBufferSize)
{
	for (size_t i = 0; i < pBufferSize; i++)
	{
		static_cast<byte*>(pBuffer)[i] = rand() % 256;
	}
}
} // anonymous namespace

bool operator==(const cbtevent& pLeft, const cbtevent& pRight)
{
	return memcmp(&pLeft, &pRight, sizeof(pLeft)) == 0;
}

class SimpleNetworkTestFixture : public ::testing::Test
{
protected:
	void SetUp() override
	{
		uint64_t seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(static_cast<uint32_t>(seed));

		grpc::SslServerCredentialsOptions server_credentials_options;
		server_credentials_options.pem_root_certs = UNIT_TEST_CA;
		server_credentials_options.pem_key_cert_pairs.push_back(UNIT_TEST_CERT_PAIR);
		Server = std::make_unique<evtc_rpc_server>("localhost:50051", &server_credentials_options);
		mServerThread = std::make_unique<std::thread>(evtc_rpc_server::ThreadStartServe, Server.get());
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
		Server->Shutdown();
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
		auto getCertificates = []() -> std::string
			{
				return std::string{UNIT_TEST_CA};
			}; 
		newClient->Client = std::make_unique<evtc_rpc_client>(std::move(getEndpoint), std::move(getCertificates), std::move(eventhandler));

		mClientThreads.emplace_back(evtc_rpc_client::ThreadStartServe, mClients.back()->Client.get());
		return *newClient.get();
	}

// A bug in grpc memory leak detection causes Server shutdown to detect leaks if there are non-destroyed clients
// active. Therefore we have to make sure that the clients are shutdown first (which leads to the strange
// private-public-private order)
private:
	std::vector<std::unique_ptr<ClientInstance>> mClients;

public:
	std::unique_ptr<evtc_rpc_server> Server;

private:
	std::unique_ptr<std::thread> mServerThread;
	std::vector<std::thread> mClientThreads;
};

// Parameters are <register before disabling>
class DisableClientTestFixture : public SimpleNetworkTestFixture, public testing::WithParamInterface<bool>
{
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
		get_init_addr("unit_test", nullptr, nullptr, GetModuleHandle(NULL), malloc, free); // Initialize exports

		uint64_t seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(static_cast<uint32_t>(seed));

		grpc::SslServerCredentialsOptions server_credentials_options;
		server_credentials_options.pem_root_certs = UNIT_TEST_CA;
		server_credentials_options.pem_key_cert_pairs.push_back(UNIT_TEST_CERT_PAIR);
		Server = std::make_unique<evtc_rpc_server>("localhost:50051", &server_credentials_options);

		auto eventhandler = [](cbtevent* /*pEvent*/, uint16_t /*pInstanceId*/)
		{
			// Do nothing
		};
		auto getEndpoint = []() -> std::string
			{
				return std::string{"localhost:50051"};
			};
		auto getCertificates = []() -> std::string
			{
				return std::string{UNIT_TEST_CA};
			}; 
		Client = std::make_unique<evtc_rpc_client>(std::function{getEndpoint}, std::function{getCertificates}, std::move(eventhandler));

		auto eventhandler2 = [this](cbtevent* pEvent, uint16_t pInstanceId)
		{
			Processor.PeerCombat(pEvent, pInstanceId);
		};
		PeerClient = std::make_unique<evtc_rpc_client>(std::move(getEndpoint), std::move(getCertificates), std::move(eventhandler2));

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
				std::lock_guard lock(Server->mRegisteredAgentsLock);
				if (Server->mRegisteredAgents.size() >= 1)
				{
					auto iter = Server->mRegisteredAgents.find("testagent.1234");
					if (iter != Server->mRegisteredAgents.end() && iter->second->InstanceId == instid)
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
			std::lock_guard lock(Server->mRegisteredAgentsLock);

			EXPECT_EQ(Server->mRegisteredAgents.size(), 1U);
			auto iter = Server->mRegisteredAgents.find("testagent.1234");
			ASSERT_NE(iter, Server->mRegisteredAgents.end());
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
			std::lock_guard lock(Server->mRegisteredAgentsLock);
			auto iter = Server->mRegisteredAgents.find("testagent.1234");
			if (iter != Server->mRegisteredAgents.end())
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
		std::lock_guard lock(Server->mRegisteredAgentsLock);

		EXPECT_EQ(Server->mRegisteredAgents.size(), 1);
		auto iter = Server->mRegisteredAgents.find("testagent.1234");
		ASSERT_NE(iter, Server->mRegisteredAgents.end());
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
			std::lock_guard lock(Server->mRegisteredAgentsLock);
			auto iter = Server->mRegisteredAgents.find("testagent.1234");
			if (iter != Server->mRegisteredAgents.end())
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
		std::lock_guard lock(Server->mRegisteredAgentsLock);

		EXPECT_EQ(Server->mRegisteredAgents.size(), 1);
		auto iter = Server->mRegisteredAgents.find("testagent.1234");
		ASSERT_NE(iter, Server->mRegisteredAgents.end());
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
			std::lock_guard lock(Server->mRegisteredAgentsLock);
			auto iter = Server->mRegisteredAgents.find("testagent.1234");
			if (iter != Server->mRegisteredAgents.end())
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
			std::lock_guard lock(Server->mRegisteredAgentsLock);
			auto iter = Server->mRegisteredAgents.find("testagent2.1234");
			if (iter != Server->mRegisteredAgents.end())
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


TEST_P(DisableClientTestFixture, DisableClient)
{
	ClientInstance& client1 = NewClient();

	if (GetParam() == true)
	{
		LogD("RegisterBeforeDisabling is true, sending events");
	}
	else
	{
		client1->SetEnabledStatus(false);
	}

	// Send a self agent registration event followed by a non-self agent registration event followed by a combat event
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

	if (GetParam() == true)
	{
		FlushEvents();

		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		bool completed = false;
		while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
		{
			{
				std::lock_guard lock(Server->mRegisteredAgentsLock);
				auto iter = Server->mRegisteredAgents.find("testagent.1234");
				if (iter != Server->mRegisteredAgents.end())
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

		LogD("Waiting for client to be registered took {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count());

		client1->SetEnabledStatus(false);
	}

	Sleep(200);

	{
		std::lock_guard lock(Server->mRegisteredAgentsLock);
		EXPECT_EQ(Server->mRegisteredAgents.size(), 0);
	}

	// Not thread safe, but shouldn't be an issue because nothing else should be writing when the client is disabled
	client1->mLastConnectionAttempt = std::chrono::steady_clock::time_point(std::chrono::seconds(0));
	// Now enable the client - the peer should be registered again
	client1->SetEnabledStatus(true);

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	bool completed = false;
	while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(100))
	{
		{
			std::lock_guard lock(Server->mRegisteredAgentsLock);
			auto iter = Server->mRegisteredAgents.find("testagent.1234");
			if (iter != Server->mRegisteredAgents.end())
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

	EXPECT_TRUE(completed);
	{
		std::lock_guard lock(Server->mRegisteredAgentsLock);

		EXPECT_EQ(Server->mRegisteredAgents.size(), 1U);
		auto iter = Server->mRegisteredAgents.find("testagent.1234");
		ASSERT_NE(iter, Server->mRegisteredAgents.end());
		EXPECT_EQ(iter->first, "testagent.1234");
		EXPECT_EQ(iter->second->Iterator, iter);
		EXPECT_EQ(iter->second->InstanceId, 10);

		auto expected_map = std::map<std::string, uint16_t>({ {"testagent2.1234", static_cast<uint16_t>(11)} });
		EXPECT_EQ(iter->second->Peers, expected_map);
	}
}

INSTANTIATE_TEST_SUITE_P(
	Normal,
	DisableClientTestFixture,
	::testing::Values(false, true));

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

	uint32_t result = Mock.ExecuteFromXevtc("xevtc_logs\\druid_MO.xevtc", parallelCallbacks, 0);
	ASSERT_EQ(result, 0U);

	Client->FlushEvents();
	PeerClient->FlushEvents();
	Sleep(1000); // TODO: Fix ugly sleep :(

	HealTableOptions options;
	Processor.mSelfUniqueId.store(100000); // Fake self having a different id
	Processor.mAgentTable.AddAgent(100000, static_cast<uint16_t>(UINT16_MAX - 1), "Local Zarwae", static_cast<uint16_t>(1), false, true);
	auto [localId, states] = Processor.GetState();

	HealingStats* localState = &states[localId].second;
	HealingStats* peerState = &states[2000].second;

	ASSERT_EQ(states.size(), 2U);
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

		float combatTime = stats.GetCombatTime();
		EXPECT_FLOAT_EQ(std::floor(combatTime), 95.0f);

		const AggregatedStatsEntry& totalEntry = stats.GetTotal();
		EXPECT_EQ(totalEntry.Healing, 304967);
		EXPECT_EQ(totalEntry.Hits, 727);

		AggregatedVector expectedTotals;
		expectedTotals.Add(0, "Group", combatTime, 207634, 449, std::nullopt);
		expectedTotals.Add(0, "Squad", combatTime, 304967, 727, std::nullopt);
		expectedTotals.Add(0, "All (Excluding Summons)", combatTime, 304967, 727, std::nullopt);
		expectedTotals.Add(0, "All (Including Summons)", combatTime, 409220, 1186, std::nullopt);

		const AggregatedVector& totals = stats.GetStats(DataSource::Totals);
		ASSERT_EQ(totals.Entries.size(), expectedTotals.Entries.size());
		EXPECT_EQ(totals.HighestHealing, expectedTotals.HighestHealing);
		for (uint32_t i = 0; i < expectedTotals.Entries.size(); i++)
		{
			EXPECT_EQ(totals.Entries[i].GetTie(), expectedTotals.Entries[i].GetTie());
		}

		AggregatedVector expectedAgents;
		expectedAgents.Add(2000, "Zarwae", combatTime, 51011, 135, std::nullopt);
		expectedAgents.Add(3148, "Apocalypse Dawn", combatTime, 47929, 89, std::nullopt);
		expectedAgents.Add(3150, "Waiana Sulis", combatTime, 40005, 86, std::nullopt);
		expectedAgents.Add(3149, "And Avr Two L Q E A", combatTime, 39603, 71, std::nullopt);
		expectedAgents.Add(3144, "Taya Celeste", combatTime, 29086, 68, std::nullopt);
		expectedAgents.Add(3145, "Teivarus", combatTime, 26490, 71, std::nullopt);
		expectedAgents.Add(3151, "Janna Larion", combatTime, 21902, 71, std::nullopt);
		expectedAgents.Add(3137, "Lady Manyak", combatTime, 20637, 52, std::nullopt);
		expectedAgents.Add(3146, "Akashi Vi Britannia", combatTime, 20084, 55, std::nullopt);
		expectedAgents.Add(3147, u8"Moa Fhómhair", combatTime, 8220, 29, std::nullopt);

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

	float combatTime = stats.GetCombatTime();
	AggregatedVector expectedStats;
	expectedStats.Add(2000, "Zarwae", combatTime, 304967, 727, std::nullopt);
	expectedStats.Add(100000, "Local Zarwae", combatTime, 304967, 727, std::nullopt);

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
