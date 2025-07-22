#pragma once
#include "Utilities.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <optional>
#include <string_view>
#include <type_traits>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
// Helpers for ImGui functions
namespace ImGuiEx
{
	class RemoveFramePadding
	{
	public:
		RemoveFramePadding()
		{
			ImVec2 padding = ImGui::GetStyle().FramePadding;
			padding.y = 1;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
		}

		~RemoveFramePadding()
		{
			ImGui::PopStyleVar();
		}
	};

	float CalcWindowHeight(size_t pLineCount, ImGuiWindow* pWindow = nullptr);

	bool SmallCheckBox(const char* pLabel, bool* pIsPressed);
	bool SmallInputFloat(const char* pLabel, float* pFloat);
	bool SmallInputText(const char* pLabel, char* pBuffer, size_t pBufferSize);
	bool SmallRadioButton(const char* pLabel, bool pIsPressed);

	void SmallIndent();
	void SmallUnindent();

	// returns minimum size needed to display the entry
	float StatsEntry(std::string_view pLeftText, std::string_view pRightText, std::optional<float> pFillRatio, std::optional<float> pBarrierGenerationRatio, std::optional<size_t> indexNumber, bool self);

	template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	bool SmallInputInt(const char* pLabel, T* pInt)
	{
		ImGuiDataType dataType;
		if constexpr (std::is_same_v<T, int32_t>)
		{
			dataType = ImGuiDataType_S32;
		}
		else if constexpr (std::is_same_v<T, uint64_t>)
		{
			dataType = ImGuiDataType_U64;
		}
		else if constexpr (std::is_same_v<T, int64_t>)
		{
			dataType = ImGuiDataType_S64;
		}
		else
		{
			assert(false);
			//static_assert(false, "unhandled type");
		}

		RemoveFramePadding pad;
		return ImGui::InputScalar(pLabel, dataType, pInt);
	}

	template <typename... Args>
	float TextRightAlignedSameLine(const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		float textSize = ImGui::CalcTextSize(buffer).x;

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - textSize); // Sending x in SameLine messes with alignment when inside of a group
		ImGui::Text("%s", buffer);

		return textSize;
	}

	template <typename... Args>
	float DetailsSummaryEntry(const char* pCategoryName, const char* pFormatString, Args... pArgs)
	{
		float leftTextSize = ImGui::CalcTextSize(pCategoryName).x;
		ImGui::Text(pCategoryName);

		float rightTextSize = TextRightAlignedSameLine(pFormatString, pArgs...);
		// Two item spacings between the left and right text, and WindowPadding on each side of the left and right text.
		return leftTextSize + ImGui::GetStyle().ItemSpacing.x * 2.0f + ImGui::GetCurrentWindowRead()->WindowPadding.x * 2.0f + rightTextSize;
	}

	template <typename... Args>
	void TextColoredCentered(ImColor pColor, const char* pFormatString, Args... pArgs)
	{
		char buffer[1024];

		snprintf(buffer, sizeof(buffer), pFormatString, pArgs...);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize(buffer).x * 0.5f);
		ImGui::TextColored(pColor, "%s", buffer);
	}

	void TextColoredUnformatted(std::optional<ImU32> pColor, const char* pText, const char* pTextEnd = nullptr);

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
	bool ComboMenu(const char* pLabel, EnumType& pCurrentItem, const EnumStringArray<EnumType, Size>& pItems)
	{
		static_assert(std::is_enum_v<EnumType>, "Accidental loss of type safety?");

		ImVec2 size{0, 0};
		for (const char* item : pItems)
		{
			ImVec2 itemSize = ImGui::CalcTextSize(item);
			size.x = (std::max)(size.x, itemSize.x);
			size.y += itemSize.y;
		}
		size.y += ImGui::GetStyle().ItemSpacing.y * (pItems.size() - 1);

		bool value_changed = false;
		ImGui::SetNextWindowContentSize(size);
		if (ImGui::BeginMenu(pLabel) == true)
		{
			for (size_t i = 0; i < pItems.size(); i++)
			{
				ImGui::PushID(static_cast<int>(i));

				bool selected = (pCurrentItem == static_cast<EnumType>(i));
				if (ImGui::Selectable(pItems[i], selected, ImGuiSelectableFlags_DontClosePopups) == true)
				{
					value_changed = true;
					pCurrentItem = static_cast<EnumType>(i);
				}

				ImGui::PopID();
			}
			ImGui::EndMenu();
		}

		return value_changed;
	}

	template <typename EnumType, size_t Size = static_cast<size_t>(EnumType::Max)>
	void SmallEnumRadioButton(const char* pInvisibleLabel, EnumType& pCurrentItem, const EnumStringArray<EnumType, Size>& pItems)
	{
		ImGui::PushID(pInvisibleLabel);
		for (size_t i = 0; i < pItems.size(); i++)
		{
			ImGui::PushID(static_cast<int>(i));

			bool selected = (pCurrentItem == static_cast<EnumType>(i));
			if (SmallRadioButton(pItems[i], selected) == true)
			{
				pCurrentItem = static_cast<EnumType>(i);
			}

			ImGui::PopID();
		}
		ImGui::PopID();
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

namespace DrawListEx
{
	static inline ImVec2 CalcCenteredPosition(const ImVec2& pBoundsPosition, const ImVec2& pBoundsSize, const ImVec2& pItemSize)
	{
		return ImVec2{
			pBoundsPosition.x + (std::max)((pBoundsSize.x - pItemSize.x) * 0.5f, 0.0f),
			pBoundsPosition.y + (std::max)((pBoundsSize.y - pItemSize.y) * 0.5f, 0.0f)
		};
	}
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif