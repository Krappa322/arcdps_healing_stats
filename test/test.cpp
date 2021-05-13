#include <gtest/gtest.h>

#include "AggregatedStats.h"
#include "arcdps_mock/CombatMock.h"
#include "Exports.h"
#include "Options.h"

#include <utility>

extern "C" __declspec(dllexport) void e3(const char* pString);
extern "C" __declspec(dllexport) uint64_t e6();
extern "C" __declspec(dllexport) uint64_t e7();

namespace
{
#pragma pack(push, 1)
	struct ArcModifiers
	{
		uint16_t _1 = VK_SHIFT;
		uint16_t _2 = VK_MENU;
		uint16_t Multi = 0;
		uint16_t Fill = 0;
	};
#pragma pack(pop)
}

void e3(const char* pString)
{
	return; // Logging, ignored
}

uint64_t e6()
{
	return 0; // everything set to false
}

uint64_t e7()
{
	ArcModifiers mods;
	return *reinterpret_cast<uint64_t*>(&mods);
}

// parameters are <max parallel callbacks, max fuzz width>
class XevtcLogTestFixture : public ::testing::TestWithParam<std::pair<uint32_t, uint32_t>>
{
protected:
	arcdps_exports Exports;
	CombatMock Mock{&Exports};

	void SetUp() override
	{
		//PlayerStats::GlobalState.Clear(); // Make sure agents etc. aren't leaked between test runs

		ModInitSignature mod_init = get_init_addr("unit_test", nullptr, nullptr, GetModuleHandle(NULL), malloc, free);

		{
			arcdps_exports* temp_exports = mod_init();
			memcpy(&Exports, temp_exports, sizeof(Exports)); // Maybe do some deep copy at some point but we're not using the strings in there anyways
		}
	}

	HealingStats GetLocalState()
	{
		auto [localId, states] = GlobalObjects::EVENT_PROCESSOR->GetState();
		auto iter = states.find(localId);
		assert(iter != states.end());
		return iter->second.second;
	}
};

TEST_P(XevtcLogTestFixture, druid_solo)
{
	auto [parallelCallbacks, fuzzWidth] = GetParam();

	uint32_t result = Mock.ExecuteFromXevtc("logs\\druid_solo.xevtc", parallelCallbacks, fuzzWidth);
	ASSERT_EQ(result, 0);
	ASSERT_TRUE(GlobalObjects::EVENT_SEQUENCER->QueueIsEmpty());

	HealWindowOptions options; // Use all defaults
	HealingStats rawStats = GetLocalState();
	AggregatedStats stats{std::move(rawStats), options, false};
	
	EXPECT_FLOAT_EQ(stats.GetCombatTime(), 47.0f);
	
	const AggregatedStatsEntry& totalEntry = stats.GetTotal();
	EXPECT_EQ(totalEntry.Healing, 121095);
	EXPECT_EQ(totalEntry.Hits, 204);

	const AggregatedVector& agentStats = stats.GetStats(DataSource::Agents);
	ASSERT_EQ(agentStats.Entries.size(), 1);
	EXPECT_EQ(agentStats.Entries[0].GetTie(),
		AggregatedStatsEntry(2000, "Zarwae", 121095, 204, std::nullopt).GetTie());

	AggregatedVector expectedSkills;
	expectedSkills.Add(31796, "Cosmic Ray", 25140, 30, std::nullopt);
	expectedSkills.Add(31894, "Rejuvenating Tides", 15362, 20, std::nullopt);
	expectedSkills.Add(718, "Regeneration", 14016, 48, std::nullopt);
	expectedSkills.Add(21775, "Aqua Surge (Self)", 12954, 3, std::nullopt);
	expectedSkills.Add(31318, "Lunar Impact", 12090, 4, std::nullopt);
	expectedSkills.Add(31535, "Ancestral Grace", 10976, 4, std::nullopt);
	expectedSkills.Add(29863, "Vigorous Recovery", 8432, 31, std::nullopt);
	expectedSkills.Add(12567, "Nature's Renewal Aura", 6862, 47, std::nullopt);
	expectedSkills.Add(21776, "Aqua Surge (Area)", 6597, 3, std::nullopt);
	expectedSkills.Add(12836, "Water Blast Combo", 4737, 3, std::nullopt);
	expectedSkills.Add(13980, "Windborne Notes", 2350, 10, std::nullopt);
	expectedSkills.Add(12825, "Water Blast Combo", 1579, 1, std::nullopt);

	const AggregatedVector& skillStats = stats.GetStats(DataSource::Skills);
	ASSERT_EQ(skillStats.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(skillStats.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(skillStats.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	const AggregatedVector& agentDetails = stats.GetDetails(DataSource::Agents, 2000);
	ASSERT_EQ(agentDetails.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(agentDetails.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(agentDetails.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		const AggregatedVector& skillDetails = stats.GetDetails(DataSource::Skills, expectedSkills.Entries[i].Id);
		AggregatedStatsEntry expected{2000, "Zarwae", expectedSkills.Entries[i].Healing, expectedSkills.Entries[i].Hits, std::nullopt};
		ASSERT_EQ(skillDetails.Entries.size(), 1);
		EXPECT_EQ(skillDetails.HighestHealing, expectedSkills.Entries[i].Healing);
		EXPECT_EQ(skillDetails.Entries[0].GetTie(), expected.GetTie());
	}
}

TEST_P(XevtcLogTestFixture, druid_MO)
{
	auto [parallelCallbacks, fuzzWidth] = GetParam();

	uint32_t result = Mock.ExecuteFromXevtc("logs\\druid_MO.xevtc", parallelCallbacks, fuzzWidth);
	ASSERT_EQ(result, 0);
	ASSERT_TRUE(GlobalObjects::EVENT_SEQUENCER->QueueIsEmpty());

	HealTableOptions options;

	// Use the "Combined" window
	HealingStats rawStats = GetLocalState();
	AggregatedStats stats{std::move(rawStats), options.Windows[9], false};

	EXPECT_FLOAT_EQ(stats.GetCombatTime(), 95.0f);

	const AggregatedStatsEntry& totalEntry = stats.GetTotal();
	EXPECT_EQ(totalEntry.Healing, 304967);
	EXPECT_EQ(totalEntry.Hits, 727);

	AggregatedVector expectedTotals;
	expectedTotals.Add(0, "Group", 207634, 449, std::nullopt);
	expectedTotals.Add(0, "Squad", 304967, 727, std::nullopt);
	expectedTotals.Add(0, "All (Excluding Summons)", 304967, 727, std::nullopt);
	expectedTotals.Add(0, "All (Including Summons)", 409220, 1186, std::nullopt);

	const AggregatedVector& totals = stats.GetStats(DataSource::Totals);
	ASSERT_EQ(totals.Entries.size(), expectedTotals.Entries.size());
	EXPECT_EQ(totals.HighestHealing, expectedTotals.HighestHealing);
	for (uint32_t i = 0; i < expectedTotals.Entries.size(); i++)
	{
		EXPECT_EQ(totals.Entries[i].GetTie(), expectedTotals.Entries[i].GetTie());
	}

	AggregatedVector expectedAgents;
	expectedAgents.Add(2000, "Zarwae", 51011, 135, std::nullopt);
	expectedAgents.Add(3148, "Apocalypse Dawn", 47929, 89, std::nullopt);
	expectedAgents.Add(3150, "Waiana Sulis", 40005, 86, std::nullopt);
	expectedAgents.Add(3149, "And Avr Two L Q E A", 39603, 71, std::nullopt);
	expectedAgents.Add(3144, "Taya Celeste", 29086, 68, std::nullopt);
	expectedAgents.Add(3145, "Teivarus", 26490, 71, std::nullopt);
	expectedAgents.Add(3151, "Janna Larion", 21902, 71, std::nullopt);
	expectedAgents.Add(3137, "Lady Manyak", 20637, 52, std::nullopt);
	expectedAgents.Add(3146, "Akashi Vi Britannia", 20084, 55, std::nullopt);
	expectedAgents.Add(3147, u8"Moa Fhómhair", 8220, 29, std::nullopt);

	const AggregatedVector& agents = stats.GetStats(DataSource::Agents);
	ASSERT_EQ(agents.Entries.size(), expectedAgents.Entries.size());
	EXPECT_EQ(agents.HighestHealing, expectedAgents.HighestHealing);
	for (uint32_t i = 0; i < expectedAgents.Entries.size(); i++)
	{
		EXPECT_EQ(agents.Entries[i].GetTie(), expectedAgents.Entries[i].GetTie());
	}
}

// This is the same as druid_solo above except druid name is replaced by null everywhere
TEST_P(XevtcLogTestFixture, null_names)
{
	auto [parallelCallbacks, fuzzWidth] = GetParam();

	uint32_t result = Mock.ExecuteFromXevtc("logs\\null_names.xevtc", parallelCallbacks, fuzzWidth);
	ASSERT_EQ(result, 0);
	ASSERT_TRUE(GlobalObjects::EVENT_SEQUENCER->QueueIsEmpty());

	HealWindowOptions options;
	options.ExcludeOffSquad = false;
	options.ExcludeUnmapped = false;

	HealingStats rawStats = GetLocalState();
	AggregatedStats stats{ std::move(rawStats), options, false };

	EXPECT_FLOAT_EQ(stats.GetCombatTime(), 47.0f);

	const AggregatedStatsEntry& totalEntry = stats.GetTotal();
	EXPECT_EQ(totalEntry.Healing, 121095);
	EXPECT_EQ(totalEntry.Hits, 204);

	const AggregatedVector& agentStats = stats.GetStats(DataSource::Agents);
	ASSERT_EQ(agentStats.Entries.size(), 1);
	EXPECT_EQ(agentStats.Entries[0].GetTie(),
		AggregatedStatsEntry(2000, "2000", 121095, 204, std::nullopt).GetTie());

	AggregatedVector expectedSkills;
	expectedSkills.Add(31796, "Cosmic Ray", 25140, 30, std::nullopt);
	expectedSkills.Add(31894, "Rejuvenating Tides", 15362, 20, std::nullopt);
	expectedSkills.Add(718, "Regeneration", 14016, 48, std::nullopt);
	expectedSkills.Add(21775, "Aqua Surge (Self)", 12954, 3, std::nullopt);
	expectedSkills.Add(31318, "Lunar Impact", 12090, 4, std::nullopt);
	expectedSkills.Add(31535, "Ancestral Grace", 10976, 4, std::nullopt);
	expectedSkills.Add(29863, "Vigorous Recovery", 8432, 31, std::nullopt);
	expectedSkills.Add(12567, "Nature's Renewal Aura", 6862, 47, std::nullopt);
	expectedSkills.Add(21776, "Aqua Surge (Area)", 6597, 3, std::nullopt);
	expectedSkills.Add(12836, "Water Blast Combo", 4737, 3, std::nullopt);
	expectedSkills.Add(13980, "Windborne Notes", 2350, 10, std::nullopt);
	expectedSkills.Add(12825, "Water Blast Combo", 1579, 1, std::nullopt);

	const AggregatedVector& skillStats = stats.GetStats(DataSource::Skills);
	ASSERT_EQ(skillStats.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(skillStats.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(skillStats.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	const AggregatedVector& agentDetails = stats.GetDetails(DataSource::Agents, 2000);
	ASSERT_EQ(agentDetails.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(agentDetails.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(agentDetails.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		const AggregatedVector& skillDetails = stats.GetDetails(DataSource::Skills, expectedSkills.Entries[i].Id);
		AggregatedStatsEntry expected{ 2000, "2000", expectedSkills.Entries[i].Healing, expectedSkills.Entries[i].Hits, std::nullopt };
		ASSERT_EQ(skillDetails.Entries.size(), 1);
		EXPECT_EQ(skillDetails.HighestHealing, expectedSkills.Entries[i].Healing);
		EXPECT_EQ(skillDetails.Entries[0].GetTie(), expected.GetTie());
	}
}

INSTANTIATE_TEST_CASE_P(
	Normal,
	XevtcLogTestFixture,
	::testing::Values(std::make_pair(0, 0)));

INSTANTIATE_TEST_CASE_P(
	Fuzz,
	XevtcLogTestFixture,
	::testing::Values(std::make_pair(0, 16), std::make_pair(0, 64)));

INSTANTIATE_TEST_CASE_P(
	MultiThreaded,
	XevtcLogTestFixture,
	::testing::Values(std::make_pair(16, 0), std::make_pair(16, 16), std::make_pair(128, 16)));
