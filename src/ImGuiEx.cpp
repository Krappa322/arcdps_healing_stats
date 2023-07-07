#include "ImGuiEx.h"

float ImGuiEx::CalcWindowHeight(size_t pLineCount, ImGuiWindow* pWindow)
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
	return decorationsSize + padding + contentSize;
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

float ImGuiEx::StatsEntry(std::string_view pLeftText, std::string_view pRightText, std::optional<float> pFillRatio, std::optional<float> pBarrierRatio)
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(204, 204, 212, 255));
	ImGui::BeginGroup();

	//ImGui::PushID(pUniqueId);
	//float startX = ImGui::GetCursorPosX();
	//ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
	//ImGui::PopID();

	ImVec2 leftTextSize = ImGui::CalcTextSize(pLeftText.data(), pLeftText.data() + pLeftText.size());
	ImVec2 rightTextSize = ImGui::CalcTextSize(pRightText.data(), pRightText.data() + pRightText.size());

	if (pFillRatio.has_value() == true)
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();

		float healingRatio = *pFillRatio;

		if (pBarrierRatio.has_value() == true)
		{
			float barrierRatio = *pBarrierRatio;
			healingRatio -= barrierRatio;

			ImVec2 barrierStart = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * healingRatio, pos.y);
			ImVec2 barrierEnd = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * (healingRatio + barrierRatio), pos.y + ImGui::GetTextLineHeight());

			ImGui::GetWindowDrawList()->AddRectFilled(barrierStart, barrierEnd, IM_COL32(255, 225, 0, 128));
		}

		ImVec2 healingStart = pos;
		ImVec2 healingEnd = ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * healingRatio, pos.y + ImGui::GetTextLineHeight());

		ImGui::GetWindowDrawList()->AddRectFilled(healingStart, healingEnd, IM_COL32(102, 178, 102, 128));
	}

	// Add ItemInnerSpacing even if no box is being drawn, that way it looks consistent with and without progress bars
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::TextUnformatted(pLeftText.data(), pLeftText.data() + pLeftText.size());

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightTextSize.x - ImGui::GetStyle().ItemInnerSpacing.x); // Sending x in SameLine messes with alignment when inside of a group
	ImGui::TextUnformatted(pRightText.data(), pRightText.data() + pRightText.size());

	ImGui::EndGroup();
	ImGui::PopStyleColor();

	// window padding - inner spacing - left text - item spacing * 2 - right text - inner spacing - window padding
	return leftTextSize.x + rightTextSize.x + ImGui::GetStyle().ItemSpacing.x * 2.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f + ImGui::GetCurrentWindowRead()->WindowPadding.x * 2.0f;
}