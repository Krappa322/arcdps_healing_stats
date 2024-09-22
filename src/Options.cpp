#include "Options.h"

#include "AddonVersion.h"
#include "AggregatedStats.h"
#include "Log.h"

#include "SimpleIni.h"

#include <filesystem>

using nlohmann::detail::value_t;

static CSimpleIniA healtable_ini(true /* isUtf8 */);

bool ReadIni(HealTableOptions& pOptions)
{
	SI_Error error = healtable_ini.LoadFile(LEGACY_INI_CONFIG_PATH);
	if (error < 0)
	{
		LOG("LoadFile failed with %i", error);
		return false;
	}

	uint32_t version = healtable_ini.GetLongValue("ini", "version");
	if (version <= 2)
	{
		healtable_ini.Reset(); // Remove everything from the old ini file
		return true;
	}

	pOptions.DebugMode = healtable_ini.GetBoolValue("settings", "debug_mode", pOptions.DebugMode);
	pOptions.LogLevel = static_cast<spdlog::level::level_enum>(healtable_ini.GetLongValue("settings", "log_level", pOptions.LogLevel));

	const char* val;
	/*const char* val = healtable_ini.GetValue("settings", "evtc_rpc_endpoint", nullptr);
	if (val != nullptr)
	{
		snprintf(pOptions.EvtcRpcEndpoint, sizeof(pOptions.EvtcRpcEndpoint), "%s", val);
	}*/ // Force everyone to use default endpoint
	pOptions.EvtcRpcEnabled = healtable_ini.GetBoolValue("settings", "evtc_rpc_enabled", pOptions.EvtcRpcEnabled);

	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char section[128];
		snprintf(section, sizeof(section), "heal_window_%u", i);

		pOptions.Windows[i].Shown = healtable_ini.GetBoolValue(section, "show_window", pOptions.Windows[i].Shown);
		pOptions.Windows[i].Hotkey = healtable_ini.GetLongValue(section, "hotkey", pOptions.Windows[i].Hotkey);

		pOptions.Windows[i].DataSourceChoice = static_cast<DataSource>(healtable_ini.GetLongValue(section, "data_source_choice", static_cast<long>(pOptions.Windows[i].DataSourceChoice)));
		pOptions.Windows[i].SortOrderChoice = static_cast<SortOrder>(healtable_ini.GetLongValue(section, "sort_order_choice", static_cast<long>(pOptions.Windows[i].SortOrderChoice)));
		pOptions.Windows[i].CombatEndConditionChoice = static_cast<CombatEndCondition>(healtable_ini.GetLongValue(section, "combat_end_condition_choice", static_cast<long>(pOptions.Windows[i].CombatEndConditionChoice)));

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

		pOptions.Windows[i].WindowFlags = static_cast<ImGuiWindowFlags_>(healtable_ini.GetLongValue(section, "window_flags", pOptions.Windows[i].WindowFlags));

		LOG("Read window %u from ini file: show_window=%s data_source_choice=%i sort_order_choice=%i combat_end_condition_choice=%i, exclude_group=%s exclude_off_group=%s exclude_off_squad=%s exclude_minions=%s exclude_unmapped=%s show_progress_bars=%s, name='%s' title_format='%s' entry_format='%s' details_entry_format='%s'",
			i, BOOL_STR(pOptions.Windows[i].Shown), pOptions.Windows[i].DataSourceChoice, pOptions.Windows[i].SortOrderChoice, pOptions.Windows[i].CombatEndConditionChoice, BOOL_STR(pOptions.Windows[i].ExcludeGroup), BOOL_STR(pOptions.Windows[i].ExcludeOffGroup), BOOL_STR(pOptions.Windows[i].ExcludeOffSquad), BOOL_STR(pOptions.Windows[i].ExcludeMinions), BOOL_STR(pOptions.Windows[i].ExcludeUnmapped), BOOL_STR(pOptions.Windows[i].ShowProgressBars), pOptions.Windows[i].Name, pOptions.Windows[i].TitleFormat, pOptions.Windows[i].EntryFormat, pOptions.Windows[i].DetailsEntryFormat);
	}

	LOG("Read ini file debug_mode=%s", BOOL_STR(pOptions.DebugMode));
	return true;
}

