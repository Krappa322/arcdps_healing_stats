#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "AddonVersion.h"
#include "EventProcessor.h"
#include "Exports.h"
#include "Utilities.h"

TEST(EventProcessorTest, ImplicitSelfCombatExitOnSelfDeregister)
{
	EventProcessor processor;
	
	// Register "local.1234"
	ag source_ag{};
	ag dest_ag{};
	source_ag.elite = 0; // agent registration
	source_ag.prof = static_cast<Prof>(1); // agent registration
	source_ag.id = 1000;
	dest_ag.id = 100;
	source_ag.name = "local";
	dest_ag.name = "local.1234";
	dest_ag.self = true;
	processor.LocalCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Enter combat
	cbtevent ev{};
	ev.src_agent = 1000;
	ev.src_instid = 100;
	ev.is_statechange = CBTS_ENTERCOMBAT;
	ev.time = timeGetTime() - 1;
	source_ag.self = true;
	processor.LocalCombat(&ev, &source_ag, &dest_ag, nullptr, 0, 0);

	// Get state, should be in combat
	auto state = processor.GetState();
	EXPECT_EQ(state.first, 1000);
	auto local_state = state.second.find(state.first);
	ASSERT_NE(local_state, state.second.end());
	EXPECT_EQ(local_state->second.second.ExitedCombatTime, 0);

	// Deregister self
	source_ag.elite = 0; // agent deregistration
	source_ag.prof = static_cast<Prof>(0); // agent deregistration
	source_ag.self = false; // this is part of destination agent for registrations
	processor.LocalCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Get state, shouldn't be in combat
	state = processor.GetState();
	EXPECT_EQ(state.first, 1000);
	local_state = state.second.find(state.first);
	ASSERT_NE(local_state, state.second.end());
	EXPECT_NE(local_state->second.second.ExitedCombatTime, 0);
}

TEST(EventProcessorTest, ImplicitPeerCombatExitOnSelfDeregister)
{
	EventProcessor processor;

	// Register "peer.1234"
	ag source_ag{};
	ag dest_ag{};
	source_ag.elite = 0; // agent registration
	source_ag.prof = static_cast<Prof>(1); // agent registration
	source_ag.id = 2000;
	dest_ag.id = 200;
	source_ag.name = "peer";
	dest_ag.name = "peer.1234";
	processor.AreaCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Enter combat
	cbtevent ev{};
	ev.src_agent = 2000;
	ev.src_instid = 200;
	ev.is_statechange = CBTS_ENTERCOMBAT;
	ev.time = timeGetTime() - 1;
	processor.PeerCombat(&ev, 200);

	// Get state, should be in combat
	auto state = processor.GetState();
	auto peer_state = state.second.find(2000);
	ASSERT_NE(peer_state, state.second.end());
	EXPECT_EQ(peer_state->second.second.ExitedCombatTime, 0);

	// Deregister self
	source_ag.elite = 0; // agent deregistration
	source_ag.prof = static_cast<Prof>(0); // agent deregistration
	processor.AreaCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Get state, shouldn't be in combat
	state = processor.GetState();
	peer_state = state.second.find(2000);
	ASSERT_NE(peer_state, state.second.end());
	EXPECT_NE(peer_state->second.second.ExitedCombatTime, 0);
}

TEST(EventProcessorTest, ResetPeerOnSelfCombatEnter)
{
	EventProcessor processor;

	// Register "peer1.1234"
	ag source_ag{};
	ag dest_ag{};
	source_ag.elite = 0; // agent registration
	source_ag.prof = static_cast<Prof>(1); // agent registration
	source_ag.id = 2001;
	dest_ag.id = 101;
	source_ag.name = "peer1";
	dest_ag.name = "peer1.1234";
	processor.AreaCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);
	
	// Register "peer2.1234"
	source_ag.id = 2002;
	dest_ag.id = 102;
	source_ag.name = "peer2";
	dest_ag.name = "peer2.1234";
	processor.AreaCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Register "local.1234"
	source_ag.id = 2000;
	dest_ag.id = 100;
	source_ag.name = "local";
	dest_ag.name = "local.1234";
	dest_ag.self = true;
	processor.LocalCombat(nullptr, &source_ag, &dest_ag, nullptr, 0, 0);

	// Enter combat with all 3
	cbtevent ev{};
	ev.src_agent = 2000;
	ev.src_instid = 100;
	ev.is_statechange = CBTS_ENTERCOMBAT;
	ev.time = timeGetTime() - 1;
	source_ag.self = true;
	processor.LocalCombat(&ev, &source_ag, &dest_ag, nullptr, 0, 0);

	ev.src_agent = 2001;
	ev.src_instid = 101;
	processor.PeerCombat(&ev, 101);

	ev.src_agent = 2002;
	ev.src_instid = 102;
	processor.PeerCombat(&ev, 102);

	// Exit combat with self and peer1
	ev.src_agent = 2000;
	ev.src_instid = 100;
	ev.is_statechange = CBTS_EXITCOMBAT;
	ev.time += 1;
	processor.LocalCombat(&ev, &source_ag, &dest_ag, nullptr, 0, 0);

	ev.src_agent = 2001;
	ev.src_instid = 101;
	processor.PeerCombat(&ev, 101);

	// Enter combat with self
	ev.src_agent = 2000;
	ev.src_instid = 100;
	ev.is_statechange = CBTS_ENTERCOMBAT;
	ev.time += 1;
	processor.LocalCombat(&ev, &source_ag, &dest_ag, nullptr, 0, 0);

	// Get state, self and peer2 should be in combat, peer1 should be completely reset
	auto state = processor.GetState();
	EXPECT_EQ(state.second.size(), 3);

	auto self_state = state.second.find(2000);
	ASSERT_NE(self_state, state.second.end());
	EXPECT_FALSE(self_state->second.second.IsOutOfCombat());
	EXPECT_NE(self_state->second.second.EnteredCombatTime, 0);
	EXPECT_EQ(self_state->second.second.ExitedCombatTime, 0);

	auto peer1_state = state.second.find(2001);
	ASSERT_NE(peer1_state, state.second.end());
	EXPECT_TRUE(peer1_state->second.second.IsOutOfCombat());
	EXPECT_EQ(peer1_state->second.second.EnteredCombatTime, 0);
	EXPECT_EQ(peer1_state->second.second.ExitedCombatTime, 0); // peer1 did exit combat, and its in combat duration should've been reset when self entered combat

	auto peer2_state = state.second.find(2002);
	ASSERT_NE(peer2_state, state.second.end());
	EXPECT_FALSE(peer2_state->second.second.IsOutOfCombat());
	EXPECT_NE(peer2_state->second.second.EnteredCombatTime, 0);
	EXPECT_EQ(peer2_state->second.second.ExitedCombatTime, 0);
}

cbtevent VERSION_EVENT;
static void e9_ExpectVersionEvent(cbtevent* pEvent, uint32_t pSignature)
{
	VERSION_EVENT = *pEvent;
	VERSION_EVENT.time = timeGetTime();
	VERSION_EVENT.is_statechange = CBTS_EXTENSION;
	memcpy(&VERSION_EVENT.pad61, &pSignature, sizeof(pSignature));

	EXPECT_EQ(pSignature, VERSION_EVENT_SIGNATURE);
	EvtcVersionHeader versionHeader;
	memcpy(&versionHeader, &pEvent->src_agent, sizeof(pEvent->src_agent));

	EXPECT_EQ(versionHeader.EvtcRevision, HEALING_STATS_EVTC_REVISION);
	EXPECT_EQ(versionHeader.Signature, HEALING_STATS_ADDON_SIGNATURE);
	EXPECT_EQ(versionHeader.VersionStringLength, constexpr_strlen(HEALING_STATS_VERSION));

	std::string_view version = std::string_view{reinterpret_cast<char*>(&pEvent->dst_agent), versionHeader.VersionStringLength};
	EXPECT_EQ(version, HEALING_STATS_VERSION);
}

TEST(EventProcessorTest, LogStart)
{
	EventProcessor processor;

	GlobalObjects::ARC_E9 = e9_ExpectVersionEvent;
	cbtevent ev = {};
	ev.is_statechange = CBTS_LOGSTART;
	processor.AreaCombat(&ev, nullptr, nullptr, nullptr, 0, 0);

	processor.AreaCombat(&VERSION_EVENT, nullptr, nullptr, nullptr, 0, 0);
	processor.LocalCombat(&VERSION_EVENT, nullptr, nullptr, nullptr, 0, 0);

	GlobalObjects::ARC_E9 = nullptr;
}