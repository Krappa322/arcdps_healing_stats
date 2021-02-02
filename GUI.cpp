#include "GUI.h"

#include "AggregatedStats.h"
#include "Log.h"
#include "PersonalStats.h"
#include "Utilities.h"

#include "imgui/imgui.h"

#include <array>
#include <Windows.h>

static bool SHOW_HEAL_WINDOW = false;

static int SORT_ORDER_CHOICE = static_cast<int>(SortOrder::DescendingSize);
static int GROUP_FILTER_CHOICE = static_cast<int>(GroupFilter::All);
static bool EXCLUDE_UNMAPPED_AGENTS = true;

//static std::unique_ptr<AggregatedStats> exampleAggregatedStats;
static std::unique_ptr<AggregatedStats> currentAggregatedStats;
static time_t lastAggregatedTime = 0;

constexpr const char* GROUP_FILTER_STRING[] = {"Group", "Squad", "All (Excluding Minions)", "All (Including Minions)"};
static_assert((sizeof(GROUP_FILTER_STRING) / sizeof(GROUP_FILTER_STRING[0])) == static_cast<size_t>(GroupFilter::Max), "Added group filter option without updating gui?");
constexpr static uint32_t MAX_GROUP_FILTER_NAME = LongestStringInArray<GROUP_FILTER_STRING, static_cast<size_t>(GroupFilter::Max) - 1>::value;

void SetContext(void* pImGuiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)pImGuiContext);
/*
	std::map<uint32_t, HealingSkill> exampleMap;
	exampleMap[100].Name = "Regeneration";
	exampleMap[100].AgentsHealing[0].Name = "Zarwae";
	exampleMap[100].AgentsHealing[0].TotalHealing = 8000;
	exampleMap[100].AgentsHealing[0].Ticks = 86;
	exampleMap[100].AgentsHealing[1].Name = "Aethnis";
	exampleMap[100].AgentsHealing[1].TotalHealing = 1000;
	exampleMap[100].AgentsHealing[1].Ticks = 13;
	exampleMap[100].AgentsHealing[2].Name = "Verticalmike";
	exampleMap[100].AgentsHealing[2].TotalHealing = 4000;
	exampleMap[100].AgentsHealing[2].Ticks = 115;

	exampleMap[101].Name = "Rejuvenating Tides";
	exampleMap[101].AgentsHealing[0].Name = "Zarwae";
	exampleMap[101].AgentsHealing[0].TotalHealing = 7100;
	exampleMap[101].AgentsHealing[0].Ticks = 17;
	exampleMap[101].AgentsHealing[1].Name = "Aethnis";
	exampleMap[101].AgentsHealing[1].TotalHealing = 998;
	exampleMap[101].AgentsHealing[1].Ticks = 2;
	exampleMap[101].AgentsHealing[2].Name = "Verticalmike";
	exampleMap[101].AgentsHealing[2].TotalHealing = 4115;
	exampleMap[101].AgentsHealing[2].Ticks = 8;

	exampleMap[102].Name = "Glyph of Rejuvenation";
	exampleMap[102].AgentsHealing[0].Name = "Zarwae";
	exampleMap[102].AgentsHealing[0].TotalHealing = 10000;
	exampleMap[102].AgentsHealing[0].Ticks = 4;
	exampleMap[102].AgentsHealing[1].Name = "Aethnis";
	exampleMap[102].AgentsHealing[1].TotalHealing = 6031;
	exampleMap[102].AgentsHealing[1].Ticks = 2;
	exampleMap[102].AgentsHealing[3].Name = u8"Arrkó";
	exampleMap[102].AgentsHealing[3].TotalHealing = 9001;
	exampleMap[102].AgentsHealing[3].Ticks = 3;

	std::unique_ptr<std::map<uint32_t, HealingSkill>> ptr(new std::map<uint32_t, HealingSkill>(std::move(exampleMap)));
	exampleAggregatedStats = std::make_unique<AggregatedStats>(std::move(ptr), SortOrder::AscendingAlphabetical, true, true, true);
*/
}

void Display_GUI()
{
	time_t curTime = ::time(0);
	if (lastAggregatedTime < curTime)
	{
		//LOG("Fetching new aggregated stats");

		HealingStats stats = PersonalStats::GetGlobalState();
		currentAggregatedStats = std::make_unique<AggregatedStats>(std::move(stats), static_cast<SortOrder>(SORT_ORDER_CHOICE), static_cast<GroupFilter>(GROUP_FILTER_CHOICE), EXCLUDE_UNMAPPED_AGENTS);
		lastAggregatedTime = curTime;
	}

	if (SHOW_HEAL_WINDOW == true)
	{
		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Heal Table", &SHOW_HEAL_WINDOW);

		const char* const sortOrderItems[] = {"Alphabetical Ascending", "Alphabetical Descending", "Heal Per Second Ascending", "Heal Per Second Descending"};
		static_assert((sizeof(sortOrderItems) / sizeof(sortOrderItems[0])) == static_cast<uint64_t>(SortOrder::Max), "Added sort option without updating gui?");

		ImGui::Combo("Sort Order", &SORT_ORDER_CHOICE, sortOrderItems, static_cast<int>(SortOrder::Max));

		ImGui::Combo("Group Filter", &GROUP_FILTER_CHOICE, GROUP_FILTER_STRING, static_cast<int>(GroupFilter::Max));

		ImGui::Checkbox("Exclude unmapped agents", &EXCLUDE_UNMAPPED_AGENTS);

		if (ImGui::CollapsingHeader("Totals") == true)
		{
			TotalHealingStats stats = currentAggregatedStats->GetTotalHealing();
			for (size_t i = 0; i < stats.size(); i++)
			{
				ImGui::Text("%*s %.2f/s", MAX_GROUP_FILTER_NAME, GROUP_FILTER_STRING[i], stats[i]);
			}
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Agents") == true)
		{
			for (const auto& agent : currentAggregatedStats->GetAgents())
			{
				uint32_t longestName = currentAggregatedStats->GetLongestAgentName();

				uint32_t nameLength = utf8_strlen(agent.Name.c_str());
				assert(nameLength <= longestName);

				ImGui::Text("%*s%s %.2f/s", longestName - nameLength, "", agent.Name.c_str(), agent.PerSecond);
			}
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Skills") == true)
		{
			for (const auto& skill : currentAggregatedStats->GetSkills())
			{
				uint32_t longestName = currentAggregatedStats->GetLongestSkillName();

				uint32_t nameLength = utf8_strlen(skill.Name.c_str());
				assert(nameLength <= longestName);

				ImGui::Text("%*s%s %.2f/s", longestName - nameLength, "", skill.Name.c_str(), skill.PerSecond);
			}
			ImGui::Spacing();
		}

		ImGui::End();
	}
}

void Display_ArcDpsOptions()
{
	ImGui::Checkbox("Heal Table", &SHOW_HEAL_WINDOW);
}
