#include "ImGuiEx.h"

float ImGuiEx::CalcWindowHeight(size_t pLineCount, float pExtraHeight, ImGuiWindow* pWindow)
{
	if (pWindow == nullptr)
	{
		pWindow = ImGui::GetCurrentWindowRead();
	}

	//return pWindow->TitleBarHeight() + ImGui::GetStyle().WindowPadding.y * 2 + pLineCount * ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;

	float decorationsSize = pWindow->TitleBarHeight + pWindow->MenuBarHeight;
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

			ImVec2 barrierStart = ImVec2(pos.x + ImGui::GetContentRegionAvail().x * healingRatio, pos.y);
			ImVec2 barrierEnd = ImVec2(pos.x + ImGui::GetContentRegionAvail().x * (healingRatio + barrierGenerationRatio), pos.y + ImGui::GetTextLineHeight());

			ImGui::GetWindowDrawList()->AddRectFilled(barrierStart, barrierEnd, barrierColor);
		}

		ImVec2 healingStart = pos;
		ImVec2 healingEnd = ImVec2(pos.x + ImGui::GetContentRegionAvail().x * healingRatio, pos.y + ImGui::GetTextLineHeight());

		ImGui::GetWindowDrawList()->AddRectFilled(healingStart, healingEnd, healingColor);
	}

	// Add ItemInnerSpacing even if no box is being drawn, that way it looks consistent with and without progress bars
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x);
	if (pIndexNumber.has_value() == true)
	{
		indexNumberSize = ImGui::CalcTextSize(pIndexNumber->data(), pIndexNumber->data() + pIndexNumber->size()) + ImGui::GetStyle().ItemSpacing;

		TextColoredUnformatted(std::optional<ImU32>(IM_COL32(255, 255, 97, 255)), *pIndexNumber);
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
	TextColoredUnformatted(leftTextColour, pLeftText);

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightTextSize.x - ImGui::GetStyle().ItemInnerSpacing.x); // Sending x in SameLine messes with alignment when inside of a group
	ImGui::TextUnformatted(pRightText.data(), pRightText.data() + pRightText.size());

	ImGui::EndGroup();
	ImGui::PopStyleColor();

	// one window padding and inner spacing on each edge of the window, two item spacings between left and right text. The other item spacings are accounted for into their respective size calculations already.
	return indexNumberSize.x + leftTextSize.x + rightTextSize.x + professionTextSize.x + professionIconSize.x + ImGui::GetStyle().ItemSpacing.x * 2.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f + ImGui::GetCurrentWindowRead()->WindowPadding.x * 2.0f;
}

void ImGuiEx::TextColoredUnformatted(std::optional<ImU32> pColor, std::string_view pText)
{
	if (pColor.has_value())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, *pColor);
		ImGui::TextUnformatted(pText.data(), pText.data() + pText.size());
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::TextUnformatted(pText.data(), pText.data() + pText.size());
	}
}

