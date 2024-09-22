#include "GUI.h"

#include "AggregatedStatsCollection.h"
#include "Exports.h"
#include "ImGuiEx.h"
#include "Log.h"
#include "Utilities.h"
#include "Widgets.h"

#include <array>
#include <Windows.h>

static constexpr EnumStringArray<AutoUpdateSettingEnum> AUTO_UPDATE_SETTING_ITEMS{
	"off", "on (only download stable)", "on (download prerelease/stable)"
};
static constexpr EnumStringArray<DataSource> DATA_SOURCE_ITEMS{
	"targets", "skills", "totals", "combined", "peers outgoing"};
static constexpr EnumStringArray<SortOrder> SORT_ORDER_ITEMS{
	"alphabetical ascending", "alphabetical descending", "heal per second ascending", "heal per second descending"};
static constexpr EnumStringArray<CombatEndCondition> COMBAT_END_CONDITION_ITEMS{
	"combat exit", "last damage event", "last heal event", "last damage / heal event"};
static constexpr EnumStringArray<spdlog::level::level_enum, 7> LOG_LEVEL_ITEMS{
	"trace", "debug", "info", "warning", "error", "critical", "off"};

static constexpr EnumStringArray<Position, static_cast<size_t>(Position::WindowRelative) + 1> POSITION_ITEMS{
	"manual", "screen relative", "window relative"};
static constexpr EnumStringArray<CornerPosition, static_cast<size_t>(CornerPosition::BottomRight) + 1> CORNER_POSITION_ITEMS{
	"top-left", "top-right", "bottom-left", "bottom-right"};

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
	ImGui::Begin(buffer, &pState.IsOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus);

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

	uint64_t adjustedHealing = (pState.Healing - pState.Barrier);

	ImGui::Text("total healing");
	ImGuiEx::TextRightAlignedSameLine("%llu", adjustedHealing);

	if (pState.Barrier > 0)
	{
		ImGui::Text("total barrier");
		ImGuiEx::TextRightAlignedSameLine("%llu", pState.Barrier);
	}

	ImGui::Text("hits");
	ImGuiEx::TextRightAlignedSameLine("%llu", pState.Hits);

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text("casts");
		ImGuiEx::TextRightAlignedSameLine("%llu", *pState.Casts);
	}

	ImGui::Text("healing per second");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(adjustedHealing, pState.TimeInCombat));

	ImGui::Text("healing per hit");
	ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(adjustedHealing, pState.Hits));

	if (pState.Casts.has_value() == true)
	{
		ImGui::Text("healing per cast");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(adjustedHealing, *pState.Casts));
	}

	if (pState.Barrier > 0)
	{
		ImGui::Text("barrier per second");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Barrier, pState.TimeInCombat));

		ImGui::Text("barrier per hit");
		ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Barrier, pState.Hits));

		if (pState.Casts.has_value() == true)
		{
			ImGui::Text("barrier per cast");
			ImGuiEx::TextRightAlignedSameLine("%.1f", divide_safe(pState.Barrier, *pState.Casts));
		}
	}

	ImGuiEx::BottomText("id %u", pState.Id);
	ImGui::EndChild();

	snprintf(buffer, sizeof(buffer), "##HEALDETAILS.ENTRIES.%i.%llu", static_cast<int>(pDataSource), pState.Id);
	ImGui::SameLine();
	ImGui::BeginChild(buffer, ImVec2(0, 0));

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetDetails(pDataSource, pState.Id);
	for (const auto& entry : stats.Entries)
	{
		// TODO: Add barrier here?
		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
				entry.Hits,
				entry.Casts,
				divide_safe(entry.Healing, entry.TimeInCombat),
				divide_safe(entry.Healing, entry.Hits),
				entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
				divide_safe(entry.Healing * 100, pState.Healing)};
		ReplaceFormatted(buffer, sizeof(buffer), pContext.DetailsEntryFormat, entryValues);

		float healingRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		float barrierRatio = static_cast<float>(divide_safe(entry.Barrier, stats.HighestHealing));

		std::string_view name = entry.Name;
		if (pContext.MaxNameLength > 0)
		{
			name = name.substr(0, pContext.MaxNameLength);
		}
		ImGuiEx::StatsEntry(name, buffer, pContext.ShowProgressBars == true ? std::optional{healingRatio} : std::nullopt, pContext.ShowProgressBars == true ? std::optional{barrierRatio} : std::nullopt);
	}
	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::End();
}

