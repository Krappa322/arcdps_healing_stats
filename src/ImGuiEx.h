#pragma once
#include "Utilities.h"

#include "imgui.h"

#include <optional>
#include <type_traits>

// Helpers for ImGui functions
namespace ImGuiEx
{
	bool SmallCheckBox(const char* pLabel, bool* pIsPressed);
	bool SmallInputText(const char* pLabel, char* pBuffer, size_t pBufferSize);
	void StatsEntry(const char* pLeftText, const char* pRightText, std::optional<float> pFillRatio);

	template <typename... Args>
	void TextRightAlignedSameLine(const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(buffer).x); // Sending x in SameLine messes with alignment when inside of a group
		ImGui::Text("%s", buffer);
	}

	template <typename... Args>
	void TextColoredCentered(ImColor pColor, const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize(buffer).x * 0.5f);
		ImGui::TextColored(pColor, "%s", buffer);
	}

	template <typename... Args>
	void BottomText(const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::CalcTextSize(buffer).y);
		ImGui::Text("%s", buffer);
	}

	template <typename... Args>
	void AddTooltipToLastItem(const char* pFormatString, Args... pArgs)
	{
		if (ImGui::IsItemHovered() == true)
		{
			ImGui::SetTooltip(pFormatString, pArgs...);
		}
	}

	template <typename EnumType, size_t Size = static_cast<size_t>(EnumType::Max)>
	void ComboMenu(const char* pLabel, EnumType& pCurrentItem, const EnumStringArray<EnumType, Size>& pItems)
	{
		static_assert(std::is_enum<EnumType>::value == true, "Accidental loss of type safety?");

		ImVec2 size{0, 0};
		for (const char* item : pItems)
		{
			ImVec2 itemSize = ImGui::CalcTextSize(item);
			size.x = (std::max)(size.x, itemSize.x);
			size.y += itemSize.y;
		}
		size.y += ImGui::GetStyle().ItemSpacing.y * (pItems.size() - 1);

		ImGui::SetNextWindowContentSize(size);
		if (ImGui::BeginMenu(pLabel) == true)
		{
			for (size_t i = 0; i < pItems.size(); i++)
			{
				ImGui::PushID(static_cast<int>(i));

				bool selected = (pCurrentItem == static_cast<EnumType>(i));
				if (ImGui::Selectable(pItems[i], selected, ImGuiSelectableFlags_DontClosePopups) == true)
				{
					pCurrentItem = static_cast<EnumType>(i);
				}

				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
	}

	template <typename EnumType>
	void SmallEnumCheckBox(const char* pLabel, EnumType* pSavedLocation, EnumType pStyleFlag, bool pCheckBoxIsInverseOfFlag)
	{
		static_assert(std::is_enum<EnumType>::value == true, "Accidental loss of type safety?");

		bool checked = (*pSavedLocation & pStyleFlag) == pStyleFlag;
		if (pCheckBoxIsInverseOfFlag == true)
		{
			checked = !checked;
		}

		if (ImGuiEx::SmallCheckBox(pLabel, &checked) == true)
		{
			*pSavedLocation = static_cast<EnumType>(*pSavedLocation ^ pStyleFlag);
		}
	}
}