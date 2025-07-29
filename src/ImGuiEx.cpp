#include "ImGuiEx.h"

float ImGuiEx::CalcWindowHeight(size_t pLineCount, float pExtraHeight, ImGuiWindow* pWindow)
{
	if (pWindow == nullptr)
	{
		pWindow = ImGui::GetCurrentWindowRead();
	}

	//return pWindow->TitleBarHeight() + ImGui::GetStyle().WindowPadding.y * 2 + pLineCount * ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;

	float decorationsSize = pWindow->TitleBarHeight() + pWindow->MenuBarHeight();
	float padding = pWindow->WindowPadding.y * 2.0f;
	float contentSize = 0;
	if (pLineCount > 0)
	{
		contentSize = pLineCount * ImGui::GetTextLineHeight() + (pLineCount - 1) * ImGui::GetStyle().ItemSpacing.y;
	}
	return decorationsSize + padding + contentSize + pExtraHeight;
}

bool ImGuiEx::SmallCheckBox(const char* pLabel, bool* pIsPressed)
{
	RemoveFramePadding pad;
	bool result = ImGui::Checkbox(pLabel, pIsPressed);
	return result;
}

bool ImGuiEx::SmallInputFloat(const char* pLabel, float* pFloat)
{
	RemoveFramePadding pad;
	bool result = ImGui::InputFloat(pLabel, pFloat, 0.0f, 0.0f, "%.1f");
	return result;
}

bool ImGuiEx::SmallInputText(const char* pLabel, char* pBuffer, size_t pBufferSize)
{
	RemoveFramePadding pad;
	bool result = ImGui::InputText(pLabel, pBuffer, pBufferSize);
	return result;
}

bool ImGuiEx::SmallRadioButton(const char* pLabel, bool pIsPressed)
{
	RemoveFramePadding pad;
	bool result = ImGui::RadioButton(pLabel, pIsPressed);
	return result;
}

void ImGuiEx::SmallIndent()
{
	ImGui::Indent(ImGui::GetCurrentContext()->FontSize);
}

void ImGuiEx::SmallUnindent()
{
	ImGui::Unindent(ImGui::GetCurrentContext()->FontSize);
}

float ImGuiEx::StatsEntry(std::string_view pLeftText, std::string_view pRightText, std::optional<float> pFillRatio, std::optional<float> pBarrierGenerationRatio, std::optional<std::string_view> pIndexNumber, std::optional<std::string> pProfessionText, void* pProfessionIcon, std::optional<ImVec4> pLeftTextColour, std::optional<ImVec4> pHealColour, std::optional<ImVec4> pBarrierGenerationColour, bool pSelf)
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(204, 204, 212, 255));
	ImGui::BeginGroup();

	//ImGui::PushID(pUniqueId);
	//float startX = ImGui::GetCursorPosX();
	//ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
	//ImGui::PopID();

	ImVec2 leftTextSize = ImGui::CalcTextSize(pLeftText.data(), pLeftText.data() + pLeftText.size());
	ImVec2 rightTextSize = ImGui::CalcTextSize(pRightText.data(), pRightText.data() + pRightText.size());
	ImVec2 indexNumberSize = ImVec2();
	ImVec2 professionTextSize = ImVec2();
	ImVec2 professionIconSize = ImVec2();

	if (pFillRatio.has_value() == true)
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();

		float healingRatio = *pFillRatio;

		ImU32 healingColor = pHealColour.has_value() ? ImGui::ColorConvertFloat4ToU32(*pHealColour) : IM_COL32(102, 178, 102, 128);
		ImU32 barrierColor = pBarrierGenerationColour.has_value() ? ImGui::ColorConvertFloat4ToU32(*pBarrierGenerationColour) : IM_COL32(255, 225, 0, 128);

		if (pBarrierGenerationRatio.has_value() == true)
		{
			float barrierGenerationRatio = *pBarrierGenerationRatio;
			healingRatio -= barrierGenerationRatio;

			ImVec2 barrierStart = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * healingRatio, pos.y);
			ImVec2 barrierEnd = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * (healingRatio + barrierGenerationRatio), pos.y + ImGui::GetTextLineHeight());

			ImGui::GetWindowDrawList()->AddRectFilled(barrierStart, barrierEnd, barrierColor);
		}

		ImVec2 healingStart = pos;
		ImVec2 healingEnd = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * healingRatio, pos.y + ImGui::GetTextLineHeight());

		ImGui::GetWindowDrawList()->AddRectFilled(healingStart, healingEnd, healingColor);
	}

	// Add ItemInnerSpacing even if no box is being drawn, that way it looks consistent with and without progress bars
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x);
	if (pIndexNumber.has_value() == true)
	{
		indexNumberSize = ImGui::CalcTextSize(pIndexNumber->data()) + ImGui::GetStyle().ItemSpacing;

		TextColoredUnformatted(std::optional<ImU32>(IM_COL32(255, 255, 97, 255)), pIndexNumber->data());
		ImGui::SameLine();
	}

	/* pProfessionIcon can be nullptr when:
	* 1 - "profession icons" option is disabled in Display settings
	* 2 - There is no icon for the profession and elite specialization pair
	* 3 - IconLoader has not finished loading the icon yet
	*/
	if (pProfessionIcon != nullptr)
	{
		professionTextSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()) + ImGui::GetStyle().ItemSpacing;
		ImGui::Image(pProfessionIcon, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
		ImGui::SameLine();
	}

	if (pProfessionText.has_value() == true)
	{
		professionTextSize = ImGui::CalcTextSize(pProfessionText->data(), pProfessionText->data() + pProfessionText->size()) + ImGui::GetStyle().ItemSpacing;
		ImGui::TextUnformatted(pProfessionText->data(), pProfessionText->data() + pProfessionText->size());
		ImGui::SameLine();
	}

	auto leftTextColour = pLeftTextColour.has_value() ? ImGui::ColorConvertFloat4ToU32(*pLeftTextColour) : pSelf ? std::optional<ImU32>(IM_COL32(255, 255, 97, 255)) : std::nullopt;
	TextColoredUnformatted(leftTextColour, pLeftText.data(), pLeftText.data() + pLeftText.size());

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightTextSize.x - ImGui::GetStyle().ItemInnerSpacing.x); // Sending x in SameLine messes with alignment when inside of a group
	ImGui::TextUnformatted(pRightText.data(), pRightText.data() + pRightText.size());

	ImGui::EndGroup();
	ImGui::PopStyleColor();

	// index number - window padding - inner spacing - left text - item spacing * 3 - right text - inner spacing - window padding
	return indexNumberSize.x + leftTextSize.x + rightTextSize.x + professionTextSize.x + professionIconSize.x + ImGui::GetStyle().ItemSpacing.x * 2.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f + ImGui::GetCurrentWindowRead()->WindowPadding.x * 2.0f;
}

void ImGuiEx::TextColoredUnformatted(std::optional<ImU32> pColor, const char* pText, const char* pTextEnd)
{
	if (pColor.has_value())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, *pColor);
		ImGui::TextUnformatted(pText, pTextEnd);
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::TextUnformatted(pText, pTextEnd);
	}
}
