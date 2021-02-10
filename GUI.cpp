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
		currentAggregatedStats = std::make_unique<AggregatedStats>(std::move(stats), pHealingOptions);
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

			ImGui::Checkbox("Show Totals", &pHealingOptions.ShowTotals);

			ImGui::Checkbox("Show Agents", &pHealingOptions.ShowAgents);

			ImGui::Checkbox("Show Skills", &pHealingOptions.ShowSkills);

			ImGui::Spacing();

			ImGui::Combo("Sort Order", &pHealingOptions.SortOrderChoice, sortOrderItems, static_cast<int>(SortOrder::Max));

			ImGui::Combo("Group Filter", &pHealingOptions.GroupFilterChoice, GROUP_FILTER_STRING, static_cast<int>(GroupFilter::Max));

			ImGui::Checkbox("Exclude unmapped agents", &pHealingOptions.ExcludeUnmappedAgents);

			ImGui::Spacing();

			ImGui::Checkbox("Debug Mode", &pHealingOptions.DebugMode);

			ImGui::EndPopup();
		}

		uint32_t longestName = 0;
		if (pHealingOptions.ShowTotals == true && MAX_GROUP_FILTER_NAME > longestName)
		{
			longestName = MAX_GROUP_FILTER_NAME;
		}
		if (pHealingOptions.ShowAgents == true && currentAggregatedStats->GetLongestAgentName() > longestName)
		{
			longestName = currentAggregatedStats->GetLongestAgentName();
		}
		if (pHealingOptions.ShowSkills == true && currentAggregatedStats->GetLongestSkillName() > longestName)
		{
			longestName = currentAggregatedStats->GetLongestSkillName();
		}

		if (pHealingOptions.ShowTotals == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Totals");

			TotalHealingStats stats = currentAggregatedStats->GetTotalHealing();
			for (size_t i = 0; i < stats.size(); i++)
			{
				uint32_t nameLength = utf8_strlen(GROUP_FILTER_STRING[i]);
				assert(nameLength <= longestName);

				ImGui::Text("%*s%s %.2f/s", longestName - nameLength, "", GROUP_FILTER_STRING[i], stats[i]);
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowAgents == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Agents");
			for (const auto& agent : currentAggregatedStats->GetAgents())
			{
				uint32_t nameLength = utf8_strlen(agent.Name.c_str());
				assert(nameLength <= longestName);

				ImGui::Text("%*s%s %.2f/s", longestName - nameLength, "", agent.Name.c_str(), agent.PerSecond);
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowSkills == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Skills");
			for (const auto& skill : currentAggregatedStats->GetSkills())
			{
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