DetailsWindowState::DetailsWindowState(const AggregatedStatsEntry& pEntry)
	: AggregatedStatsEntry(pEntry)
{
}

HealTableOptions::HealTableOptions()
{
	Windows[0].DataSourceChoice = DataSource::Totals;
	snprintf(Windows[0].Name, sizeof(Windows[0].Name), "%s", "Totals");
	snprintf(Windows[0].TitleFormat, sizeof(Windows[0].TitleFormat), "%s", "Totals ({1}s in combat)");
	snprintf(Windows[0].EntryFormat, sizeof(Windows[0].EntryFormat), "%s", "{1} ({4}/s)");

	Windows[1].DataSourceChoice = DataSource::Agents;
	snprintf(Windows[1].Name, sizeof(Windows[1].Name), "%s", "Targets");
	snprintf(Windows[1].TitleFormat, sizeof(Windows[1].TitleFormat), "%s", "Targets {1} ({4}/s, {7}s in combat)");

	Windows[2].DataSourceChoice = DataSource::Skills;
	snprintf(Windows[2].Name, sizeof(Windows[2].Name), "%s", "Skills");
	snprintf(Windows[2].TitleFormat, sizeof(Windows[2].TitleFormat), "%s", "Skills {1} ({4}/s, {7}s in combat)");

	Windows[3].DataSourceChoice = DataSource::Agents;
	snprintf(Windows[3].Name, sizeof(Windows[3].Name), "%s", "Targets (hits)");
	snprintf(Windows[3].TitleFormat, sizeof(Windows[3].TitleFormat), "%s", "Targets {1} ({5}/hit, {2} hits)");
	snprintf(Windows[3].EntryFormat, sizeof(Windows[3].EntryFormat), "%s", "{1} ({5}/hit, {2} hits)");
	snprintf(Windows[3].DetailsEntryFormat, sizeof(Windows[3].DetailsEntryFormat), "%s", "{1} ({5}/hit, {2} hits)");

	Windows[4].DataSourceChoice = DataSource::Skills;
	snprintf(Windows[4].Name, sizeof(Windows[4].Name), "%s", "Skills (hits)");
	snprintf(Windows[4].TitleFormat, sizeof(Windows[4].TitleFormat), "%s", "Skills {1} ({5}/hit, {2} hits)");
	snprintf(Windows[4].EntryFormat, sizeof(Windows[4].EntryFormat), "%s", "{1} ({5}/hit, {2} hits)");
	snprintf(Windows[4].DetailsEntryFormat, sizeof(Windows[4].DetailsEntryFormat), "%s", "{1} ({5}/hit, {2} hits)");

	Windows[5].DataSourceChoice = DataSource::PeersOutgoing;
	snprintf(Windows[5].Name, sizeof(Windows[5].Name), "%s", "Peers outgoing");
	snprintf(Windows[5].TitleFormat, sizeof(Windows[5].TitleFormat), "%s", "Outgoing healing {1} ({4}/s, {7}s in combat)");

	Windows[9].DataSourceChoice = DataSource::Combined;
	snprintf(Windows[9].Name, sizeof(Windows[9].Name), "%s", "Combined");
	snprintf(Windows[9].TitleFormat, sizeof(Windows[9].TitleFormat), "%s", "Combined {1} ({4}/s, {7}s in combat)");
}

void HealTableOptions::Load(const char* pConfigPath)
{
	std::string buffer;
	buffer.reserve(128 * 1024);

	FILE* file = fopen(pConfigPath, "r");
	if (file == nullptr)
	{
		LogW("Opening '{}' failed - errno={} GetLastError={}. Attempting to read ini", pConfigPath, errno, GetLastError());

		bool readIni = ReadIni(*this);
		if (readIni == true)
		{
			LogI("Successfully read from legacy ini. Saving json");
			bool savedJson = Save(pConfigPath);
			if (savedJson == true)
			{
				LogI("Saving json succesful. Deleting ini");
				std::error_code ec;
				bool deleted = std::filesystem::remove(LEGACY_INI_CONFIG_PATH, ec);
				LogI("Deleting ini result={} ec.value={} ec.message={} ec.category.name={}",
					BOOL_STR(deleted), ec.value(), ec.message(), ec.category().name());
			}
		}

		return;
	}

	size_t read = fread(buffer.data(), sizeof(char), 128 * 1024 - 1, file);
	if (read == 128 * 1024 - 1)
	{
		LogW("Reading '{}' failed - insufficient buffer", pConfigPath);
		return;
	}
	else if (ferror(file) != 0)
	{
		LogW("Reading '{}' failed - errno={} GetLastError={}", pConfigPath, errno, GetLastError());
		return;
	}

	if (fclose(file) == EOF)
	{
		LogW("Closing '{}' failed - errno={} GetLastError={}", pConfigPath, errno, GetLastError());
		return;
	}

	buffer.data()[read] = '\0';
	LogT("Parsing {}", buffer.data());

	try
	{
		nlohmann::json jsonObject = nlohmann::json::parse(buffer.data());
		FromJson(jsonObject);
	}
	catch (std::exception& e)
	{
		LogW("Parsing settings failed - {}", e.what());
	}
}

