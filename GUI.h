#pragma once

#include "Options.h"

#include <Windows.h>

void SetContext(void* pImGuiContext);
void Display_GUI(HealTableOptions& pHealingOptions);
void Display_ArcDpsOptions(HealTableOptions& pHealingOptions);
void ImGui_ProcessKeyEvent(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL);