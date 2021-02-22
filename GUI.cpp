#include "GUI.h"

#include "AggregatedStats.h"
#include "ImGuiEx.h"
#include "Log.h"
#include "PersonalStats.h"
#include "Utilities.h"

#include <array>
#include <Windows.h>

constexpr const char* GROUP_FILTER_STRING[] = { "Group", "Squad", "All (Excluding Summons)", "All (Including Summons)" };
static_assert((sizeof(GROUP_FILTER_STRING) / sizeof(GROUP_FILTER_STRING[0])) == static_cast<size_t>(GroupFilter::Max), "Added group filter option without updating gui?");
constexpr static uint32_t MAX_GROUP_FILTER_NAME = LongestStringInArray<GROUP_FILTER_STRING, static_cast<size_t>(GroupFilter::Max) - 1>::value;

struct DetailsWindowState
{
	std::string CachedName;
	float CachedPerSecond = 0;
	bool IsOpen = false;
};

static std::map<uint32_t, DetailsWindowState> OPEN_SKILL_WINDOWS;
static std::map<uintptr_t, DetailsWindowState> OPEN_AGENT_WINDOWS;

//static std::unique_ptr<AggregatedStats> exampleAggregatedStats;
static std::unique_ptr<AggregatedStats> currentAggregatedStats;
static time_t lastAggregatedTime = 0;

enum UniqueIdSpace
{
	UniqueIdSpace_Targets = 1,
	UniqueIdSpace_Skills = 2
};

static void Display_SkillDetailsWindow(uint32_t pSkillId, const std::string& pSkillName, float pHealPerSecond, bool* pIsOpen)
{
	if (*pIsOpen == false)
	{
		return;
	}

	char buffer[1024];
	// Using "###" means the id of the window is calculated only from the part after the hashes (which
	// in turn means that the name of the window can change if necessary)
	snprintf(buffer, sizeof(buffer), "%s###HEALSKILL%u", pSkillName.c_str(), pSkillId);
	ImGui::SetNextWindowSize(ImVec2(600, 360), ImGuiCond_FirstUseEver);
	ImGui::Begin(buffer, pIsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	snprintf(buffer, sizeof(buffer), "##HEALSKILL.TOTALS.%u", pSkillId);

	ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	bgColor.w = 0.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
	ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.35, 0));
	ImGui::Text("total healing");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("healing per second");
	ImGuiEx::TextRightAlignedSameLine("%.2f/s", pHealPerSecond);

	ImGui::Text("healing per hit");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("healing per cast");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("hits");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("casts");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGuiEx::BottomText("id %u", pSkillId);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALSKILL.AGENTS.%u", pSkillId);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));
	for (const auto& agent : currentAggregatedStats->GetSkillDetails(pSkillId))
	{
		ImGui::Text("%s", agent.Name.c_str());
		ImGuiEx::TextRightAlignedSameLine("%.2f/s", agent.PerSecond);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	ImGui::End();
}