bool HealTableOptions::Save(const char* pConfigPath) const
{
	nlohmann::json jsonObject;
	ToJson(jsonObject);

	std::string serialized = jsonObject.dump();
	FILE* file = fopen(pConfigPath, "w");
	if (file == nullptr)
	{
		LogW("Opening '{}' failed - errno={} GetLastError={}", pConfigPath, errno, GetLastError());
		return false;
	}

	size_t written = fwrite(serialized.c_str(), sizeof(char), serialized.size(), file);
	if (written != serialized.size())
	{
		LogW("Writing to '{}' failed - written={} errno={} GetLastError={}", pConfigPath, written, errno, GetLastError());
		return false;
	}

	if (fclose(file) == EOF)
	{
		LogW("Closing '{}' failed - errno={} GetLastError={}", pConfigPath, errno, GetLastError());
		return false;
	}

	LogD("Saved to {}", pConfigPath);
	return true;
}

constexpr static value_t NUMBER_TYPE = static_cast<value_t>(250);

template<typename T>
constexpr static value_t JsonTypeSelector()
{
	if constexpr (std::is_same_v<T, bool>)
	{
		return value_t::boolean;
	}
	else if constexpr (std::is_signed_v<T>)
	{
		return NUMBER_TYPE;
	}
	else if constexpr (std::is_enum_v<T>)
	{
		using RealT = std::underlying_type_t<T>;
		if constexpr (std::is_signed_v<RealT>)
		{
			return NUMBER_TYPE;
		}
		else if constexpr (std::is_unsigned_v<RealT>)
		{
			return value_t::number_unsigned;
		}
		else
		{
			assert(false);
			//static_assert(false);
		}
	}
	else if constexpr (std::is_unsigned_v<T>)
	{
		return value_t::number_unsigned;
	}
	else
	{
		assert(false);
		//static_assert(false);
	}
}

template <typename ResultType>
void GetJsonValue(const nlohmann::json& pJsonObject, const char* pFieldName, ResultType& pResult)
{
	constexpr value_t jsonType = JsonTypeSelector<ResultType>();

	const auto iter = pJsonObject.find(pFieldName);
	if (iter != pJsonObject.end())
	{
		if ((jsonType == NUMBER_TYPE && iter->is_number_integer() == false) ||
			(jsonType != NUMBER_TYPE && iter->type() != jsonType))
		{
			LogW("Found '{}' but it has the wrong type {} (expected {})", pFieldName, static_cast<uint8_t>(iter->type()), static_cast<uint8_t>(jsonType));
		}
		else
		{
			iter->get_to(pResult);
		}
	}
}

template <size_t StringSize>
void GetJsonValue(const nlohmann::json& pJsonObject, const char* pFieldName, char(&pResult)[StringSize])
{
	const auto iter = pJsonObject.find(pFieldName);
	if (iter != pJsonObject.end())
	{
		if (iter->is_string())
		{
			std::string temp;
			iter->get_to(temp);
			snprintf(pResult, StringSize, "%s", temp.c_str());
		}
		else
		{
			LogW("(String) Found '{}' but it has the wrong type {}", pFieldName, static_cast<uint8_t>(iter->type()));
		}
	}
}

