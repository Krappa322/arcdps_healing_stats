#include <gtest/gtest.h>

#include "AggregatedStats.h"
#include "arcdps_mock/CombatMock.h"
#include "Exports.h"

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

TEST(all, druid_solo)
{
	ModInitSignature mod_init = get_init_addr("unit_test", nullptr, nullptr, GetModuleHandle(NULL), malloc, free);

	arcdps_exports exports;
	{
		arcdps_exports* temp_exports = mod_init();
		memcpy(&exports, temp_exports, sizeof(exports)); // Maybe do some deep copy at some point but we're not using the strings in there anyways
	}

	CombatMock mock{&exports};
	uint32_t result = mock.ExecuteFromXevtc("logs\\druid_solo.xevtc");
	ASSERT_EQ(result, 0);

	HealWindowOptions options; // Use all defaults
	HealingStats rawStats = PersonalStats::GetGlobalState();
	AggregatedStats stats{std::move(rawStats), options, false};
	
	EXPECT_FLOAT_EQ(stats.GetCombatTime(), 47.0f);
	
	const AggregatedStatsEntry& totalEntry = stats.GetTotal();
	EXPECT_EQ(totalEntry.Healing, 114951);
	EXPECT_EQ(totalEntry.Hits, 194);

	const AggregatedVector& agentStats = stats.GetStats(DataSource::Agents);
	ASSERT_EQ(agentStats.Entries.size(), 1);
	EXPECT_EQ(agentStats.Entries[0].GetTie(),
		AggregatedStatsEntry(2000, "Zarwae", 114951, 194, std::nullopt).GetTie());

	AggregatedVector expectedSkills;
	expectedSkills.Add(31796, "Cosmic Ray", 25140, 30, std::nullopt);
	expectedSkills.Add(31894, "Rejuvenating Tides", 15362, 20, std::nullopt);
	expectedSkills.Add(718, "Regeneration", 13140, 45, std::nullopt);
	expectedSkills.Add(21775, "Aqua Surge (Self)", 12954, 3, std::nullopt);
	expectedSkills.Add(31318, "Lunar Impact", 12090, 4, std::nullopt);
	expectedSkills.Add(31535, "Ancestral Grace", 8232, 3, std::nullopt);
	expectedSkills.Add(29863, "Vigorous Recovery", 8160, 30, std::nullopt);
	expectedSkills.Add(21776, "Aqua Surge (Area)", 6597, 3, std::nullopt);
	expectedSkills.Add(12567, "Nature's Renewal Aura", 6424, 44, std::nullopt);
	expectedSkills.Add(12836, "Water Blast Combo", 3158, 2, std::nullopt);
	expectedSkills.Add(13980, "Windborne Notes", 2115, 9, std::nullopt);
	expectedSkills.Add(12825, "Water Blast Combo", 1579, 1, std::nullopt);

	const AggregatedVector& skillStats = stats.GetStats(DataSource::Skills);
	EXPECT_EQ(skillStats.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(skillStats.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(skillStats.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	const AggregatedVector& agentDetails = stats.GetDetails(DataSource::Agents, 2000);
	EXPECT_EQ(agentDetails.Entries.size(), expectedSkills.Entries.size());
	EXPECT_EQ(agentDetails.HighestHealing, expectedSkills.HighestHealing);
	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		EXPECT_EQ(agentDetails.Entries[i].GetTie(), expectedSkills.Entries[i].GetTie());
	}

	for (uint32_t i = 0; i < expectedSkills.Entries.size(); i++)
	{
		const AggregatedVector& skillDetails = stats.GetDetails(DataSource::Skills, expectedSkills.Entries[i].Id);
		AggregatedStatsEntry expected{2000, "Zarwae", expectedSkills.Entries[i].Healing, expectedSkills.Entries[i].Hits, std::nullopt};
		EXPECT_EQ(skillDetails.Entries.size(), 1);
		EXPECT_EQ(skillDetails.HighestHealing, expectedSkills.Entries[i].Healing);
		EXPECT_EQ(skillDetails.Entries[0].GetTie(), expected.GetTie());
	}
}