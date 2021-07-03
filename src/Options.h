#pragma once

#include "AggregatedStatsCollection.h"
#include "Log.h"
#include "State.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
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
	spdlog::level::level_enum LogLevel = spdlog::level::off;
	
	bool EvtcLoggingEnabled = true;

	char EvtcRpcEndpoint[128] = "evtc-rpc.kappa322.com:443";
	bool EvtcRpcEnabled = false;

	HealWindowContext Windows[HEAL_WINDOW_COUNT];

	HealTableOptions();
};
static_assert(std::is_same<std::underlying_type<spdlog::level::level_enum>::type, int>::value == true, "HealTableOptions::LogLevel size changed");

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);