#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "EventProcessor.h"

TEST(EventProcessorTest, ImplicitSelfCombatExit)
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

TEST(EventProcessorTest, ImplicitPeerCombatExit)
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
