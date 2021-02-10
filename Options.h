#pragma once

struct HealTableOptions
{
	bool ShowHealWindow;

	bool ShowTotals;
	bool ShowAgents;
	bool ShowSkills;

	int SortOrderChoice;
	int GroupFilterChoice;
	bool ExcludeUnmappedAgents;
	bool DebugMode;
};

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);