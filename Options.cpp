#include "Options.h"

#include "AggregatedStats.h"
#include "Log.h"

#include "simpleini/SimpleIni.h"

static CSimpleIniA healtable_ini(true /* isUtf8 */);

#define BOOL_TO_INTSTRING(pBool) pBool == true ? "1" : "0"

void WriteIni(const HealTableOptions& pOptions)
{
	char buffer[1024];

	// Store a version so that we can possibly translate ini file in the future (for example if some breaking change is made to enum values)
	SI_Error error = healtable_ini.SetValue("ini", "version", "1");
	if (error < 0)
	{
		LOG("SetValue version failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "show_heal_window", BOOL_TO_INTSTRING(pOptions.ShowHealWindow));
	if (error < 0)
	{
		LOG("SetValue show_heal_window failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "show_totals", BOOL_TO_INTSTRING(pOptions.ShowTotals));
	if (error < 0)
	{
		LOG("SetValue show_totals failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "show_agents", BOOL_TO_INTSTRING(pOptions.ShowAgents));
	if (error < 0)
	{
		LOG("SetValue show_agents failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "show_skills", BOOL_TO_INTSTRING(pOptions.ShowSkills));
	if (error < 0)
	{
		LOG("SetValue show_skills failed with %i", error);
	}

	snprintf(buffer, sizeof(buffer), "%i", pOptions.SortOrderChoice);
	error = healtable_ini.SetValue("settings", "sort_order_choice", buffer);
	if (error < 0)
	{
		LOG("SetValue sort_order_choice failed with %i", error);
	}

	snprintf(buffer, sizeof(buffer), "%i", pOptions.GroupFilterChoice);
	error = healtable_ini.SetValue("settings", "group_filter_choice", buffer);
	if (error < 0)
	{
		LOG("SetValue group_filter_choice failed with %i", error);
	}

	snprintf(buffer, sizeof(buffer), "%i", pOptions.HealTableHotkey);
	error = healtable_ini.SetValue("settings", "heal_table_hotkey", buffer);
	if (error < 0)
	{
		LOG("SetValue heal_table_hotkey failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "exclude_unmapped_agents", BOOL_TO_INTSTRING(pOptions.ExcludeUnmappedAgents));
	if (error < 0)
	{
		LOG("SetValue exclude_unmapped_agents failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "debug_mode", BOOL_TO_INTSTRING(pOptions.DebugMode));
	if (error < 0)
	{
		LOG("SetValue debug_mode failed with %i", error);
	}

	error = healtable_ini.SaveFile("addons\\arcdps\\arcdps_healing_stats.ini");
	if (error < 0)
	{
		LOG("SaveFile failed with %i", error);
	}
}

void ReadIni(HealTableOptions& pOptions)
{
	SI_Error error = healtable_ini.LoadFile("addons\\arcdps\\arcdps_healing_stats.ini");
	if (error < 0)
	{
		LOG("LoadFile failed with %i", error);
	}

	const char* stringValue = healtable_ini.GetValue("settings", "show_heal_window", "0");
	pOptions.ShowHealWindow = (atoi(stringValue) != 0);

	stringValue = healtable_ini.GetValue("settings", "show_totals", "1");
	pOptions.ShowTotals = (atoi(stringValue) != 0);

	stringValue = healtable_ini.GetValue("settings", "show_agents", "1");
	pOptions.ShowAgents = (atoi(stringValue) != 0);

	stringValue = healtable_ini.GetValue("settings", "show_skills", "1");
	pOptions.ShowSkills = (atoi(stringValue) != 0);

	stringValue = healtable_ini.GetValue("settings", "sort_order_choice", "3");
	pOptions.SortOrderChoice = atoi(stringValue);

	stringValue = healtable_ini.GetValue("settings", "group_filter_choice", "1");
	pOptions.GroupFilterChoice = atoi(stringValue);

	stringValue = healtable_ini.GetValue("settings", "heal_table_hotkey", "0");
	pOptions.HealTableHotkey = atoi(stringValue);

	stringValue = healtable_ini.GetValue("settings", "exclude_unmapped_agents", "1");
	pOptions.ExcludeUnmappedAgents = (atoi(stringValue) != 0);

	stringValue = healtable_ini.GetValue("settings", "debug_mode", "0");
	pOptions.DebugMode = (atoi(stringValue) != 0);

	LOG("Read options from ini file: show_heal_window=%s show_totals=%s show_agents=%s show_skills=%s sort_order_choice=%i group_filter_choice=%i heal_table_hotkey=%i exclude_unmapped_agents=%s debug_mode=%s",
		BOOL_STR(pOptions.ShowHealWindow), BOOL_STR(pOptions.ShowTotals), BOOL_STR(pOptions.ShowAgents), BOOL_STR(pOptions.ShowSkills), pOptions.SortOrderChoice, pOptions.GroupFilterChoice, pOptions.HealTableHotkey, BOOL_STR(pOptions.ExcludeUnmappedAgents), BOOL_STR(pOptions.DebugMode));
}
