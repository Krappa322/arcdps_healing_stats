#pragma once

#include <stdint.h>

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

	int Hotkey = 0;
};
static_assert(std::is_same<std::underlying_type<DataSource>::type, int>::value == true, "HealWindowOptions::DataSourceChoice size changed");
