#pragma warning(push, 0)
#pragma warning(disable : 4005)
#pragma warning(disable : 4389)
#pragma warning(disable : 26439)
#pragma warning(disable : 26495)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "Options.h"

namespace
{
template<typename T>
T rand_t()
{
	uint8_t components[sizeof(T)];
	for (size_t i = 0; i < std::size(components); i++)
	{
		components[i] = static_cast<uint8_t>(rand());
	}

	T result;
	memcpy(&result, &components, sizeof(result));
	return result;
}

template<size_t Size>
void rand_string(char(&pStringBuffer)[Size])
{
	uint64_t length = rand_t<uint64_t>() % Size;

	for (size_t i = 0; i < length; i++)
	{
		do
		{
			pStringBuffer[i] = rand_t<char>();
		} while (pStringBuffer[i] < 1 || isprint(pStringBuffer[i]) == 0);
	}
	pStringBuffer[length] = '\0';
}
}; // anonymous namespace

TEST(ConfigTest, Serialize_Deserialize)
{
	uint64_t raw_seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	uint32_t seed = (reinterpret_cast<uint32_t*>(&raw_seed)[0] ^ reinterpret_cast<uint32_t*>(&raw_seed)[1]);
	LogD("Seed {} (raw_seed={})", seed, raw_seed);
	srand(seed);

	HealTableOptions options;
	options.DebugMode = rand_t<bool>();
	options.LogLevel = rand_t<spdlog::level::level_enum>();
	options.EvtcLoggingEnabled = rand_t<bool>();
	rand_string(options.EvtcRpcEndpoint);
	options.EvtcRpcEnabled = rand_t<bool>();

	for (HealWindowContext& window : options.Windows)
	{
		window.Shown = rand_t<bool>();

		window.DataSourceChoice = rand_t<DataSource>();
		window.SortOrderChoice = rand_t<SortOrder>();
		window.CombatEndConditionChoice = rand_t<CombatEndCondition>();

		window.ExcludeGroup = rand_t<bool>();
		window.ExcludeOffGroup = rand_t<bool>();
		window.ExcludeOffSquad = rand_t<bool>();
		window.ExcludeMinions = rand_t<bool>();
		window.ExcludeUnmapped = rand_t<bool>();
		window.ExcludeHealing = rand_t<bool>();
		window.ExcludeBarrierGeneration = rand_t<bool>();

		window.ShowProgressBars = rand_t<bool>();
		window.UseSubgroupForBarColour = rand_t<bool>();
		window.IndexNumbers = rand_t<bool>();
		window.ProfessionText = rand_t<bool>();
		window.ProfessionIcons = rand_t<bool>();
		window.ReplacePlayerWithAccountName = rand_t<bool>();
		rand_string(window.Name);
		rand_string(window.TitleFormat);
		rand_string(window.EntryFormat);
		rand_string(window.DetailsEntryFormat);

		window.WindowFlags = rand_t<ImGuiWindowFlags_>();

		window.Hotkey = rand_t<int>();

		window.PositionRule = rand_t<Position>();
		window.RelativeScreenCorner = rand_t<CornerPosition>();
		window.RelativeSelfCorner = rand_t<CornerPosition>();
		window.RelativeAnchorWindowCorner = rand_t<CornerPosition>();
		window.RelativeX = rand_t<int64_t>();
		window.RelativeY = rand_t<int64_t>();
		window.AnchorWindowId = rand_t<ImGuiID>();

		window.AutoResize = rand_t<bool>();
		window.MinLinesDisplayed = rand_t<size_t>();
		window.MaxLinesDisplayed = rand_t<size_t>();
		window.FixedWindowWidth = rand_t<size_t>();
	}

	HealTableOptions options2;

	nlohmann::json temp;
	options.ToJson(temp);

	LogI("Json: {}", temp.dump());

	options2.FromJson(temp);

	ASSERT_EQ(options.DebugMode, options2.DebugMode);
	ASSERT_EQ(options.LogLevel, options2.LogLevel);
	ASSERT_EQ(options.EvtcLoggingEnabled, options2.EvtcLoggingEnabled);
	ASSERT_EQ(strcmp(options.EvtcRpcEndpoint, options2.EvtcRpcEndpoint), 0);
	ASSERT_EQ(options.EvtcRpcEnabled, options2.EvtcRpcEnabled);

	for (size_t i = 0; i < options2.Windows.size(); i++)
	{
		const HealWindowContext& windowLeft = options.Windows[i];
		const HealWindowContext& windowRight = options2.Windows[i];

		ASSERT_EQ(windowLeft.Shown, windowRight.Shown);

		ASSERT_EQ(windowLeft.DataSourceChoice, windowRight.DataSourceChoice);
		ASSERT_EQ(windowLeft.SortOrderChoice, windowRight.SortOrderChoice);
		ASSERT_EQ(windowLeft.CombatEndConditionChoice, windowRight.CombatEndConditionChoice);

		ASSERT_EQ(windowLeft.ExcludeGroup, windowRight.ExcludeGroup);
		ASSERT_EQ(windowLeft.ExcludeOffGroup, windowRight.ExcludeOffGroup);
		ASSERT_EQ(windowLeft.ExcludeOffSquad, windowRight.ExcludeOffSquad);
		ASSERT_EQ(windowLeft.ExcludeMinions, windowRight.ExcludeMinions);
		ASSERT_EQ(windowLeft.ExcludeUnmapped, windowRight.ExcludeUnmapped);
		ASSERT_EQ(windowLeft.ExcludeHealing, windowRight.ExcludeHealing);
		ASSERT_EQ(windowLeft.ExcludeBarrierGeneration, windowRight.ExcludeBarrierGeneration);

		ASSERT_EQ(windowLeft.ShowProgressBars, windowRight.ShowProgressBars);
		ASSERT_EQ(windowLeft.UseSubgroupForBarColour, windowRight.UseSubgroupForBarColour);
		ASSERT_EQ(windowLeft.IndexNumbers, windowRight.IndexNumbers);
		ASSERT_EQ(windowLeft.ProfessionText, windowRight.ProfessionText);
		ASSERT_EQ(windowLeft.ProfessionIcons, windowRight.ProfessionIcons);
		ASSERT_EQ(windowLeft.ReplacePlayerWithAccountName, windowRight.ReplacePlayerWithAccountName);
		ASSERT_EQ(strcmp(windowLeft.Name, windowRight.Name), 0);
		ASSERT_EQ(strcmp(windowLeft.TitleFormat, windowRight.TitleFormat), 0);
		ASSERT_EQ(strcmp(windowLeft.EntryFormat, windowRight.EntryFormat), 0);
		ASSERT_EQ(strcmp(windowLeft.DetailsEntryFormat, windowRight.DetailsEntryFormat), 0);

		ASSERT_EQ(windowLeft.WindowFlags, windowRight.WindowFlags);

		ASSERT_EQ(windowLeft.Hotkey, windowRight.Hotkey);

		ASSERT_EQ(windowLeft.PositionRule, windowRight.PositionRule);
		ASSERT_EQ(windowLeft.RelativeScreenCorner, windowRight.RelativeScreenCorner);
		ASSERT_EQ(windowLeft.RelativeSelfCorner, windowRight.RelativeSelfCorner);
		ASSERT_EQ(windowLeft.RelativeAnchorWindowCorner, windowRight.RelativeAnchorWindowCorner);
		ASSERT_EQ(windowLeft.RelativeX, windowRight.RelativeX);
		ASSERT_EQ(windowLeft.RelativeY, windowRight.RelativeY);
		ASSERT_EQ(windowLeft.AnchorWindowId, windowRight.AnchorWindowId);

		ASSERT_EQ(windowLeft.AutoResize, windowRight.AutoResize);
		ASSERT_EQ(windowLeft.MinLinesDisplayed, windowRight.MinLinesDisplayed);
		ASSERT_EQ(windowLeft.MaxLinesDisplayed, windowRight.MaxLinesDisplayed);
		ASSERT_EQ(windowLeft.FixedWindowWidth, windowRight.FixedWindowWidth);
	}
}

TEST(ConfigTest, Defaults)
{
	HealTableOptions options;

	nlohmann::json temp;
	options.ToJson(temp);

	ASSERT_EQ(temp.dump(), "{\"Version\":1}");
}
