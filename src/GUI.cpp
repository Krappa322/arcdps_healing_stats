#include "GUI.h"

#include "AggregatedStatsCollection.h"
#include "Exports.h"
#include "ImGuiEx.h"
#include "Log.h"
#include "Utilities.h"

#include <array>
#include <Windows.h>

static void Display_DetailsWindow(HealWindowContext& pContext, DetailsWindowState& pState, DataSource pDataSource)
{
	if (pState.IsOpen == false)
	{
		return;
	}

	char buffer[1024];
	// Using "###" means the id of the window is calculated only from the part after the hashes (which
	// in turn means that the name of the window can change if necessary)
	snprintf(buffer, sizeof(buffer), "%s###HEALDETAILS.%i.%llu", pState.Name.c_str(), static_cast<int>(pDataSource), pState.Id);
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
	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.TOTALS.%i.%llu", static_cast<int>(pDataSource), pState.Id);
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
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.TimeInCombat));

	ImGui::Text("healing per hit");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, pState.Hits));

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text("healing per cast");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Healing, *pState.Casts));
	}

	ImGuiEx::BottomText("id %u", pState.Id);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.ENTRIES.%i.%llu", static_cast<int>(pDataSource), pState.Id);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetDetails(pDataSource, pState.Id);
	for (const auto& entry : stats.Entries)
	{
		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
			entry.Hits,
			entry.Casts,
			divide_safe(entry.Healing, entry.TimeInCombat),
			divide_safe(entry.Healing, entry.Hits),
			entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
			divide_safe(entry.Healing * 100, pState.Healing)};
		ReplaceFormatted(buffer, sizeof(buffer), pContext.DetailsEntryFormat, entryValues);

		float fillRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		ImGuiEx::StatsEntry(entry.Name.c_str(), buffer, pContext.ShowProgressBars == true ? std::optional{fillRatio} : std::nullopt);
	}
	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::End();
}