static void Display_AgentDetailsWindow(uintptr_t pAgentId, const std::string& pAgentName, float pHealPerSecond, bool* pIsOpen)
{
	if (*pIsOpen == false)
	{
		return;
	}

	char buffer[1024];
	// Using "###" means the id of the window is calculated only from the part after the hashes (which
	// in turn means that the name of the window can change if necessary)
	snprintf(buffer, sizeof(buffer), "%s###HEALGENT%llu", pAgentName.c_str(), pAgentId);
	ImGui::SetNextWindowSize(ImVec2(600, 360), ImGuiCond_FirstUseEver);
	ImGui::Begin(buffer, pIsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	snprintf(buffer, sizeof(buffer), "##HEALAGENT.TOTALS.%llu", pAgentId);

	ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	bgColor.w = 0.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
	ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.35, 0));

	ImGui::Text("total healing");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("healing per second");
	ImGuiEx::TextRightAlignedSameLine("%.2f/s", pHealPerSecond);

	ImGui::Text("healing per hit");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGui::Text("hits");
	ImGuiEx::TextRightAlignedSameLine("soon (tm)");

	ImGuiEx::BottomText("id %llu", pAgentId);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALSKILL.AGENTS.%llu", pAgentId);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));
	for (const auto& skill : currentAggregatedStats->GetAgentDetails(pAgentId))
	{
		ImGui::Text("%s", skill.Name.c_str());
		ImGuiEx::TextRightAlignedSameLine("%.2f/s", skill.PerSecond);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	ImGui::End();
}

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
		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
		ImGui::Begin("Heal Table##HEAL", &pHealingOptions.ShowHealWindow, ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginPopupContextItem("Options##HEAL") == true || ImGui::BeginPopupContextWindow("Options##HEAL") == true)
		{
			const char* const sortOrderItems[] = { "Alphabetical Ascending", "Alphabetical Descending", "Heal Per Second Ascending", "Heal Per Second Descending" };
			static_assert((sizeof(sortOrderItems) / sizeof(sortOrderItems[0])) == static_cast<uint64_t>(SortOrder::Max), "Added sort option without updating gui?");

			ImGui::Checkbox("Show Totals", &pHealingOptions.ShowTotals);
			ImGuiEx::AddTooltipToLastItem("Toggles if the 'Totals' section should be displayed.");

			ImGui::Checkbox("Show Targets", &pHealingOptions.ShowAgents);
			ImGuiEx::AddTooltipToLastItem("Toggles if the 'Targets' section should be displayed.");

			ImGui::Checkbox("Show Skills", &pHealingOptions.ShowSkills);
			ImGuiEx::AddTooltipToLastItem("Toggles if the 'Skills' section should be displayed.");

			ImGui::Combo("Sort Order", &pHealingOptions.SortOrderChoice, sortOrderItems, static_cast<int>(SortOrder::Max));
			ImGuiEx::AddTooltipToLastItem("Decides how targets and skills are sorted in the 'Targets' and 'Skills' sections.");

			ImGui::Combo("Target Filter", &pHealingOptions.GroupFilterChoice, GROUP_FILTER_STRING, static_cast<int>(GroupFilter::Max));
			ImGuiEx::AddTooltipToLastItem(
				"Filters out targets based on their subgroup and if they are part of the squad.\n\n"
				"- '%s': Targets in the same subgroup as yourself\n"
				"- '%s': Targets in your squad\n"
				"- '%s': All healing, not counting summons (summons in this\n"
				"        case means all player spawned targets such as pets, spirits, clones, etc.)\n"
				"- '%s': All healing"
				, GROUP_FILTER_STRING[0], GROUP_FILTER_STRING[1], GROUP_FILTER_STRING[2], GROUP_FILTER_STRING[3]);

			const char* hotkeyTooltip = "Numerical value (virtual key code) for the key\n"
										"used to open and close the healing table";

			float oldPosY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
			ImGui::Text("Hotkey");
			ImGuiEx::AddTooltipToLastItem(hotkeyTooltip);

			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::SetCursorPosY(oldPosY);
			ImGui::InputInt("##HOTKEY", &pHealingOptions.HealTableHotkey, 0);
			ImGuiEx::AddTooltipToLastItem(hotkeyTooltip);

			ImGui::SameLine();
			ImGui::Text("(%s)", VirtualKeyToString(pHealingOptions.HealTableHotkey).c_str());
			ImGuiEx::AddTooltipToLastItem(hotkeyTooltip);

			ImGui::Checkbox("Exclude unmapped targets", &pHealingOptions.ExcludeUnmappedAgents);
			ImGuiEx::AddTooltipToLastItem(
				"Filters out targets which could not be mapped to a name. The filtering\n"
				"applies to the 'Totals' section as well as 'Targets' and 'Skills'.\n"
				"If not filtered, these targets will appear with a number instead of a name\n"
				"in the 'Targets' section");

			ImGui::Checkbox("Debug Mode", &pHealingOptions.DebugMode);
			ImGuiEx::AddTooltipToLastItem(
				"Includes debug data in target and skill names.\n"
				"Turn this on before taking screenshots of potential calculation issues.");

			ImGui::EndPopup();
		}

		if (pHealingOptions.ShowTotals == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize("Totals").x * 0.5f);
			ImGui::TextColored(ImColor(0, 209, 165), "Totals");

			TotalHealingStats stats = currentAggregatedStats->GetTotalHealing();
			for (size_t i = 0; i < stats.size(); i++)
			{
				ImGui::Text("%s", GROUP_FILTER_STRING[i]);

				snprintf(buffer, sizeof(buffer), "%.2f/s", stats[i]);
				ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buffer).x);
				ImGui::Text("%s", buffer);
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowAgents == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize("Targets").x * 0.5f);
			ImGui::TextColored(ImColor(0, 209, 165), "Targets");

			for (const auto& agent : currentAggregatedStats->GetAgents())
			{
				ImGui::BeginGroup();
				ImGui::PushID(UniqueIdSpace_Targets);
				ImGui::PushID(agent.Id);
				float startX = ImGui::GetCursorPosX();
				ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::PopID();
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::SetCursorPosX(startX);
				ImGui::Text("%s", agent.Name.c_str());

				snprintf(buffer, sizeof(buffer), "%.2f/s", agent.PerSecond);
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buffer).x); // Setting x in SameLine would add group offset which makes the calculation incorrect
				ImGui::Text("%s", buffer);

				ImGui::EndGroup();

				auto [state, _inserted] = OPEN_AGENT_WINDOWS.emplace(std::piecewise_construct, std::forward_as_tuple(agent.Id), std::forward_as_tuple());
				state->second.CachedName = agent.Name;
				state->second.CachedPerSecond = agent.PerSecond;

				if (ImGui::IsItemClicked() == true)
				{
					state->second.IsOpen = !state->second.IsOpen;

					LOG("Toggled details window for agent %llu %s", agent.Id, agent.Name.c_str());
				}
			}
			ImGui::Spacing();
		}

		if (pHealingOptions.ShowSkills == true)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize("Skills").x * 0.5f);
			ImGui::TextColored(ImColor(0, 209, 165), "Skills");

			for (const auto& skill : currentAggregatedStats->GetSkills())
			{
				/*const ImVec2 p = ImGui::GetCursorScreenPos();
				drawList->AddRect(
					ImVec2(p.x, p.y),
					ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y + ImGui::GetTextLineHeight()),
					IM_COL32(0, 255, 0, 128));*/

				ImGui::BeginGroup();
				ImGui::PushID(UniqueIdSpace_Skills);
				ImGui::PushID(skill.Id);
				float startX = ImGui::GetCursorPosX();
				ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::PopID();
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::SetCursorPosX(startX);
				ImGui::Text("%s", skill.Name.c_str());

				snprintf(buffer, sizeof(buffer), "%.2f/s", skill.PerSecond);
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buffer).x); // Setting x in SameLine would add group offset which makes the calculation incorrect
				ImGui::Text("%s", buffer);

				ImGui::EndGroup();

				auto [state, _inserted] = OPEN_SKILL_WINDOWS.emplace(std::piecewise_construct, std::forward_as_tuple(skill.Id), std::forward_as_tuple());
				state->second.CachedName = skill.Name;
				state->second.CachedPerSecond = skill.PerSecond;

				if (ImGui::IsItemClicked() == true && skill.Id != IndirectHealingSkillId)
				{
					state->second.IsOpen = !state->second.IsOpen;

					LOG("Toggled details window for skill %u %s", skill.Id, skill.Name.c_str());
				}
			}
			ImGui::Spacing();
		}

		for (auto& [skillId, windowState] : OPEN_SKILL_WINDOWS)
		{
			Display_SkillDetailsWindow(skillId, windowState.CachedName, windowState.CachedPerSecond, &windowState.IsOpen);
		}

		for (auto& [agentId, windowState] : OPEN_AGENT_WINDOWS)
		{
			Display_AgentDetailsWindow(agentId, windowState.CachedName, windowState.CachedPerSecond, &windowState.IsOpen);
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
