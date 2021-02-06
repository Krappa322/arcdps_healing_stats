#pragma once

struct HealTableOptions
{
	bool ShowHealWindow;
	int SortOrderChoice;
	int GroupFilterChoice;
	bool ExcludeUnmappedAgents;
};

void WriteIni(const HealTableOptions& pOptions);
void ReadIni(HealTableOptions& pOptions);