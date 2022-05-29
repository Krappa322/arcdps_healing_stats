#pragma once

#include "arcdps_structs.h"
#include "imgui.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <type_traits>

constexpr static uint32_t MAX_HEAL_WINDOW_NAME = 31;
constexpr static uint32_t MAX_HEAL_WINDOW_TITLE = 127;
constexpr static uint32_t MAX_HEAL_WINDOW_ENTRY = 127;
constexpr static uint32_t HEAL_WINDOW_COUNT = 10;

enum class DataSource
{
	Agents = 0,
	Skills = 1,
	Totals = 2,
	Combined = 3,
	PeersOutgoing = 4,
	Max
};

enum class SortOrder
{
	AscendingAlphabetical = 0,
	DescendingAlphabetical = 1,
	AscendingSize = 2,
	DescendingSize = 3,
	Max
};

enum class CombatEndCondition
{
	CombatExit = 0,
	LastDamageEvent = 1,
	LastHealEvent = 2,
	LastDamageOrHealEvent = 3,
	Max
};

struct HealWindowOptions
{
	bool Shown = false;

	DataSource DataSourceChoice = DataSource::Agents;
	SortOrder SortOrderChoice = SortOrder::DescendingSize;
	CombatEndCondition CombatEndConditionChoice = CombatEndCondition::LastDamageEvent;

	bool ExcludeGroup = false;
	bool ExcludeOffGroup = false;
	bool ExcludeOffSquad = false;
	bool ExcludeMinions = true;
	bool ExcludeUnmapped = true;

	bool ShowProgressBars = true;
	char Name[MAX_HEAL_WINDOW_NAME + 1] = {};
	char TitleFormat[MAX_HEAL_WINDOW_TITLE + 1] = "{1} ({4}/s, {7}s in combat)";
	char EntryFormat[MAX_HEAL_WINDOW_ENTRY + 1] = "{1} ({4}/s, {7}%)";
	char DetailsEntryFormat[MAX_HEAL_WINDOW_ENTRY + 1] = "{1} ({4}/s, {7}%)";

	ImGuiWindowFlags_ WindowFlags = ImGuiWindowFlags_None;

	int Hotkey = 0;

	Position PositionRule = Position::Manual;
	CornerPosition RelativeScreenCorner = CornerPosition::TopLeft;
	CornerPosition RelativeSelfCorner = CornerPosition::TopLeft;
	CornerPosition RelativeAnchorWindowCorner = CornerPosition::TopLeft;
	int64_t RelativeX = 0;
	int64_t RelativeY = 0;
	ImGuiID AnchorWindowId = 0;

	bool AutoResize = false;
	size_t MaxNameLength = 0;
	size_t MinLinesDisplayed = 0;
	size_t MaxLinesDisplayed = 10;
	size_t FixedWindowWidth = 400;

	void FromJson(const nlohmann::json& pJsonObject);
	void ToJson(nlohmann::json& pJsonObject, const HealWindowOptions& pDefault) const;
};
static_assert(std::is_same<std::underlying_type<DataSource>::type, int>::value == true, "HealWindowOptions::DataSourceChoice size changed");
static_assert(std::is_same<std::underlying_type<SortOrder>::type, int>::value == true, "HealWindowOptions::SortOrderChoice size changed");
static_assert(std::is_same<std::underlying_type<CombatEndCondition>::type, int>::value == true, "HealWindowOptions::CombatEndConditionChoice size changed");
static_assert(std::is_same<std::underlying_type<ImGuiWindowFlags_>::type, int>::value == true, "HealWindowOptions::WindowFlags size changed");
