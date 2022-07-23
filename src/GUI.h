#pragma once
#include "Options.h"
#include "UpdateGUI.h"

#include <Windows.h>

void SetContext(void* pImGuiContext);
void FindAndResolveCyclicDependencies(HealTableOptions& pHealingOptions, size_t pStartIndex);

void Display_GUI(HealTableOptions& pHealingOptions);
void Display_AddonOptions(HealTableOptions& pHealingOptions);
void Display_PostNewFrame(ImGuiContext* pImguiContext, HealTableOptions& pHealingOptions);
void Display_PreEndFrame(ImGuiContext* pImguiContext, HealTableOptions& pHealingOptions);
void ImGui_ProcessKeyEvent(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL);