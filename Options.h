#pragma once

#include <stdint.h>

struct HealTableOptions
{
	bool ShowHealWindow;

	bool ShowTotals;
	bool ShowAgents;
	bool ShowSkills;

	int SortOrderChoice;
	int GroupFilterChoice;

	int HealTableHotkey;

	bool ExcludeUnmappedAgents;
	bool DebugMode;
};

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);