static void Display_Content(HealWindowContext& pContext, DataSource pDataSource, uint32_t pWindowIndex)
{
	UNREFERENCED_PARAMETER(pWindowIndex);
	char buffer[1024];

	const AggregatedStatsEntry& aggregatedTotal = pContext.CurrentAggregatedStats->GetTotal(pDataSource);

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetStats(pDataSource);
	for (int i = 0; i < stats.Entries.size(); i++)
	{
		const auto& entry = stats.Entries[i];

		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
			entry.Hits,
			entry.Casts,
			divide_safe(entry.Healing, entry.TimeInCombat),
			divide_safe(entry.Healing, entry.Hits),
			entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
			static_cast<DataSource>(pContext.DataSourceChoice) != DataSource::Totals ? std::optional{divide_safe(entry.Healing * 100, aggregatedTotal.Healing)} : std::nullopt };
		ReplaceFormatted(buffer, sizeof(buffer), pContext.EntryFormat, entryValues);

		float fillRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		ImGuiEx::StatsEntry(entry.Name.c_str(), buffer, pContext.ShowProgressBars == true ? std::optional{fillRatio} : std::nullopt);

		DetailsWindowState* state = nullptr;
		std::vector<DetailsWindowState>* vec;
		switch (pDataSource)
		{
		case DataSource::Agents:
			vec = &pContext.OpenAgentWindows;
			break;
		case DataSource::Skills:
			vec = &pContext.OpenSkillWindows;
			break;
		case DataSource::PeersOutgoing:
			vec = &pContext.OpenPeersOutgoingWindows;
			break;
		default:
			vec = nullptr;
			break;
		}

		if (vec != nullptr)
		{
			for (auto& iter : *vec)
			{
				if (iter.Id == entry.Id)
				{
					state = &(iter);
					break;
				}
			}
		}

		// If it was opened in a previous frame, we need the update the statistics stored so they are up to date
		if (state != nullptr)
		{
			*static_cast<AggregatedStatsEntry*>(state) = entry;
		}

		if (vec != nullptr && ImGui::IsItemClicked() == true)
		{
			if (state == nullptr)
			{
				state = &vec->emplace_back(entry);
			}
			state->IsOpen = !state->IsOpen;

			LOG("Toggled details window for entry %llu %s in window %u", entry.Id, entry.Name.c_str(), pWindowIndex);
		}
	}
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

			auto [localId, states] = GlobalObjects::EVENT_PROCESSOR->GetState();
			curWindow.CurrentAggregatedStats = std::make_unique<AggregatedStatsCollection>(std::move(states), localId, curWindow, pHealingOptions.DebugMode);
			curWindow.LastAggregatedTime = curTime;
		}

		float timeInCombat = curWindow.CurrentAggregatedStats->GetCombatTime();
		const AggregatedStatsEntry& aggregatedTotal = curWindow.CurrentAggregatedStats->GetTotal(static_cast<DataSource>(curWindow.DataSourceChoice));

		if (static_cast<DataSource>(curWindow.DataSourceChoice) != DataSource::Totals)
		{
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
				aggregatedTotal.Healing,
				aggregatedTotal.Hits,
				aggregatedTotal.Casts,
				divide_safe(aggregatedTotal.Healing, aggregatedTotal.TimeInCombat),
				divide_safe(aggregatedTotal.Healing, aggregatedTotal.Hits),
				aggregatedTotal.Casts.has_value() == true ? std::optional{divide_safe(aggregatedTotal.Healing, *aggregatedTotal.Casts)} : std::nullopt,
				timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}
		else
		{
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
			timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}

		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
		ImGui::Begin(buffer, &curWindow.Shown, ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
		{
			const char* const dataSourceItems[] = {"targets", "skills", "totals", "combined", "peers outgoing"};
			static_assert((sizeof(dataSourceItems) / sizeof(dataSourceItems[0])) == static_cast<uint64_t>(DataSource::Max), "Added data source without updating gui?");
			ImGui::Combo("data source", &curWindow.DataSourceChoice, dataSourceItems, static_cast<int>(DataSource::Max));
			ImGuiEx::AddTooltipToLastItem("Decides how targets and skills are sorted in the 'Targets' and 'Skills' sections.");

			if (static_cast<DataSource>(curWindow.DataSourceChoice) != DataSource::Totals)
			{
				const char* const sortOrderItems[] = { "alphabetical ascending", "alphabetical descending", "heal per second ascending", "heal per second descending" };
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
			}

			const char* const combatEndConditionItems[] = { "combat exit", "last damage event", "last heal event", "last damage / heal event" };
			static_assert((sizeof(combatEndConditionItems) / sizeof(combatEndConditionItems[0])) == static_cast<uint64_t>(CombatEndCondition::Max), "Added combat end condition without updating gui?");
			ImGui::Combo("combat end", &curWindow.CombatEndConditionChoice, combatEndConditionItems, static_cast<int>(CombatEndCondition::Max));
			ImGuiEx::AddTooltipToLastItem("Decides what should be used for determining combat\n"
			                              "end (and consequently time in combat)");

			ImGui::Checkbox("show progress bars", &curWindow.ShowProgressBars);
			ImGuiEx::AddTooltipToLastItem("Show a colored bar under each entry signifying what the value of\n"
			                              "that entry is in proportion to the largest entry");

			ImGui::InputText("short name", curWindow.Name, sizeof(curWindow.Name));
			ImGuiEx::AddTooltipToLastItem("The name used to represent this window in the \"heal stats\" menu");

			ImGui::InputText("window title", curWindow.TitleFormat, sizeof(curWindow.TitleFormat));
			if (static_cast<DataSource>(curWindow.DataSourceChoice) != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem("Format for the title of this window.\n"
											  "{1}: Total healing\n"
											  "{2}: Total hits\n"
											  "{3}: Total casts (not implemented yet)\n"
											  "{4}: Healing per second\n"
											  "{5}: Healing per hit\n"
											  "{6}: Healing per cast (not implemented yet)\n"
											  "{7}: Time in combat");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem("Format for the title of this window.\n"
											  "{1}: Time in combat");
			}

			ImGui::InputText("entry format", curWindow.EntryFormat, sizeof(curWindow.EntryFormat));
			if (static_cast<DataSource>(curWindow.DataSourceChoice) != DataSource::Totals)
			{
				ImGuiEx::AddTooltipToLastItem("Format for displayed data (statistics are per entry).\n"
											  "{1}: Healing\n"
											  "{2}: Hits\n"
											  "{3}: Casts (not implemented yet)\n"
											  "{4}: Healing per second\n"
											  "{5}: Healing per hit\n"
											  "{6}: Healing per cast (not implemented yet)\n"
											  "{7}: Percent of total healing");
			}
			else
			{
				ImGuiEx::AddTooltipToLastItem("Format for displayed data (statistics are per entry).\n"
											  "{1}: Healing\n"
											  "{2}: Hits\n"
											  "{3}: Casts (not implemented yet)\n"
											  "{4}: Healing per second\n"
											  "{5}: Healing per hit\n"
											  "{6}: Healing per cast (not implemented yet)");
			}

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

		if (static_cast<DataSource>(curWindow.DataSourceChoice) != DataSource::Combined)
		{
			Display_Content(curWindow, static_cast<DataSource>(curWindow.DataSourceChoice), i);
		}
		else
		{
			ImGui::PushID(static_cast<int>(DataSource::Totals));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Totals");
			Display_Content(curWindow, DataSource::Totals, i);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Agents));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Targets");
			Display_Content(curWindow, DataSource::Agents, i);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Skills));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Skills");
			Display_Content(curWindow, DataSource::Skills, i);
			ImGui::PopID();
		}

		for (const DataSource dataSource : std::array{DataSource::Agents, DataSource::Skills, DataSource::PeersOutgoing})
		{
			std::vector<DetailsWindowState>* vec;
			switch (dataSource)
			{
			case DataSource::Agents:
				vec = &curWindow.OpenAgentWindows;
				break;
			case DataSource::Skills:
				vec = &curWindow.OpenSkillWindows;
				break;
			case DataSource::PeersOutgoing:
				vec = &curWindow.OpenPeersOutgoingWindows;
				break;
			default:
				vec = nullptr;
				break;
			}

			auto iter = vec->begin();
			while (iter != vec->end())
			{
				if (iter->IsOpen == false)
				{
					iter = vec->erase(iter);
					continue;
				}

				Display_DetailsWindow(curWindow, *iter, dataSource);
				iter++;
			}
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

		ImGui::InputText("evtc rpc server", pHealingOptions.EvtcRpcEndpoint, sizeof(pHealingOptions.EvtcRpcEndpoint));
		ImGuiEx::AddTooltipToLastItem(
			"The server to communicate with for evtc_rpc communication\n"
			"(allowing other squad members to see your healing stats).\n"
			"All local combat events will be sent to this server. Make\n"
			"sure you trust it.");

		if (ImGui::Button("reset all settings") == true)
		{
			pHealingOptions = HealTableOptions{};
			LOG("Reset settings");
		}
		ImGuiEx::AddTooltipToLastItem("Resets all global and window specific settings to their default values.");

		ImGui::EndMenu();
	}
}

void ImGui_ProcessKeyEvent(HWND /*pWindowHandle*/, UINT pMessage, WPARAM pAdditionalW, LPARAM /*pAdditionalL*/)
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
