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
	char buffer[1024];

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
		ImGui::Begin("Heal Table", &pHealingOptions.ShowHealWindow, ImGuiWindowFlags_NoScrollbar);

		if (ImGui::BeginPopupContextItem("Options") == true || ImGui::BeginPopupContextWindow("Options") == true)
		{
			const char* const sortOrderItems[] = { "Alphabetical Ascending", "Alphabetical Descending", "Heal Per Second Ascending", "Heal Per Second Descending" };
			static_assert((sizeof(sortOrderItems) / sizeof(sortOrderItems[0])) == static_cast<uint64_t>(SortOrder::Max), "Added sort option without updating gui?");

			ImGui::Checkbox("Show Totals", &pHealingOptions.ShowTotals);

			ImGui::Checkbox("Show Agents", &pHealingOptions.ShowAgents);

			ImGui::Checkbox("Show Skills", &pHealingOptions.ShowSkills);

			ImGui::Combo("Sort Order", &pHealingOptions.SortOrderChoice, sortOrderItems, static_cast<int>(SortOrder::Max));

			ImGui::Combo("Group Filter", &pHealingOptions.GroupFilterChoice, GROUP_FILTER_STRING, static_cast<int>(GroupFilter::Max));

			float oldPosY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
			ImGui::Text("Hotkey");
			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::SetCursorPosY(oldPosY);
			ImGui::InputInt("", &pHealingOptions.HealTableHotkey, 0);
			ImGui::SameLine();
			ImGui::Text("(%s)", VirtualKeyToString(pHealingOptions.HealTableHotkey).c_str());

			ImGui::Checkbox("Exclude unmapped agents", &pHealingOptions.ExcludeUnmappedAgents);

			ImGui::Checkbox("Debug Mode", &pHealingOptions.DebugMode);

			ImGui::EndPopup();
		}

		if (pHealingOptions.ShowTotals == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Totals");

			TotalHealingStats stats = currentAggregatedStats->GetTotalHealing();
			for (size_t i = 0; i < stats.size(); i++)
			{
				ImGui::Text("%s", GROUP_FILTER_STRING[i]);

				snprintf(buffer, sizeof(buffer), "%.2f/s", stats[i]);
				ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer).x - ImGui::GetStyle().ItemSpacing.x * 2);
				ImGui::Text("%s", buffer);
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowAgents == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Agents");

			for (const auto& agent : currentAggregatedStats->GetAgents())
			{
				ImGui::Text("%s", agent.Name.c_str());

				snprintf(buffer, sizeof(buffer), "%.2f/s", agent.PerSecond);
				ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer).x - ImGui::GetStyle().ItemSpacing.x * 2);
				ImGui::Text("%s", buffer);
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowSkills == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f - ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextColored(ImColor(0, 209, 165), "Skills");

			for (const auto& skill : currentAggregatedStats->GetSkills())
			{
				ImGui::Text("%s", skill.Name.c_str());

				snprintf(buffer, sizeof(buffer), "%.2f/s", skill.PerSecond);
				ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer).x - ImGui::GetStyle().ItemSpacing.x * 2);
				ImGui::Text("%s", buffer);
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

void ImGui_ProcessKeyEvent(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL)
{
	ImGuiIO& io = ImGui::GetIO();

	switch (pMessage)
	{
	case WM_KEYUP:
	case WM_SYSKEYUP: // WM_SYSKEYUP is called when a key is pressed with the ALT held down
		io.KeysDown[static_cast<int>(pAdditionalW)] = false;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN: // WM_SYSKEYDOWN is called when a key is pressed with the ALT held down
		io.KeysDown[static_cast<int>(pAdditionalW)] = true;
		break;

	default:
		break;
	}
}