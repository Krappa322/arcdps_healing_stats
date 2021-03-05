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

static void Display_DetailsWindow(HealWindowContext& pContext, DetailsWindowState& pState)
{
	if (pState.IsOpen == false)
	{
		return;
	}

	uint64_t timeInCombat = pContext.CurrentAggregatedStats->GetCombatTime();
	const AggregatedStatsEntry& aggregatedTotal = pContext.CurrentAggregatedStats->GetTotal();

	char buffer[1024];
	// Using "###" means the id of the window is calculated only from the part after the hashes (which
	// in turn means that the name of the window can change if necessary)
	snprintf(buffer, sizeof(buffer), "%s###HEALDETAILS%llu", pState.Name.c_str(), pState.Id);
	ImGui::SetNextWindowSize(ImVec2(600, 360), ImGuiCond_FirstUseEver);
	ImGui::Begin(buffer, &pState.IsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
	{
		ImGui::InputText("entry format", pContext.DetailsEntryFormat, sizeof(pContext.DetailsEntryFormat));
		ImGuiEx::AddTooltipToLastItem("Format for displayed data (statistics are per entry).\n"
		                              "{1}: Healing\n"
		                              "{2}: Hits\n"
		                              "{3}: Casts (not implemented yet)\n"
		                              "{4}: Healing per second\n"
		                              "{5}: Healing per hit\n"
		                              "{6}: Healing per cast (not implemented yet)\n"
		                              "{7}: Percent of total healing\n");

		ImGui::EndPopup();
	}

	ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	bgColor.w = 0.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.TOTALS.%llu", pState.Id);
	ImGui::BeginChild(buffer, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.35f, 0));
	ImGui::Text("total healing");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Healing);

	ImGui::Text("hits");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Hits);

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text("casts");
		ImGuiEx::TextRightAlignedSameLine("%llu", *pState.Casts);
	}

	ImGui::Text("healing per second");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, timeInCombat));

	ImGui::Text("healing per hit");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.Hits));

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text("healing per cast");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, *pState.Casts));
	}

	ImGuiEx::BottomText("id %u", pState.Id);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.ENTRIES.%llu", pState.Id);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));
	for (const auto& entry : pContext.CurrentAggregatedStats->GetDetails(pState.Id))
	{
		ImGui::Text("%s", entry.Name.c_str());

		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(204, 204, 212, 255));
		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
			entry.Hits,
			entry.Casts,
			divide_safe(entry.Healing, timeInCombat),
			divide_safe(entry.Healing, entry.Hits),
			entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
			divide_safe(entry.Healing * 100, aggregatedTotal.Healing)};
		ReplaceFormatted(buffer, sizeof(buffer), pContext.DetailsEntryFormat, entryValues);
		ImGuiEx::TextRightAlignedSameLine("%s", buffer);
		ImGui::PopStyleColor();
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

	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		HealWindowContext& curWindow = pHealingOptions.Windows[i];

		if (curWindow.Shown == false)
		{
			continue;
		}

		time_t curTime = ::time(0);
		if (curWindow.LastAggregatedTime < curTime)
		{
			//LOG("Fetching new aggregated stats");

			HealingStats stats = PersonalStats::GetGlobalState();
			curWindow.CurrentAggregatedStats = std::make_unique<AggregatedStats>(std::move(stats), curWindow, pHealingOptions.DebugMode);
			curWindow.LastAggregatedTime = curTime;
		}

		uint64_t timeInCombat = curWindow.CurrentAggregatedStats->GetCombatTime();

		const AggregatedStatsEntry& aggregatedTotal = curWindow.CurrentAggregatedStats->GetTotal();
		std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
			aggregatedTotal.Healing,
			aggregatedTotal.Hits,
			aggregatedTotal.Casts,
			divide_safe(aggregatedTotal.Healing, timeInCombat),
			divide_safe(aggregatedTotal.Healing, aggregatedTotal.Hits),
			aggregatedTotal.Casts.has_value() == true ? std::optional{divide_safe(aggregatedTotal.Healing, *aggregatedTotal.Casts)} : std::nullopt,
			timeInCombat};
		size_t written = ReplaceFormatted(buffer, 128, curWindow.TitleFormat, titleValues);
		snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);

		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
		ImGui::Begin(buffer, &curWindow.Shown, ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
		{
			const char* const dataSourceItems[] = {"targets", "skills", "totals"};
			static_assert((sizeof(dataSourceItems) / sizeof(dataSourceItems[0])) == static_cast<uint64_t>(DataSource::Max), "Added data source without updating gui?");
			ImGui::Combo("data source", &curWindow.DataSourceChoice, dataSourceItems, static_cast<int>(DataSource::Max));
			ImGuiEx::AddTooltipToLastItem("Decides how targets and skills are sorted in the 'Targets' and 'Skills' sections.");

			const char* const sortOrderItems[] = {"alphabetical ascending", "alphabetical descending", "heal per second ascending", "heal per second descending"};
			static_assert((sizeof(sortOrderItems) / sizeof(sortOrderItems[0])) == static_cast<uint64_t>(SortOrder::Max), "Added sort option without updating gui?");
			ImGui::Combo("sort order", &curWindow.SortOrderChoice, sortOrderItems, static_cast<int>(SortOrder::Max));
			ImGuiEx::AddTooltipToLastItem("Decides how targets and skills are sorted in the 'Targets' and 'Skills' sections.");

			if (ImGui::BeginMenu("stats exclude") == true)
			{
				ImGui::Checkbox("group", &curWindow.ExcludeGroup);
				ImGui::Checkbox("off-group", &curWindow.ExcludeOffGroup);
				ImGui::Checkbox("off-squad", &curWindow.ExcludeOffSquad);
				ImGui::Checkbox("summons", &curWindow.ExcludeMinions);
				ImGui::Checkbox("unmapped", &curWindow.ExcludeUnmapped);
				ImGui::EndMenu();
			}

			ImGui::InputText("short name", curWindow.Name, sizeof(curWindow.Name));
			ImGuiEx::AddTooltipToLastItem("The name used to represent this window in the \"heal stats\" menu");

			ImGui::InputText("window title", curWindow.TitleFormat, sizeof(curWindow.TitleFormat));
			ImGuiEx::AddTooltipToLastItem("Format for the title of this window.\n"
				                          "{1}: Total healing\n"
			                              "{2}: Total hits\n"
			                              "{3}: Total casts (not implemented yet)\n"
			                              "{4}: Healing per second\n"
			                              "{5}: Healing per hit\n"
			                              "{6}: Healing per cast (not implemented yet)\n"
			                              "{7}: Time in combat\n");

			ImGui::InputText("entry format", curWindow.EntryFormat, sizeof(curWindow.EntryFormat));
			ImGuiEx::AddTooltipToLastItem("Format for displayed data (statistics are per entry).\n"
			                              "{1}: Healing\n"
			                              "{2}: Hits\n"
			                              "{3}: Casts (not implemented yet)\n"
			                              "{4}: Healing per second\n"
			                              "{5}: Healing per hit\n"
			                              "{6}: Healing per cast (not implemented yet)\n"
			                              "{7}: Percent of total healing\n");

			float oldPosY = ImGui::GetCursorPosY();
			ImGui::BeginGroup();

			ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
			ImGui::Text("hotkey");

			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::SetCursorPosY(oldPosY);
			ImGui::InputInt("##HOTKEY", &curWindow.Hotkey, 0);

			ImGui::SameLine();
			ImGui::Text("(%s)", VirtualKeyToString(curWindow.Hotkey).c_str());

			ImGui::EndGroup();
			ImGuiEx::AddTooltipToLastItem("Numerical value (virtual key code) for the key\n"
				"used to open and close this window");

			ImGui::EndPopup();
		}

		for (const auto& entry : curWindow.CurrentAggregatedStats->GetStats())
		{
			ImGui::BeginGroup();
			ImGui::PushID(static_cast<int>(entry.Id));
			float startX = ImGui::GetCursorPosX();
			ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::SetCursorPosX(startX);
			ImGui::Text("%s", entry.Name.c_str());

			std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
				entry.Healing,
				entry.Hits,
				entry.Casts,
				divide_safe(entry.Healing, timeInCombat),
				divide_safe(entry.Healing, entry.Hits),
				entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
				divide_safe(entry.Healing * 100, aggregatedTotal.Healing)};
			ReplaceFormatted(buffer, sizeof(buffer), curWindow.EntryFormat, entryValues);
			ImGuiEx::TextRightAlignedSameLine("%s", buffer);

			ImGui::EndGroup();

			DetailsWindowState* state = nullptr;
			for (auto& iter : curWindow.OpenDetailWindows)
			{
				if (iter.Id == entry.Id)
				{
					state = &(iter);
					break;
				}
			}

			// If it was opened in a previous frame, we need the update the statistics stored so they are up to date
			if (state != nullptr)
			{
				*static_cast<AggregatedStatsEntry*>(state) = entry;
			}

			if (ImGui::IsItemClicked() == true)
			{
				if (state == nullptr)
				{
					state = &curWindow.OpenDetailWindows.emplace_back(entry);
				}
				state->IsOpen = !state->IsOpen;

				LOG("Toggled details window for entry %llu %s in window %u", entry.Id, entry.Name.c_str(), i);
			}
		}

		auto iter = curWindow.OpenDetailWindows.begin();
		while (iter != curWindow.OpenDetailWindows.end())
		{
			if (iter->IsOpen == false)
			{
				iter = curWindow.OpenDetailWindows.erase(iter);
				continue;
			}

			Display_DetailsWindow(curWindow, *iter);
			iter++;
		}

		ImGui::End();
	}
}

void Display_ArcDpsOptions(HealTableOptions& pHealingOptions)
{
	if (ImGui::BeginMenu("Heal Stats##HEAL") == true)
	{
		for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
		{
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "(%u) %s", i, pHealingOptions.Windows[i].Name);
			ImGui::Checkbox(buffer, &pHealingOptions.Windows[i].Shown);
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Heal Stats Options##HEAL") == true)
	{
		ImGui::Checkbox("debug mode", &pHealingOptions.DebugMode);
		ImGuiEx::AddTooltipToLastItem(
			"Includes debug data in target and skill names.\n"
			"Turn this on before taking screenshots of potential calculation issues.");

		ImGui::EndMenu();
	}
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
