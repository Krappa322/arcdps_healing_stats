#include "GUI.h"

#include "AggregatedStats.h"
#include "Log.h"
#include "PersonalStats.h"
#include "Utilities.h"

#include "imgui/imgui.h"

#include <array>
#include <Windows.h>

//static std::unique_ptr<AggregatedStats> exampleAggregatedStats;
static std::unique_ptr<AggregatedStats> currentAggregatedStats;
static time_t lastAggregatedTime = 0;

constexpr const char* GROUP_FILTER_STRING[] = {"Group", "Squad", "All (Excluding Minions)", "All (Including Minions)"};
static_assert((sizeof(GROUP_FILTER_STRING) / sizeof(GROUP_FILTER_STRING[0])) == static_cast<size_t>(GroupFilter::Max), "Added group filter option without updating gui?");
constexpr static uint32_t MAX_GROUP_FILTER_NAME = LongestStringInArray<GROUP_FILTER_STRING, static_cast<size_t>(GroupFilter::Max) - 1>::value;

void SetContext(void* pImGuiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)pImGuiContext);
}

void Display_GUI(HealTableOptions& pHealingOptions)
{
	time_t curTime = ::time(0);
	if (lastAggregatedTime < curTime)
	{
		//LOG("Fetching new aggregated stats");

		HealingStats stats = PersonalStats::GetGlobalState();
		currentAggregatedStats = std::make_unique<AggregatedStats>(std::move(stats), static_cast<SortOrder>(pHealingOptions.SortOrderChoice), static_cast<GroupFilter>(pHealingOptions.GroupFilterChoice), pHealingOptions.ExcludeUnmappedAgents);
		lastAggregatedTime = curTime;
	}

	if (pHealingOptions.ShowHealWindow == true)
	{
		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
		ImGui::Begin("Heal Table", &pHealingOptions.ShowHealWindow);

		if (ImGui::BeginPopupContextItem("Options") == true || ImGui::BeginPopupContextWindow("Options") == true)
		{
			const char* const sortOrderItems[] = { "Alphabetical Ascending", "Alphabetical Descending", "Heal Per Second Ascending", "Heal Per Second Descending" };
			static_assert((sizeof(sortOrderItems) / sizeof(sortOrderItems[0])) == static_cast<uint64_t>(SortOrder::Max), "Added sort option without updating gui?");

			ImGui::Combo("Sort Order", &pHealingOptions.SortOrderChoice, sortOrderItems, static_cast<int>(SortOrder::Max));

			ImGui::Combo("Group Filter", &pHealingOptions.GroupFilterChoice, GROUP_FILTER_STRING, static_cast<int>(GroupFilter::Max));

			ImGui::Checkbox("Exclude unmapped agents", &pHealingOptions.ExcludeUnmappedAgents);

			ImGui::EndPopup();
		}

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

void Display_ArcDpsOptions(HealTableOptions& pHealingOptions)
{
	ImGui::Checkbox("Heal Table", &pHealingOptions.ShowHealWindow);
}