void HealTableOptions::FromJson(const nlohmann::json& pJsonObject)
{
	uint32_t version = UINT32_MAX;
	GetJsonValue(pJsonObject, "Version", version);
	if (version != 1)
	{
		LogW("Invalid config version {}", version);
	}

	GetJsonValue(pJsonObject, "AutoUpdateSetting", AutoUpdateSetting);
	GetJsonValue(pJsonObject, "DebugMode", DebugMode);
	GetJsonValue(pJsonObject, "LogLevel", LogLevel);
	GetJsonValue(pJsonObject, "EvtcLoggingEnabled", EvtcLoggingEnabled);
	GetJsonValue(pJsonObject, "EvtcRpcEndpoint", EvtcRpcEndpoint);
	GetJsonValue(pJsonObject, "EvtcRpcEnabled", EvtcRpcEnabled);
	GetJsonValue(pJsonObject, "EvtcRpcBudgetMode", EvtcRpcBudgetMode);
	GetJsonValue(pJsonObject, "EvtcRpcDisableEncryption", EvtcRpcDisableEncryption);
	GetJsonValue(pJsonObject, "EvtcRpcEnabledHotkey", EvtcRpcEnabledHotkey);
	GetJsonValue(pJsonObject, "IncludeBarrier", IncludeBarrier);

	const auto iter = pJsonObject.find("Windows");
	if (iter != pJsonObject.end())
	{
		if (iter->is_object())
		{
			for (size_t i = 0; i < Windows.size(); i++)
			{
				std::string i_str = std::to_string(i);
				const auto iter2 = iter->find(i_str);
				if (iter2 != iter->end())
				{
					Windows[i].FromJson(iter2.value());
				}
				else
				{
					LogT("Didn't find '{}' in json", i_str);
				}
			}
		}
		else
		{
			LogW("Found 'Windows' but it's not an array (type {})", static_cast<uint8_t>(iter->type()));
		}
	}

	LogI("Parsed {}", *this);
}

