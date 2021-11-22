#include "ImGuiEx.h"

namespace
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
}; // anonymous namespace

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

void ImGuiEx::StatsEntry(const char* pLeftText, const char* pRightText, std::optional<float> pFillRatio)
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(204, 204, 212, 255));
	ImGui::BeginGroup();

	//ImGui::PushID(pUniqueId);
	//float startX = ImGui::GetCursorPosX();
	//ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
	//ImGui::PopID();

	if (pFillRatio.has_value() == true)
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + ImGui::GetContentRegionAvailWidth() * *pFillRatio, pos.y + ImGui::GetTextLineHeight()), IM_COL32(102, 178, 102, 128));
	}

	// Add ItemInnerSpacing even if no box is being drawn, that way it looks consistent with and without progress bars
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::TextUnformatted(pLeftText);

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(pRightText).x - ImGui::GetStyle().ItemInnerSpacing.x); // Sending x in SameLine messes with alignment when inside of a group
	ImGui::TextUnformatted(pRightText);

	ImGui::EndGroup();
	ImGui::PopStyleColor();
}