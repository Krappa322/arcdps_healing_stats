#include "Options.h"

#include "AggregatedStats.h"
#include "Log.h"

#include "simpleini/SimpleIni.h"

static CSimpleIniA healtable_ini(true /* isUtf8 */);

void WriteIni(const HealTableOptions& pOptions)
{
	// Store a version so that we can possibly translate ini file in the future (for example if some breaking change is made to enum values)
	SI_Error error = healtable_ini.SetLongValue("ini", "version", 2);
	if (error < 0)
	{
		LOG("SetValue version failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "show_heal_window", pOptions.ShowHealWindow);
	if (error < 0)
	{
		LOG("SetValue show_heal_window failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "show_totals", pOptions.ShowTotals);
	if (error < 0)
	{
		LOG("SetValue show_totals failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "show_agents", pOptions.ShowAgents);
	if (error < 0)
	{
		LOG("SetValue show_agents failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "show_skills", pOptions.ShowSkills);
	if (error < 0)
	{
		LOG("SetValue show_skills failed with %i", error);
	}

	error = healtable_ini.SetLongValue("settings", "sort_order_choice", pOptions.SortOrderChoice);
	if (error < 0)
	{
		LOG("SetValue sort_order_choice failed with %i", error);
	}

	error = healtable_ini.SetLongValue("settings", "group_filter_choice", pOptions.GroupFilterChoice);
	if (error < 0)
	{
		LOG("SetValue group_filter_choice failed with %i", error);
	}

	error = healtable_ini.SetLongValue("settings", "heal_table_hotkey", pOptions.HealTableHotkey);
	if (error < 0)
	{
		LOG("SetValue heal_table_hotkey failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "exclude_unmapped_agents", pOptions.ExcludeUnmappedAgents);
	if (error < 0)
	{
		LOG("SetValue exclude_unmapped_agents failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "debug_mode", pOptions.DebugMode);
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

static void ReadIni_V1(HealTableOptions& pOptions)
{
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

void ReadIni(HealTableOptions& pOptions)
{
	SI_Error error = healtable_ini.LoadFile("addons\\arcdps\\arcdps_healing_stats.ini");
	if (error < 0)
	{
		LOG("LoadFile failed with %i", error);
	}

	uint32_t version = healtable_ini.GetLongValue("ini", "version");
	if (version == 1)
	{
		return ReadIni_V1(pOptions);
	}

	pOptions.ShowHealWindow = healtable_ini.GetBoolValue("settings", "show_heal_window", false);
	pOptions.ShowTotals = healtable_ini.GetBoolValue("settings", "show_totals", true);
	pOptions.ShowAgents = healtable_ini.GetBoolValue("settings", "show_agents", true);
	pOptions.ShowSkills = healtable_ini.GetBoolValue("settings", "show_skills", true);

	pOptions.SortOrderChoice = healtable_ini.GetLongValue("settings", "sort_order_choice", static_cast<long>(SortOrder::DescendingSize));
	pOptions.GroupFilterChoice = healtable_ini.GetLongValue("settings", "group_filter_choice", static_cast<long>(GroupFilter::Squad));
	pOptions.HealTableHotkey = healtable_ini.GetLongValue("settings", "heal_table_hotkey", 0);

	pOptions.ExcludeUnmappedAgents = healtable_ini.GetBoolValue("settings", "exclude_unmapped_agents", true);
	pOptions.DebugMode = healtable_ini.GetBoolValue("settings", "debug_mode", false);

	LOG("Read options from ini file: show_heal_window=%s show_totals=%s show_agents=%s show_skills=%s sort_order_choice=%i group_filter_choice=%i heal_table_hotkey=%i exclude_unmapped_agents=%s debug_mode=%s",
		BOOL_STR(pOptions.ShowHealWindow), BOOL_STR(pOptions.ShowTotals), BOOL_STR(pOptions.ShowAgents), BOOL_STR(pOptions.ShowSkills), pOptions.SortOrderChoice, pOptions.GroupFilterChoice, pOptions.HealTableHotkey, BOOL_STR(pOptions.ExcludeUnmappedAgents), BOOL_STR(pOptions.DebugMode));
}