ImGuiKey ImGuiEx::ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam)
{
    // There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED.
    if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
        return ImGuiKey_KeypadEnter;

    switch (wParam)
    {
    case VK_SHIFT: return ImGuiKey_LeftShift;
    case VK_MENU: return ImGuiKey_LeftAlt;
    case VK_CONTROL: return ImGuiKey_LeftCtrl;
    }

    const int scancode = (int)LOBYTE(HIWORD(lParam));
    //IMGUI_DEBUG_LOG("scancode %3d, keycode = 0x%02X\n", scancode, wParam);
    switch (wParam)
    {
    case VK_TAB: return ImGuiKey_Tab;
    case VK_LEFT: return ImGuiKey_LeftArrow;
    case VK_RIGHT: return ImGuiKey_RightArrow;
    case VK_UP: return ImGuiKey_UpArrow;
    case VK_DOWN: return ImGuiKey_DownArrow;
    case VK_PRIOR: return ImGuiKey_PageUp;
    case VK_NEXT: return ImGuiKey_PageDown;
    case VK_HOME: return ImGuiKey_Home;
    case VK_END: return ImGuiKey_End;
    case VK_INSERT: return ImGuiKey_Insert;
    case VK_DELETE: return ImGuiKey_Delete;
    case VK_BACK: return ImGuiKey_Backspace;
    case VK_SPACE: return ImGuiKey_Space;
    case VK_RETURN: return ImGuiKey_Enter;
    case VK_ESCAPE: return ImGuiKey_Escape;
        //case VK_OEM_7: return ImGuiKey_Apostrophe;
    case VK_OEM_COMMA: return ImGuiKey_Comma;
        //case VK_OEM_MINUS: return ImGuiKey_Minus;
    case VK_OEM_PERIOD: return ImGuiKey_Period;
        //case VK_OEM_2: return ImGuiKey_Slash;
        //case VK_OEM_1: return ImGuiKey_Semicolon;
        //case VK_OEM_PLUS: return ImGuiKey_Equal;
        //case VK_OEM_4: return ImGuiKey_LeftBracket;
        //case VK_OEM_5: return ImGuiKey_Backslash;
        //case VK_OEM_6: return ImGuiKey_RightBracket;
        //case VK_OEM_3: return ImGuiKey_GraveAccent;
    case VK_CAPITAL: return ImGuiKey_CapsLock;
    case VK_SCROLL: return ImGuiKey_ScrollLock;
    case VK_NUMLOCK: return ImGuiKey_NumLock;
    case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
    case VK_PAUSE: return ImGuiKey_Pause;
    case VK_NUMPAD0: return ImGuiKey_Keypad0;
    case VK_NUMPAD1: return ImGuiKey_Keypad1;
    case VK_NUMPAD2: return ImGuiKey_Keypad2;
    case VK_NUMPAD3: return ImGuiKey_Keypad3;
    case VK_NUMPAD4: return ImGuiKey_Keypad4;
    case VK_NUMPAD5: return ImGuiKey_Keypad5;
    case VK_NUMPAD6: return ImGuiKey_Keypad6;
    case VK_NUMPAD7: return ImGuiKey_Keypad7;
    case VK_NUMPAD8: return ImGuiKey_Keypad8;
    case VK_NUMPAD9: return ImGuiKey_Keypad9;
    case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
    case VK_DIVIDE: return ImGuiKey_KeypadDivide;
    case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case VK_ADD: return ImGuiKey_KeypadAdd;
    case VK_LSHIFT: return ImGuiKey_LeftShift;
    case VK_LCONTROL: return ImGuiKey_LeftCtrl;
    case VK_LMENU: return ImGuiKey_LeftAlt;
    case VK_LWIN: return ImGuiKey_LeftSuper;
    case VK_RSHIFT: return ImGuiKey_RightShift;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_RMENU: return ImGuiKey_RightAlt;
    case VK_RWIN: return ImGuiKey_RightSuper;
    case VK_APPS: return ImGuiKey_Menu;
    case '0': return ImGuiKey_0;
    case '1': return ImGuiKey_1;
    case '2': return ImGuiKey_2;
    case '3': return ImGuiKey_3;
    case '4': return ImGuiKey_4;
    case '5': return ImGuiKey_5;
    case '6': return ImGuiKey_6;
    case '7': return ImGuiKey_7;
    case '8': return ImGuiKey_8;
    case '9': return ImGuiKey_9;
    case 'A': return ImGuiKey_A;
    case 'B': return ImGuiKey_B;
    case 'C': return ImGuiKey_C;
    case 'D': return ImGuiKey_D;
    case 'E': return ImGuiKey_E;
    case 'F': return ImGuiKey_F;
    case 'G': return ImGuiKey_G;
    case 'H': return ImGuiKey_H;
    case 'I': return ImGuiKey_I;
    case 'J': return ImGuiKey_J;
    case 'K': return ImGuiKey_K;
    case 'L': return ImGuiKey_L;
    case 'M': return ImGuiKey_M;
    case 'N': return ImGuiKey_N;
    case 'O': return ImGuiKey_O;
    case 'P': return ImGuiKey_P;
    case 'Q': return ImGuiKey_Q;
    case 'R': return ImGuiKey_R;
    case 'S': return ImGuiKey_S;
    case 'T': return ImGuiKey_T;
    case 'U': return ImGuiKey_U;
    case 'V': return ImGuiKey_V;
    case 'W': return ImGuiKey_W;
    case 'X': return ImGuiKey_X;
    case 'Y': return ImGuiKey_Y;
    case 'Z': return ImGuiKey_Z;
    case VK_F1: return ImGuiKey_F1;
    case VK_F2: return ImGuiKey_F2;
    case VK_F3: return ImGuiKey_F3;
    case VK_F4: return ImGuiKey_F4;
    case VK_F5: return ImGuiKey_F5;
    case VK_F6: return ImGuiKey_F6;
    case VK_F7: return ImGuiKey_F7;
    case VK_F8: return ImGuiKey_F8;
    case VK_F9: return ImGuiKey_F9;
    case VK_F10: return ImGuiKey_F10;
    case VK_F11: return ImGuiKey_F11;
    case VK_F12: return ImGuiKey_F12;
    case VK_F13: return ImGuiKey_F13;
    case VK_F14: return ImGuiKey_F14;
    case VK_F15: return ImGuiKey_F15;
    case VK_F16: return ImGuiKey_F16;
    case VK_F17: return ImGuiKey_F17;
    case VK_F18: return ImGuiKey_F18;
    case VK_F19: return ImGuiKey_F19;
    case VK_F20: return ImGuiKey_F20;
    case VK_F21: return ImGuiKey_F21;
    case VK_F22: return ImGuiKey_F22;
    case VK_F23: return ImGuiKey_F23;
    case VK_F24: return ImGuiKey_F24;
    case VK_BROWSER_BACK: return ImGuiKey_AppBack;
    case VK_BROWSER_FORWARD: return ImGuiKey_AppForward;
    default: break;
    }

    // Fallback to scancode
    // https://handmade.network/forums/t/2011-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names
    switch (scancode)
    {
    case 41: return ImGuiKey_GraveAccent;  // VK_OEM_8 in EN-UK, VK_OEM_3 in EN-US, VK_OEM_7 in FR, VK_OEM_5 in DE, etc.
    case 12: return ImGuiKey_Minus;
    case 13: return ImGuiKey_Equal;
    case 26: return ImGuiKey_LeftBracket;
    case 27: return ImGuiKey_RightBracket;
    case 86: return ImGuiKey_Oem102;
    case 43: return ImGuiKey_Backslash;
    case 39: return ImGuiKey_Semicolon;
    case 40: return ImGuiKey_Apostrophe;
    case 51: return ImGuiKey_Comma;
    case 52: return ImGuiKey_Period;
    case 53: return ImGuiKey_Slash;
    default: break;
    }

    return ImGuiKey_None;
}