static void Display_Content(HealWindowContext& pContext, DataSource pDataSource, uint32_t pWindowIndex, bool pEvtcRpcEnabled)
{
	UNREFERENCED_PARAMETER(pWindowIndex);
	char buffer[1024];

	if (pDataSource == DataSource::PeersOutgoing)
	{
		if (pEvtcRpcEnabled == false)
		{
			ImGui::TextWrapped("Live stats sharing is disabled. Enable \"live stats sharing\" under \"Heal Stats Options\" in order to see the healing done by other players in the squad.");
			pContext.CurrentFrameLineCount += 3;
			return;
		}

		evtc_rpc_client_status status = GlobalObjects::EVTC_RPC_CLIENT->GetStatus();
		if (status.Connected == false)
		{
			ImGui::TextWrapped("Not connected to the live stats sharing server (\"%s\").", status.Endpoint.c_str());
			pContext.CurrentFrameLineCount += 3;
			return;
		}
	}

	const AggregatedStatsEntry& aggregatedTotal = pContext.CurrentAggregatedStats->GetTotal(pDataSource);

	const AggregatedVector& stats = pContext.CurrentAggregatedStats->GetStats(pDataSource);
	for (size_t i = 0; i < stats.Entries.size(); i++)
	{
		const auto& entry = stats.Entries[i];

		// TODO: Add barrier here?
		std::array<std::optional<std::variant<uint64_t, double>>, 7> entryValues{
			entry.Healing,
				entry.Hits,
				entry.Casts,
				divide_safe(entry.Healing, entry.TimeInCombat),
				divide_safe(entry.Healing, entry.Hits),
				entry.Casts.has_value() == true ? std::optional{divide_safe(entry.Healing, *entry.Casts)} : std::nullopt,
				pContext.DataSourceChoice != DataSource::Totals ? std::optional{divide_safe(entry.Healing * 100, aggregatedTotal.Healing)} : std::nullopt };
		ReplaceFormatted(buffer, sizeof(buffer), pContext.EntryFormat, entryValues);

		float healingRatio = static_cast<float>(divide_safe(entry.Healing, stats.HighestHealing));
		float barrierRatio = static_cast<float>(divide_safe(entry.Barrier, stats.HighestHealing));

		std::string_view name = entry.Name;
		if (pContext.MaxNameLength > 0)
		{
			name = name.substr(0, pContext.MaxNameLength);
		}
		float minSize = ImGuiEx::StatsEntry(name, buffer, pContext.ShowProgressBars == true ? std::optional{healingRatio} : std::nullopt, pContext.ShowProgressBars == true ? std::optional{barrierRatio} : std::nullopt);

		pContext.LastFrameMinWidth = (std::max)(pContext.LastFrameMinWidth, minSize);
		pContext.CurrentFrameLineCount += 1;

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

static void Display_WindowOptions_Position(HealTableOptions& pHealingOptions, HealWindowContext& pContext)
{
	ImGuiEx::SmallEnumRadioButton("PositionEnum", pContext.PositionRule, POSITION_ITEMS);

	switch (pContext.PositionRule)
	{
	case Position::ScreenRelative:
	{
		ImGui::Separator();
		ImGui::TextUnformatted("relative to corner");
		ImGuiEx::SmallIndent();
		ImGuiEx::SmallEnumRadioButton("ScreenCornerPositionEnum", pContext.RelativeScreenCorner, CORNER_POSITION_ITEMS);
		ImGuiEx::SmallUnindent();

		ImGui::PushItemWidth(39.0f);
		ImGuiEx::SmallInputInt("x", &pContext.RelativeX);
		ImGuiEx::SmallInputInt("y", &pContext.RelativeY);
		ImGui::PopItemWidth();

		break;
	}
	case Position::WindowRelative:
	{
		ImGui::Separator();
		ImGui::TextUnformatted("from anchor panel corner");
		ImGuiEx::SmallIndent();
		ImGuiEx::SmallEnumRadioButton("AnchorCornerPositionEnum", pContext.RelativeAnchorWindowCorner, CORNER_POSITION_ITEMS);
		ImGuiEx::SmallUnindent();

		ImGui::TextUnformatted("to this panel corner");
		ImGuiEx::SmallIndent();
		ImGuiEx::SmallEnumRadioButton("SelfCornerPositionEnum", pContext.RelativeSelfCorner, CORNER_POSITION_ITEMS);
		ImGuiEx::SmallUnindent();

		ImGui::PushItemWidth(39.0f);
		ImGuiEx::SmallInputInt("x", &pContext.RelativeX);
		ImGuiEx::SmallInputInt("y", &pContext.RelativeY);
		ImGui::PopItemWidth();

		ImGuiWindow* selectedWindow = ImGui::FindWindowByID(pContext.AnchorWindowId);
		const char* selectedWindowName = "";
		if (selectedWindow != nullptr)
		{
			selectedWindowName = selectedWindow->Name;
		}

		ImGui::SetNextItemWidth(260.0f);
		if (ImGui::BeginCombo("anchor window", selectedWindowName) == true)
		{
			// This doesn't return the same thing as RootWindow interestingly enough, RootWindow returns a "higher" parent
			ImGuiWindow* parent = ImGui::GetCurrentWindowRead();
			while (parent->ParentWindow != nullptr)
			{
				parent = parent->ParentWindow;
			}

			for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows)
			{
				if (window != parent && // Not the window we're currently in
					window->ParentWindow == nullptr && // Not a child window of another window
					window->Hidden == false && // Not hidden
					window->WasActive == true && // Not closed (we check ->WasActive because ->Active might not be true yet if the window gets rendered after this one)
					window->IsFallbackWindow == false && // Not default window ("Debug##Default")
					(window->Flags & ImGuiWindowFlags_Tooltip) == 0) // Not a tooltip window ("##Tooltip_<id>")
				{
					std::string windowName = "(";
					windowName += std::to_string(pHealingOptions.AnchoringHighlightedWindows.size());
					windowName += ") ";
					windowName += window->Name;

					// Print the window with ##/### part still there - it doesn't really hurt (and with the presence
					// of ### it's arguably more correct, even though it probably doesn't matter for a Selectable if
					// the id is unique or not)
					if (ImGui::Selectable(windowName.c_str()))
					{
						pContext.AnchorWindowId = window->ID;
						FindAndResolveCyclicDependencies(pHealingOptions, std::distance(pHealingOptions.Windows.data(), &pContext));
					}

					pHealingOptions.AnchoringHighlightedWindows.emplace_back(window->ID);
				}
			}
			ImGui::EndCombo();
		}

		break;
	}
	default:
		break;
	}
}

static void Display_WindowOptions(HealTableOptions& pHealingOptions, HealWindowContext& pContext)
{
	if (ImGui::BeginPopupContextWindow("Options##HEAL") == true)
	{
		ImGuiEx::SmallIndent();
		ImGuiEx::ComboMenu("data source", pContext.DataSourceChoice, DATA_SOURCE_ITEMS);
		ImGuiEx::AddTooltipToLastItem("Decides what data is shown in the window");

		if (pContext.DataSourceChoice != DataSource::Totals)
		{
			ImGuiEx::ComboMenu("sort order", pContext.SortOrderChoice, SORT_ORDER_ITEMS);
			ImGuiEx::AddTooltipToLastItem("Decides how targets and skills are sorted in the 'Targets' and 'Skills' sections.");

			if (ImGui::BeginMenu("stats exclude") == true)
			{
				ImGuiEx::SmallCheckBox("group", &pContext.ExcludeGroup);
				ImGuiEx::SmallCheckBox("off-group", &pContext.ExcludeOffGroup);
				ImGuiEx::SmallCheckBox("off-squad", &pContext.ExcludeOffSquad);
				ImGuiEx::SmallCheckBox("summons", &pContext.ExcludeMinions);
				ImGuiEx::SmallCheckBox("unmapped", &pContext.ExcludeUnmapped);

				ImGui::EndMenu();
			}
		}

		ImGuiEx::ComboMenu("combat end", pContext.CombatEndConditionChoice, COMBAT_END_CONDITION_ITEMS);
		ImGuiEx::AddTooltipToLastItem("Decides what should be used for determining combat\n"
			"end (and consequently time in combat)");

		ImGuiEx::SmallUnindent();
		ImGui::Separator();

		if (ImGui::BeginMenu("Display") == true)
		{
			ImGuiEx::SmallCheckBox("draw bars", &pContext.ShowProgressBars);
			ImGuiEx::AddTooltipToLastItem("Show a colored bar under each entry signifying what the value of\n"
				"that entry is in proportion to the largest entry");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText("short name", pContext.Name, sizeof(pContext.Name));
			ImGuiEx::AddTooltipToLastItem("The name used to represent this window in the \"heal stats\" menu");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt("max name length", &pContext.MaxNameLength);
			ImGuiEx::AddTooltipToLastItem(
				"Truncate displayed names to this many characters. Set to 0 to disable.");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt("min displayed", &pContext.MinLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				"The minimum amount of lines of data to show in this window.");

			ImGui::SetNextItemWidth(39.0f);
			ImGuiEx::SmallInputInt("max displayed", &pContext.MaxLinesDisplayed);
			ImGuiEx::AddTooltipToLastItem(
				"The maximum amount of lines of data to show in this window. Set to 0 for no limit");

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText("stats format", pContext.EntryFormat, sizeof(pContext.EntryFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
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

			ImGui::SetNextItemWidth(260.0f);
			ImGuiEx::SmallInputText("title bar format", pContext.TitleFormat, sizeof(pContext.TitleFormat));
			if (pContext.DataSourceChoice != DataSource::Totals)
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

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Style") == true)
		{
			ImGuiEx::SmallEnumCheckBox("title bar", &pContext.WindowFlags, ImGuiWindowFlags_NoTitleBar, true);
			ImGuiEx::SmallEnumCheckBox("scroll bar", &pContext.WindowFlags, ImGuiWindowFlags_NoScrollbar, true);
			ImGuiEx::SmallEnumCheckBox("background", &pContext.WindowFlags, ImGuiWindowFlags_NoBackground, true);

			ImGui::Separator();

			ImGuiEx::SmallCheckBox("auto resize window", &pContext.AutoResize);
			if (pContext.AutoResize == false)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
			}
			ImGuiEx::SmallIndent();

			ImGui::SetNextItemWidth(65.0f);
			ImGuiEx::SmallInputInt("window width", &pContext.FixedWindowWidth);
			ImGuiEx::AddTooltipToLastItem(
				"Set to 0 for dynamic resizing of width");

			ImGuiEx::SmallUnindent();
			if (pContext.AutoResize == false)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleColor();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Position") == true)
		{
			Display_WindowOptions_Position(pHealingOptions, pContext);
			ImGui::EndMenu();
		}

		ImGui::Separator();

		float oldPosY = ImGui::GetCursorPosY();
		ImGui::BeginGroup();

		ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
		ImGui::Text("Hotkey");

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetCursorPosY(oldPosY);

		ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
		ImGui::InputInt("##HOTKEY", &pContext.Hotkey, 0);

		ImGui::SameLine();
		ImGui::Text("(%s)", VirtualKeyToString(pContext.Hotkey).c_str());

		ImGui::EndGroup();
		ImGuiEx::AddTooltipToLastItem("Numerical value (virtual key code) for the key\n"
			"used to open and close this window");

		ImGui::EndPopup();
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
		const AggregatedStatsEntry& aggregatedTotal = curWindow.CurrentAggregatedStats->GetTotal(curWindow.DataSourceChoice);

		if (curWindow.DataSourceChoice != DataSource::Totals)
		{
			// TODO: Add barrier here?
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
				aggregatedTotal.Healing,
					aggregatedTotal.Hits,
					aggregatedTotal.Casts,
					divide_safe(aggregatedTotal.Healing, aggregatedTotal.TimeInCombat),
					divide_safe(aggregatedTotal.Healing, aggregatedTotal.Hits),
					aggregatedTotal.Casts.has_value() == true ? std::optional{divide_safe(aggregatedTotal.Healing, *aggregatedTotal.Casts)} : std::nullopt,
					timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128ULL, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}
		else
		{
			std::array<std::optional<std::variant<uint64_t, double>>, 7> titleValues{
				timeInCombat };
			size_t written = ReplaceFormatted(buffer, 128ULL, curWindow.TitleFormat, titleValues);
			snprintf(buffer + written, sizeof(buffer) - written, "###HEALWINDOW%u", i);
		}

		ImGuiWindowFlags window_flags = curWindow.WindowFlags | ImGuiWindowFlags_NoCollapse;
		if (curWindow.PositionRule != Position::Manual &&
			(curWindow.PositionRule != Position::WindowRelative || curWindow.AnchorWindowId != 0))
		{
			window_flags |= ImGuiWindowFlags_NoMove;
		}

		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
		ImGui::Begin(buffer, &curWindow.Shown, window_flags);

		// Adjust x size. This is based on last frame width and has to happen before we draw anything since some items are
		// aligned to the right edge of the window.
		if (curWindow.AutoResize == true)
		{
			ImVec2 size = ImGui::GetCurrentWindowRead()->SizeFull;
			if (curWindow.FixedWindowWidth == 0)
			{
				if (curWindow.LastFrameMinWidth != 0)
				{
					size.x = curWindow.LastFrameMinWidth + ImGui::GetCurrentWindowRead()->ScrollbarSizes.x;
				}
				// else size.x = current size x
			}
			else
			{
				size.x = static_cast<float>(curWindow.FixedWindowWidth);
			}

			LogT("FixedWindowWidth={} LastFrameMinWidth={} x={}",
				curWindow.FixedWindowWidth, curWindow.LastFrameMinWidth, size.x);
			ImGui::SetWindowSize(size);
		}

		curWindow.LastFrameMinWidth = 0;
		curWindow.CurrentFrameLineCount = 0;

		curWindow.WindowId = ImGui::GetCurrentWindow()->ID;

		ImGui::SetNextWindowSize(ImVec2(170, 0));
		Display_WindowOptions(pHealingOptions, curWindow);

		if (curWindow.DataSourceChoice != DataSource::Combined)
		{
			Display_Content(curWindow, curWindow.DataSourceChoice, i, pHealingOptions.EvtcRpcEnabled);
		}
		else
		{
			curWindow.CurrentFrameLineCount += 3; // For the 3 headers

			ImGui::PushID(static_cast<int>(DataSource::Totals));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Totals");
			Display_Content(curWindow, DataSource::Totals, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Agents));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Targets");
			Display_Content(curWindow, DataSource::Agents, i, pHealingOptions.EvtcRpcEnabled);
			ImGui::PopID();

			ImGui::PushID(static_cast<int>(DataSource::Skills));
			ImGuiEx::TextColoredCentered(ImColor(0, 209, 165), "Skills");
			Display_Content(curWindow, DataSource::Skills, i, pHealingOptions.EvtcRpcEnabled);
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

		// Adjust y size. This is based on the current frame line count, so we do it at the end of the frame (meaning
		// the resize has no lag)
		if (curWindow.AutoResize == true)
		{
			ImVec2 size = ImGui::GetCurrentWindowRead()->SizeFull;

			size_t lineCount = (std::max)(curWindow.CurrentFrameLineCount, curWindow.MinLinesDisplayed);
			lineCount = (std::min)(lineCount, curWindow.MaxLinesDisplayed);

			size.y = ImGuiEx::CalcWindowHeight(lineCount);

			LogT("lineCount={} CurrentFrameLineCount={} MinLinesDisplayed={} MaxLinesDisplayed={} y={}",
				lineCount, curWindow.CurrentFrameLineCount, curWindow.MinLinesDisplayed, curWindow.MaxLinesDisplayed, size.y);

			ImGui::SetWindowSize(size);
		}
		ImGui::End();
	}
}

static void Display_EvtcRpcStatus(const HealTableOptions& pHealingOptions)
{
	evtc_rpc_client_status status = GlobalObjects::EVTC_RPC_CLIENT->GetStatus();
	if (status.Connected == true)
	{
		uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - status.ConnectTime).count();
		const char* encryptString = "with";
		if (status.Encrypted == false)
		{
			encryptString = "without";
		}
		ImGui::TextColored(ImVec4(0.0f, 0.75f, 0.0f, 1.0f), "Connected to %s for %llu seconds %s encryption", status.Endpoint.c_str(), seconds, encryptString);
	}
	else if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Not connected since live stats sharing is disabled");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.75f, 0.0f, 0.0f, 1.0f), "Failed connecting to %s. Retrying...", status.Endpoint.c_str());
	}
}

void Display_AddonOptions(HealTableOptions& pHealingOptions)
{
	ImGui::TextUnformatted("Heal Stats");
	ImGuiEx::SmallIndent();
	for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
	{
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "(%u) %s", i, pHealingOptions.Windows[i].Name);
		ImGuiEx::SmallCheckBox(buffer, &pHealingOptions.Windows[i].Shown);
	}
	ImGuiEx::SmallUnindent();

	ImGuiEx::ComboMenu("auto updates", pHealingOptions.AutoUpdateSetting, AUTO_UPDATE_SETTING_ITEMS);
	ImGuiEx::SmallCheckBox("debug mode", &pHealingOptions.DebugMode);
	ImGuiEx::AddTooltipToLastItem(
		"Includes debug data in target and skill names.\n"
		"Turn this on before taking screenshots of potential calculation issues.");

	spdlog::string_view_t log_level_names[] = SPDLOG_LEVEL_NAMES;
	std::string log_level_items[std::size(log_level_names)];
	for (size_t i = 0; i < std::size(log_level_names); i++)
	{
		log_level_items[i] = std::string_view(log_level_names[i].data(), log_level_names[i].size());
	}

	if (ImGuiEx::ComboMenu("debug logging", pHealingOptions.LogLevel, LOG_LEVEL_ITEMS) == true)
	{
		Log_::SetLevel(pHealingOptions.LogLevel);
	}
	ImGuiEx::AddTooltipToLastItem(
		"If not set to off, enables logging at the specified log level.\n"
		"Logs are saved in addons\\logs\\arcdps_healing_stats\\. Logging\n"
		"will have a small impact on performance.");

	ImGui::Separator();


	if (ImGuiEx::SmallCheckBox("Include barrier", &pHealingOptions.IncludeBarrier) == true)
	{
		GlobalObjects::EVENT_PROCESSOR->SetUseBarrier(pHealingOptions.IncludeBarrier);
	}
	ImGui::Separator();



	if (ImGuiEx::SmallCheckBox("log healing to EVTC logs", &pHealingOptions.EvtcLoggingEnabled) == true)
	{
		GlobalObjects::EVENT_PROCESSOR->SetEvtcLoggingEnabled(pHealingOptions.EvtcLoggingEnabled);
	}

	if (ImGuiEx::SmallCheckBox("enable live stats sharing", &pHealingOptions.EvtcRpcEnabled) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetEnabledStatus(pHealingOptions.EvtcRpcEnabled);
	}
	ImGuiEx::AddTooltipToLastItem(
		"Enables live sharing of healing statistics with other\n"
		"players in your squad. This is done through a central server\n"
		"(see option below). After enabling live sharing, it might not\n"
		"work properly until after changing map instance so all squad\n"
		"members are properly detected.\n"
		"\n"
		"The stats shared from other players can be viewed through a\n"
		"heal stats window with its data source set to \"%s\"\n"
		"\n"
		"Enabling this option has a small impact on performance, and\n"
		"will put some additional load on your connection (a maximum\n"
		"of about 10 kiB/s up and 100 kiB/s down). The download\n"
		"connection usage increases the more players in your squad\n"
		"have live stats sharing enabled, the upload connection usage\n"
		"does not."
		, DATA_SOURCE_ITEMS[DataSource::PeersOutgoing]);

	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128, 255));
	}
	if (ImGuiEx::SmallCheckBox("live stats sharing - budget mode", &pHealingOptions.EvtcRpcBudgetMode) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetBudgetMode(pHealingOptions.EvtcRpcBudgetMode);
	}
	ImGuiEx::AddTooltipToLastItem(
		"Only send a minimal subset of events to peers. This reduces\n"
		"the amount of upload bandwidth used by the addon. Healing\n"
		"statistics shown will still be fully accurate, however combat\n"
		"times as viewed by other players may be slightly inaccurate\n"
		"while still in combat. If those players are running a version\n"
		"of the addon released before budget mode support was\n"
		"introduced, the combat times may be highly inaccurate, even\n"
		"when out of combat. This option has no effect on download\n"
		"bandwidth usage, only upload. Expected connection usage with\n"
		"this option enabled should go down to <1kiB/s up.");

	if (ImGuiEx::SmallCheckBox("live stats sharing - disable encryption", &pHealingOptions.EvtcRpcDisableEncryption) == true)
	{
		GlobalObjects::EVTC_RPC_CLIENT->SetDisableEncryption(pHealingOptions.EvtcRpcDisableEncryption);
	}
	ImGuiEx::AddTooltipToLastItem(
		"By default, all messages sent to and from the live stats\n"
		"sharing server are encrypted using TLS. This option disables\n"
		"the encryption, which can be useful to work around certain\n"
		"security applications, as well as to reduce the CPU usage of\n"
		"live stats sharing");

	if (pHealingOptions.EvtcRpcEnabled == false)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}

	float oldPosY = ImGui::GetCursorPosY();
	ImGui::BeginGroup();

	ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
	ImGui::Text("live stats sharing hotkey");

	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::SetCursorPosY(oldPosY);

	ImGui::SetNextItemWidth(ImGui::CalcTextSize("-----").x);
	ImGui::InputInt("##HOTKEY", &pHealingOptions.EvtcRpcEnabledHotkey, 0);

	ImGui::SameLine();
	ImGui::Text("(%s)", VirtualKeyToString(pHealingOptions.EvtcRpcEnabledHotkey).c_str());

	ImGui::EndGroup();
	ImGuiEx::AddTooltipToLastItem("Numerical value (virtual key code) for the key\n"
		"used to toggle live stats sharing");

	ImGuiEx::SmallInputText("evtc rpc server", pHealingOptions.EvtcRpcEndpoint, sizeof(pHealingOptions.EvtcRpcEndpoint));
	ImGuiEx::AddTooltipToLastItem(
		"The server to communicate with for evtc_rpc communication\n"
		"(allowing other squad members to see your healing stats).\n"
		"All local combat events will be sent to this server. Make\n"
		"sure you trust it.");
	Display_EvtcRpcStatus(pHealingOptions);

	ImGui::Separator();

	if (ImGui::Button("reset all settings") == true)
	{
		pHealingOptions.Reset();
		LogD("Reset settings");
	}
	ImGuiEx::AddTooltipToLastItem("Resets all global and window specific settings to their default values.");
}

void FindAndResolveCyclicDependencies(HealTableOptions& pHealingOptions, size_t pStartIndex)
{
	std::unordered_map<ImGuiID, size_t> windowIdToWindowIndex;
	for (size_t i = 0; i < pHealingOptions.Windows.size(); i++)
	{
		HealWindowContext& window = pHealingOptions.Windows[i];
		if (window.WindowId != 0)
		{
			windowIdToWindowIndex.emplace(window.WindowId, i);
		}
	}

	std::vector<size_t> traversedNodes;
	size_t curIndex = pStartIndex;
	while (pHealingOptions.Windows[curIndex].AnchorWindowId != 0)
	{
		if (std::find(traversedNodes.begin(), traversedNodes.end(), curIndex) != traversedNodes.end())
		{
			traversedNodes.emplace_back(curIndex); // add the node to the list so the printed string is more useful
			std::string loop_string =
				std::accumulate(traversedNodes.begin(), traversedNodes.end(), std::string{},
					[](const std::string& pLeft, const size_t& pRight) -> std::string
					{
						std::string separator = (pLeft.length() > 0 ? " -> " : "");
						return pLeft + separator + std::to_string(pRight);
					});

			LogW("Found cyclic loop {}, removing link from {}", loop_string, pStartIndex);

			pHealingOptions.Windows[pStartIndex].AnchorWindowId = 0;
			return;
		}

		traversedNodes.emplace_back(curIndex);

		auto iter = windowIdToWindowIndex.find(pHealingOptions.Windows[curIndex].AnchorWindowId);
		if (iter == windowIdToWindowIndex.end())
		{
			LogD("Chain starting at {} points to window {} which is not a window we have control over, can't determine if it's cyclic or not",
				pStartIndex, pHealingOptions.Windows[curIndex].AnchorWindowId);
			return;
		}
		curIndex = iter->second;
	}

	LogD("Chain starting at {} ends at 0, not cyclic", pStartIndex);
}

