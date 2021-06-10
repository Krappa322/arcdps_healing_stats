#include "Options.h"

#include "AggregatedStats.h"
#include "Log.h"

#include "SimpleIni.h"

static CSimpleIniA healtable_ini(true /* isUtf8 */);

void WriteIni(const HealTableOptions& pOptions)
{
	// Store a version so that we can possibly translate ini file in the future (for example if some breaking change is made to enum values)
	SI_Error error = healtable_ini.SetLongValue("ini", "version", 3);
	if (error < 0)
	{
		LOG("SetValue version failed with %i", error);
	}

	error = healtable_ini.SetBoolValue("settings", "debug_mode", pOptions.DebugMode);
	if (error < 0)
	{
		LOG("SetValue debug_mode failed with %i", error);
	}

	error = healtable_ini.SetLongValue("settings", "log_level", pOptions.LogLevel);
	if (error < 0)
	{
		LOG("SetValue log_level failed with %i", error);
	}

	error = healtable_ini.SetValue("settings", "evtc_rpc_endpoint", pOptions.EvtcRpcEndpoint);
	if (error < 0)
	{
		LOG("SetValue evtc_rpc_endpoint failed with %i", error);
	}

	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char section[128];
		snprintf(section, sizeof(section), "heal_window_%u", i);

		error = healtable_ini.SetBoolValue(section, "show_window", pOptions.Windows[i].Shown);
		if (error < 0)
		{
			LOG("SetValue show_window failed with %i", error);
		}

		error = healtable_ini.SetLongValue(section, "hotkey", pOptions.Windows[i].Hotkey);
		if (error < 0)
		{
			LOG("SetValue hotkey failed with %i", error);
		}

		error = healtable_ini.SetLongValue(section, "data_source_choice", pOptions.Windows[i].DataSourceChoice);
		if (error < 0)
		{
			LOG("SetValue data_source_choice failed with %i", error);
		}

		error = healtable_ini.SetLongValue(section, "sort_order_choice", pOptions.Windows[i].SortOrderChoice);
		if (error < 0)
		{
			LOG("SetValue sort_order_choice failed with %i", error);
		}

		error = healtable_ini.SetLongValue(section, "combat_end_condition_choice", pOptions.Windows[i].CombatEndConditionChoice);
		if (error < 0)
		{
			LOG("SetValue combat_end_condition_choice failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "exclude_group", pOptions.Windows[i].ExcludeGroup);
		if (error < 0)
		{
			LOG("SetValue exclude_group failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "exclude_off_group", pOptions.Windows[i].ExcludeOffGroup);
		if (error < 0)
		{
			LOG("SetValue exclude_off_group failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "exclude_off_squad", pOptions.Windows[i].ExcludeOffSquad);
		if (error < 0)
		{
			LOG("SetValue exclude_off_squad failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "exclude_minions", pOptions.Windows[i].ExcludeMinions);
		if (error < 0)
		{
			LOG("SetValue exclude_minions failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "exclude_unmapped", pOptions.Windows[i].ExcludeUnmapped);
		if (error < 0)
		{
			LOG("SetValue exclude_unmapped failed with %i", error);
		}

		error = healtable_ini.SetBoolValue(section, "show_progress_bars", pOptions.Windows[i].ShowProgressBars);
		if (error < 0)
		{
			LOG("SetValue show_progress_bars failed with %i", error);
		}

		error = healtable_ini.SetValue(section, "name", pOptions.Windows[i].Name);
		if (error < 0)
		{
			LOG("SetValue name failed with %i", error);
		}

		error = healtable_ini.SetValue(section, "title_format", pOptions.Windows[i].TitleFormat);
		if (error < 0)
		{
			LOG("SetValue title_format failed with %i", error);
		}

		error = healtable_ini.SetValue(section, "entry_format", pOptions.Windows[i].EntryFormat);
		if (error < 0)
		{
			LOG("SetValue entry_format failed with %i", error);
		}

		error = healtable_ini.SetValue(section, "details_entry_format", pOptions.Windows[i].DetailsEntryFormat);
		if (error < 0)
		{
			LOG("SetValue details_entry_format failed with %i", error);
		}
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

	uint32_t version = healtable_ini.GetLongValue("ini", "version");
	if (version <= 2)
	{
		healtable_ini.Reset(); // Remove everything from the old ini file
		return;
	}

	pOptions.DebugMode = healtable_ini.GetBoolValue("settings", "debug_mode", pOptions.DebugMode);
	pOptions.LogLevel = static_cast<spdlog::level::level_enum>(healtable_ini.GetLongValue("settings", "log_level", pOptions.LogLevel));

	const char* val;
	/*const char* val = healtable_ini.GetValue("settings", "evtc_rpc_endpoint", nullptr);
	if (val != nullptr)
	{
		snprintf(pOptions.EvtcRpcEndpoint, sizeof(pOptions.EvtcRpcEndpoint), "%s", val);
	}*/ // Force everyone to use default endpoint

	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char section[128];
		snprintf(section, sizeof(section), "heal_window_%u", i);

		pOptions.Windows[i].Shown = healtable_ini.GetBoolValue(section, "show_window", pOptions.Windows[i].Shown);
		pOptions.Windows[i].Hotkey = healtable_ini.GetLongValue(section, "hotkey", pOptions.Windows[i].Hotkey);

		pOptions.Windows[i].DataSourceChoice = healtable_ini.GetLongValue(section, "data_source_choice", pOptions.Windows[i].DataSourceChoice);
		pOptions.Windows[i].SortOrderChoice = healtable_ini.GetLongValue(section, "sort_order_choice", pOptions.Windows[i].SortOrderChoice);
		pOptions.Windows[i].CombatEndConditionChoice = healtable_ini.GetLongValue(section, "combat_end_condition_choice", pOptions.Windows[i].CombatEndConditionChoice);

		pOptions.Windows[i].ExcludeGroup = healtable_ini.GetBoolValue(section, "exclude_group", pOptions.Windows[i].ExcludeGroup);
		pOptions.Windows[i].ExcludeOffGroup = healtable_ini.GetBoolValue(section, "exclude_off_group", pOptions.Windows[i].ExcludeOffGroup);
		pOptions.Windows[i].ExcludeOffSquad = healtable_ini.GetBoolValue(section, "exclude_off_squad", pOptions.Windows[i].ExcludeOffSquad);
		pOptions.Windows[i].ExcludeMinions = healtable_ini.GetBoolValue(section, "exclude_minions", pOptions.Windows[i].ExcludeMinions);
		pOptions.Windows[i].ExcludeUnmapped = healtable_ini.GetBoolValue(section, "exclude_unmapped", pOptions.Windows[i].ExcludeUnmapped);

		pOptions.Windows[i].ShowProgressBars = healtable_ini.GetBoolValue(section, "show_progress_bars", pOptions.Windows[i].ShowProgressBars);

		val = healtable_ini.GetValue(section, "name", nullptr);
		if (val != nullptr)
		{
			snprintf(pOptions.Windows[i].Name, sizeof(pOptions.Windows[i].Name), "%s", val);
		}

		val = healtable_ini.GetValue(section, "title_format", nullptr);
		if (val != nullptr)
		{
			snprintf(pOptions.Windows[i].TitleFormat, sizeof(pOptions.Windows[i].TitleFormat), "%s", val);
		}

		val = healtable_ini.GetValue(section, "entry_format", nullptr);
		if (val != nullptr)
		{
			snprintf(pOptions.Windows[i].EntryFormat, sizeof(pOptions.Windows[i].EntryFormat), "%s", val);
		}

		val = healtable_ini.GetValue(section, "details_entry_format", nullptr);
		if (val != nullptr)
		{
			snprintf(pOptions.Windows[i].DetailsEntryFormat, sizeof(pOptions.Windows[i].DetailsEntryFormat), "%s", val);
		}

		LOG("Read window %u from ini file: show_window=%s data_source_choice=%i sort_order_choice=%i combat_end_condition_choice=%i, exclude_group=%s exclude_off_group=%s exclude_off_squad=%s exclude_minions=%s exclude_unmapped=%s show_progress_bars=%s, name='%s' title_format='%s' entry_format='%s' details_entry_format='%s'",
			i, BOOL_STR(pOptions.Windows[i].Shown), pOptions.Windows[i].DataSourceChoice, pOptions.Windows[i].SortOrderChoice, pOptions.Windows[i].CombatEndConditionChoice, BOOL_STR(pOptions.Windows[i].ExcludeGroup), BOOL_STR(pOptions.Windows[i].ExcludeOffGroup), BOOL_STR(pOptions.Windows[i].ExcludeOffSquad), BOOL_STR(pOptions.Windows[i].ExcludeMinions), BOOL_STR(pOptions.Windows[i].ExcludeUnmapped), BOOL_STR(pOptions.Windows[i].ShowProgressBars), pOptions.Windows[i].Name, pOptions.Windows[i].TitleFormat, pOptions.Windows[i].EntryFormat, pOptions.Windows[i].DetailsEntryFormat);
	}

	LOG("Read ini file debug_mode=%s", BOOL_STR(pOptions.DebugMode));
}

DetailsWindowState::DetailsWindowState(const AggregatedStatsEntry& pEntry)
	: AggregatedStatsEntry(pEntry)
{
}

HealTableOptions::HealTableOptions()
{
	Windows[0].DataSourceChoice = static_cast<int>(DataSource::Totals);
	snprintf(Windows[0].Name, sizeof(Windows[0].Name), "%s", "Totals");
	snprintf(Windows[0].TitleFormat, sizeof(Windows[0].TitleFormat), "%s", "Totals ({1}s in combat)");
	snprintf(Windows[0].EntryFormat, sizeof(Windows[0].EntryFormat), "%s", "{1} ({4}/s)");

	Windows[1].DataSourceChoice = static_cast<int>(DataSource::Agents);
	snprintf(Windows[1].Name, sizeof(Windows[1].Name), "%s", "Targets");
	snprintf(Windows[1].TitleFormat, sizeof(Windows[1].TitleFormat), "%s", "Targets {1} ({4}/s, {7}s in combat)");

	Windows[2].DataSourceChoice = static_cast<int>(DataSource::Skills);
	snprintf(Windows[2].Name, sizeof(Windows[2].Name), "%s", "Skills");
	snprintf(Windows[2].TitleFormat, sizeof(Windows[2].TitleFormat), "%s", "Skills {1} ({4}/s, {7}s in combat)");

	Windows[3].DataSourceChoice = static_cast<int>(DataSource::Agents);
	snprintf(Windows[3].Name, sizeof(Windows[3].Name), "%s", "Targets (hits)");
	snprintf(Windows[3].TitleFormat, sizeof(Windows[3].TitleFormat), "%s", "Targets {1} ({5}/hit, {2} hits)");
	snprintf(Windows[3].EntryFormat, sizeof(Windows[3].EntryFormat), "%s", "{1} ({5}/hit, {2} hits)");
	snprintf(Windows[3].DetailsEntryFormat, sizeof(Windows[3].DetailsEntryFormat), "%s", "{1} ({5}/hit, {2} hits)");

	Windows[4].DataSourceChoice = static_cast<int>(DataSource::Skills);
	snprintf(Windows[4].Name, sizeof(Windows[4].Name), "%s", "Skills (hits)");
	snprintf(Windows[4].TitleFormat, sizeof(Windows[4].TitleFormat), "%s", "Skills {1} ({5}/hit, {2} hits)");
	snprintf(Windows[4].EntryFormat, sizeof(Windows[4].EntryFormat), "%s", "{1} ({5}/hit, {2} hits)");
	snprintf(Windows[4].DetailsEntryFormat, sizeof(Windows[4].DetailsEntryFormat), "%s", "{1} ({5}/hit, {2} hits)");

	Windows[5].DataSourceChoice = static_cast<int>(DataSource::PeersOutgoing);
	snprintf(Windows[5].Name, sizeof(Windows[5].Name), "%s", "Peers outgoing");
	snprintf(Windows[5].TitleFormat, sizeof(Windows[5].TitleFormat), "%s", "Outgoing healing {1} ({4}/s, {7}s in combat)");

	Windows[9].DataSourceChoice = static_cast<int>(DataSource::Combined);
	snprintf(Windows[9].Name, sizeof(Windows[9].Name), "%s", "Combined");
	snprintf(Windows[9].TitleFormat, sizeof(Windows[9].TitleFormat), "%s", "Combined {1} ({4}/s, {7}s in combat)");
}
