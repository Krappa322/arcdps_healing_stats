#pragma once

#include "AggregatedStatsCollection.h"
#include "State.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct DetailsWindowState : AggregatedStatsEntry
{
	bool IsOpen = false;

	explicit DetailsWindowState(const AggregatedStatsEntry& pEntry);
};

struct HealWindowContext : HealWindowOptions
{
	std::unique_ptr<AggregatedStatsCollection> CurrentAggregatedStats; // In-Memory only
	time_t LastAggregatedTime = 0; // In-Memory only

	std::vector<DetailsWindowState> OpenSkillWindows; // In-Memory only
	std::vector<DetailsWindowState> OpenAgentWindows; // In-Memory only
	std::vector<DetailsWindowState> OpenPeersOutgoingWindows; // In-Memory only
};

struct HealTableOptions
{
	bool DebugMode = false;
	char EvtcRpcEndpoint[128] = "evtc-rpc.kappa322.com:443";
	HealWindowContext Windows[HEAL_WINDOW_COUNT];

	HealTableOptions();
};

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);