static void RepositionWindows(HealTableOptions& pHealingOptions)
{
	size_t iterations = 0;
	size_t lastChangedPos = 0;

	// Loop windows and try to reposition them. Whenever a window was moved, try to reposition all windows again (to
	// solve possible dependencies between windows). Maximum loop count is equal to the window count, otherwise the
	// windows probably have a circular dependency
	for (; iterations < pHealingOptions.Windows.size(); iterations++)
	{
		for (size_t i = 0; i < pHealingOptions.Windows.size(); i++)
		{
			if (iterations > 0 && i == lastChangedPos)
			{
				LogT("Aborting repositioning on position {} after {} iterations", lastChangedPos, iterations);
				return;
			}

			const HealWindowContext& curWindow = pHealingOptions.Windows[i];
			ImVec2 posVector{ static_cast<float>(curWindow.RelativeX), static_cast<float>(curWindow.RelativeY) };

			ImGuiWindow* window = ImGui::FindWindowByID(curWindow.WindowId);
			if (window != nullptr)
			{
				if (ImGuiEx::WindowReposition(
					window,
					curWindow.PositionRule,
					posVector,
					curWindow.RelativeScreenCorner,
					curWindow.AnchorWindowId,
					curWindow.RelativeAnchorWindowCorner,
					curWindow.RelativeSelfCorner) == true)
				{
					LogT("Window {} moved!", i);
					lastChangedPos = i;
				}
			}
		}
	}

	LogW("Aborted repositioning after {} iterations - recursive window relationships?", iterations);
}

