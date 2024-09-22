#pragma once

#include "AggregatedStatsCollection.h"
#include "Log.h"
#include "State.h"

#pragma warning(push, 0)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#pragma warning(disable : 28183)
#include <nlohmann/json.hpp>
#pragma warning(pop)

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

enum class AutoUpdateSettingEnum : uint8_t
{
	Off = 0,
	On = 1,
	PreReleases = 2,
	Max = 3
};

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

	ImGuiID WindowId = 0; // In-Memory only

	float LastFrameMinWidth = 0.0f; // In-Memory only
	size_t CurrentFrameLineCount = 0; // In-Memory only
};

struct HealTableOptions
{
	AutoUpdateSettingEnum AutoUpdateSetting = AutoUpdateSettingEnum::On;
	bool DebugMode = false;
	bool IncludeBarrier = false;
	spdlog::level::level_enum LogLevel = spdlog::level::off;

	bool EvtcLoggingEnabled = true;

	char EvtcRpcEndpoint[128] = "evtc-rpc.kappa322.com";
	bool EvtcRpcEnabled = false;
	bool EvtcRpcBudgetMode = false;
	bool EvtcRpcDisableEncryption = false;
	int EvtcRpcEnabledHotkey = 0;

	std::array<HealWindowContext, HEAL_WINDOW_COUNT> Windows;

	std::vector<ImGuiID> AnchoringHighlightedWindows; // In-Memory only

	HealTableOptions();
	~HealTableOptions() = default;

	void Load(const char* pConfigPath);
	bool Save(const char* pConfigPath) const;

	void FromJson(const nlohmann::json& pJsonObject);
	void ToJson(nlohmann::json& pJsonObject) const;

	void Reset();
};
static_assert(std::is_same<std::underlying_type<spdlog::level::level_enum>::type, int>::value == true, "HealTableOptions::LogLevel size changed");

template<>
struct fmt::formatter<HealTableOptions> : SimpleFormatter
{
	// Formats the point p using the parsed format specification (presentation)
	// stored in this formatter.
	template <typename FormatContext>
	auto format(const HealTableOptions& pObject, FormatContext& pContext) -> decltype(pContext.out())
	{
		nlohmann::json jsonObject;
		pObject.ToJson(jsonObject);

		return fmt::format_to(
			pContext.out(),
			"{}",
			jsonObject.dump());
	}
};