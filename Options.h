#pragma once

struct HealTableOptions
{
	bool ShowHealWindow;
	int SortOrderChoice;
	int GroupFilterChoice;
	bool ExcludeUnmappedAgents;
	bool DebugMode;
};

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);