void HealTableOptions::ToJson(nlohmann::json& pJsonObject) const
{
#define SET_JSON_VAL(pKey)\
do {\
	if (pKey != defaults.pKey)\
	{\
		pJsonObject[#pKey] = pKey;\
	}\
} while(false)
#define SET_JSON_VAL_CSTR_ARRAY(pKey)\
do {\
	if (strcmp(pKey, defaults.pKey) != 0)\
	{\
		pJsonObject[#pKey] = std::string_view{pKey};\
	}\
} while(false)

	HealTableOptions defaults;

	pJsonObject["Version"] = 1U;

	SET_JSON_VAL(AutoUpdateSetting);
	SET_JSON_VAL(DebugMode);
	SET_JSON_VAL(LogLevel);
	SET_JSON_VAL(EvtcLoggingEnabled);
	SET_JSON_VAL_CSTR_ARRAY(EvtcRpcEndpoint);
	SET_JSON_VAL(EvtcRpcEnabled);
	SET_JSON_VAL(EvtcRpcBudgetMode);
	SET_JSON_VAL(EvtcRpcDisableEncryption);
	SET_JSON_VAL(EvtcRpcEnabledHotkey);
	SET_JSON_VAL(IncludeBarrier);

	nlohmann::json windows;
	for (size_t i = 0; i < Windows.size(); i++)
	{
		nlohmann::json window;
		Windows[i].ToJson(window, defaults.Windows[i]);

		if (window.is_null() == false)
		{
			std::string i_str = std::to_string(i);
			windows[i_str] = window;
		}
	}

	if (windows.is_null() == false)
	{
		pJsonObject["Windows"] = windows;
	}
#undef SET_JSON_VAL
#undef SET_JSON_VAL_CSTR_ARRAY
}

void HealTableOptions::Reset()
{
	// avoids having to maintain operator= for all sub-objects
	this->~HealTableOptions();
	new(this) HealTableOptions;
}

void HealWindowOptions::FromJson(const nlohmann::json& pJsonObject)
{
	GetJsonValue(pJsonObject, "Shown", Shown);

	GetJsonValue(pJsonObject, "DataSourceChoice", DataSourceChoice);
	GetJsonValue(pJsonObject, "SortOrderChoice", SortOrderChoice);
	GetJsonValue(pJsonObject, "CombatEndConditionChoice", CombatEndConditionChoice);

	GetJsonValue(pJsonObject, "ExcludeGroup", ExcludeGroup);
	GetJsonValue(pJsonObject, "ExcludeOffGroup", ExcludeOffGroup);
	GetJsonValue(pJsonObject, "ExcludeOffSquad", ExcludeOffSquad);
	GetJsonValue(pJsonObject, "ExcludeMinions", ExcludeMinions);
	GetJsonValue(pJsonObject, "ExcludeUnmapped", ExcludeUnmapped);

	GetJsonValue(pJsonObject, "ShowProgressBars", ShowProgressBars);
	GetJsonValue(pJsonObject, "Name", Name);
	GetJsonValue(pJsonObject, "TitleFormat", TitleFormat);
	GetJsonValue(pJsonObject, "EntryFormat", EntryFormat);
	GetJsonValue(pJsonObject, "DetailsEntryFormat", DetailsEntryFormat);

	GetJsonValue(pJsonObject, "WindowFlags", WindowFlags);
	GetJsonValue(pJsonObject, "Hotkey", Hotkey);

	GetJsonValue(pJsonObject, "PositionRule", PositionRule);
	GetJsonValue(pJsonObject, "RelativeScreenCorner", RelativeScreenCorner);
	GetJsonValue(pJsonObject, "RelativeSelfCorner", RelativeSelfCorner);
	GetJsonValue(pJsonObject, "RelativeAnchorWindowCorner", RelativeAnchorWindowCorner);
	GetJsonValue(pJsonObject, "RelativeX", RelativeX);
	GetJsonValue(pJsonObject, "RelativeY", RelativeY);
	GetJsonValue(pJsonObject, "AnchorWindowId", AnchorWindowId);

	GetJsonValue(pJsonObject, "AutoResize", AutoResize);
	GetJsonValue(pJsonObject, "MaxNameLength", MaxNameLength);
	GetJsonValue(pJsonObject, "MinLinesDisplayed", MinLinesDisplayed);
	GetJsonValue(pJsonObject, "MaxLinesDisplayed", MaxLinesDisplayed);
	GetJsonValue(pJsonObject, "FixedWindowWidth", FixedWindowWidth);
}

void HealWindowOptions::ToJson(nlohmann::json& pJsonObject, const HealWindowOptions& pDefault) const
{
#define SET_JSON_VAL(pKey)\
do {\
	if (pKey != pDefault.pKey)\
	{\
		pJsonObject[#pKey] = pKey;\
	}\
} while(false)
#define SET_JSON_VAL_CSTR_ARRAY(pKey)\
do {\
	if (strcmp(pKey, pDefault.pKey) != 0)\
	{\
		pJsonObject[#pKey] = std::string_view{pKey};\
	}\
} while(false)

	SET_JSON_VAL(Shown);

	SET_JSON_VAL(DataSourceChoice);
	SET_JSON_VAL(SortOrderChoice);
	SET_JSON_VAL(CombatEndConditionChoice);

	SET_JSON_VAL(ExcludeGroup);
	SET_JSON_VAL(ExcludeOffGroup);
	SET_JSON_VAL(ExcludeOffSquad);
	SET_JSON_VAL(ExcludeMinions);
	SET_JSON_VAL(ExcludeUnmapped);

	SET_JSON_VAL(ShowProgressBars);
	SET_JSON_VAL_CSTR_ARRAY(Name);
	SET_JSON_VAL_CSTR_ARRAY(TitleFormat);
	SET_JSON_VAL_CSTR_ARRAY(EntryFormat);
	SET_JSON_VAL_CSTR_ARRAY(DetailsEntryFormat);

	SET_JSON_VAL(WindowFlags);
	SET_JSON_VAL(Hotkey);

	SET_JSON_VAL(PositionRule);
	SET_JSON_VAL(RelativeScreenCorner);
	SET_JSON_VAL(RelativeSelfCorner);
	SET_JSON_VAL(RelativeAnchorWindowCorner);
	SET_JSON_VAL(RelativeX);
	SET_JSON_VAL(RelativeY);
	SET_JSON_VAL(AnchorWindowId);

	SET_JSON_VAL(AutoResize);
	SET_JSON_VAL(MaxNameLength);
	SET_JSON_VAL(MinLinesDisplayed);
	SET_JSON_VAL(MaxLinesDisplayed);
	SET_JSON_VAL(FixedWindowWidth);
#undef SET_JSON_VAL
#undef SET_JSON_VAL_CSTR_ARRAY
}