void Display_PostNewFrame(ImGuiContext* /*pImguiContext*/, HealTableOptions& pHealingOptions)
{
	// Repositioning windows at the start of the frame prevents lag when moving windows
	RepositionWindows(pHealingOptions);
}

void Display_PreEndFrame(ImGuiContext* pImguiContext, HealTableOptions& pHealingOptions)
{
	float font_size = pImguiContext->FontSize * 2.0f;

	for (size_t i = 0; i < pHealingOptions.AnchoringHighlightedWindows.size(); i++)
	{
		ImGuiWindow* window = ImGui::FindWindowByID(pHealingOptions.AnchoringHighlightedWindows[i]);

		if (window != nullptr)
		{
			std::string text = std::to_string(i);

			ImVec2 regionSize{ 32.0f, 32.0f };
			ImVec2 regionPos = DrawListEx::CalcCenteredPosition(window->Pos, window->Size, regionSize);

			ImVec2 textSize = pImguiContext->Font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, text.c_str());
			ImVec2 textPos = DrawListEx::CalcCenteredPosition(regionPos, regionSize, textSize);

			window->DrawList->AddRectFilled(regionPos, regionPos + regionSize, IM_COL32(0, 0, 0, 255));
			window->DrawList->AddText(pImguiContext->Font, font_size, textPos, ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
		}
	}
	//
	//RepositionWindows(pHealingOptions);
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