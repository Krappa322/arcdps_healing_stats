/*
* arcdps combat api example
*/

#include "ArcDPS.h"
#include "GUI.h"
#include "Log.h"
#include "PersonalStats.h"
#include "Skills.h"
#include "Utilities.h"

#include "imgui/imgui.h"

#include <atomic>

#include <assert.h>
#include <d3d9helper.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

/* proto/globals */
uint32_t cbtcount = 0;
arcdps_exports arc_exports;
char* arcvers;

void dll_init(HANDLE hModule);
void dll_exit();
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext, IDirect3DDevice9 * id3dd9, HMODULE pArcModule);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_imgui(uint32_t not_charsel_or_loading);
uintptr_t mod_options_end();
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision);
uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision);
uintptr_t mod_wnd(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL);

std::mutex HEAL_TABLE_OPTIONS_MUTEX;
static HealTableOptions HEAL_TABLE_OPTIONS;

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
	switch (ulReasonForCall) {
	case DLL_PROCESS_ATTACH: dll_init(hModule); break;
	case DLL_PROCESS_DETACH: dll_exit(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* dll attach -- from winapi */
void dll_init(HANDLE hModule) {
	return;
}

/* dll detach -- from winapi */
void dll_exit() {
	return;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext, IDirect3DDevice9 * id3dd9, HMODULE pArcModule)
{
	ARC_E7 = reinterpret_cast<E7Signature>(GetProcAddress(pArcModule, "e7"));
	assert(ARC_E7 != nullptr);
	ARC_E3 = reinterpret_cast<E3Signature>(GetProcAddress(pArcModule, "e3"));
	assert(ARC_E3 != nullptr);

	arcvers = arcversionstr;
	SetContext(imguicontext);

	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
	arcvers = 0;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init() {
#ifdef DEBUG
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);

	/* big buffer */
	char buff[4096];
	char* p = &buff[0];
	p += _snprintf(p, 400, "==== mod_init ====\n");
	p += _snprintf(p, 400, "arcdps: %s\n", arcvers);

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], (DWORD)(p - &buff[0]), &written, 0);
#endif // DEBUG

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		ReadIni(HEAL_TABLE_OPTIONS);
	}

	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = 0x9c9b3c99;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "healing_stats";
	arc_exports.out_build = "0.7";
	arc_exports.combat = mod_combat;
	arc_exports.imgui = mod_imgui;
	arc_exports.options_end = mod_options_end;
	arc_exports.combat_local = mod_combat_local;
	arc_exports.wnd_nofilter = mod_wnd;
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		WriteIni(HEAL_TABLE_OPTIONS);
	}

#ifdef DEBUG
	FreeConsole();
#endif

	return 0;
}

uintptr_t mod_imgui(uint32_t pNotCharSelOrLoading)
{
	if (pNotCharSelOrLoading == false)
	{
		return false;
	}

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		Display_GUI(HEAL_TABLE_OPTIONS);
	}

	return 0;
}

uintptr_t mod_options_end()
{
	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		Display_ArcDpsOptions(HEAL_TABLE_OPTIONS);
	}

	return 0;
}

static std::atomic<uint32_t> SELF_INSTANCE_ID = UINT32_MAX;
/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pEvent == nullptr)
	{
		if (pSourceAgent->elite != 0)
		{
			// Not agent adding event, uninteresting
			return 0;
		}

		if (pSourceAgent->prof != 0)
		{
			LOG("Register agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			// Assume that the agent is not a minion. If it is a minion then we will find out once it enters combat
			PersonalStats::GlobalState.AddAgent(pSourceAgent->id, pSourceAgent->name, pDestinationAgent->team, false);

			if (pDestinationAgent->self != 0)
			{
				assert(pDestinationAgent->id <= UINT16_MAX);

				LOG("Storing self instance id %llu", pDestinationAgent->id);
				SELF_INSTANCE_ID.store(static_cast<uint16_t>(pDestinationAgent->id), std::memory_order_relaxed);
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			if (pDestinationAgent->id == SELF_INSTANCE_ID.load(std::memory_order_relaxed))
			{
				LOG("Exiting combat since self agent was deregistered");
				PersonalStats::GlobalState.ExitedCombat(timeGetTime());
				return 0;
			}
		}

		return 0;
	}

	if (pEvent->is_statechange == CBTS_ENTERCOMBAT)
	{
		if (pSourceAgent->self != 0)
		{
			PersonalStats::GlobalState.EnteredCombat(pEvent->time, static_cast<uint16_t>(pEvent->dst_agent));
		}

		bool isMinion = false;
		if (pEvent->src_master_instid != 0)
		{
			isMinion = true;
		}
		PersonalStats::GlobalState.AddAgent(pSourceAgent->id, pSourceAgent->name, static_cast<uint16_t>(pEvent->dst_agent), isMinion);

		return 0;
	}
	else if (pEvent->is_statechange == CBTS_EXITCOMBAT)
	{
		if (pSourceAgent->self != 0)
		{
			PersonalStats::GlobalState.ExitedCombat(pEvent->time);
			return 0;
		}
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		return 0;
	}

	if (pEvent->buff != 0 && pEvent->buff_dmg == 0)
	{
		// Buff application - not interesting
		return 0;
	}

	// Event actually did something
	if (pEvent->buff_dmg > 0 || pEvent->value > 0)
	{
		SkillTable::GlobalState.RegisterDamagingSkill(pEvent->skillid, pSkillname);
		return 0;
	}

	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pEvent == nullptr)
	{
		// Agent notification - not interesting
		return 0;
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		// Not a HP modifying event - not interesting
		return 0;
	}

	if (pSourceAgent->self == 0 &&
		pEvent->src_master_instid != SELF_INSTANCE_ID.load(std::memory_order_relaxed))
	{
		// Source is someone else - not interesting
		return 0;
	}

	if (pEvent->value <= 0 && pEvent->buff_dmg <= 0)
	{
		// Not healing - not interesting
		return 0;
	}

	if (pEvent->is_shields != 0)
	{
		// Shield application - not tracking for now
		return 0;
	}

	bool isMinion = (pEvent->dst_master_instid != 0);
	PersonalStats::GlobalState.HealingEvent(pEvent, pDestinationAgent->id, pDestinationAgent->name, isMinion, SkillTable::GlobalState.GetSkillName(pEvent->skillid, pSkillname));
	return 0;
}

#pragma pack(push, 1)
struct ArcModifiers
{
	uint16_t _1;
	uint16_t _2;
	uint16_t Multi;
};
#pragma pack(pop)

/* window callback -- return is assigned to umsg (return zero to not be processed by arcdps or game) */
uintptr_t mod_wnd(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL)
{
	ImGui_ProcessKeyEvent(pWindowHandle, pMessage, pAdditionalW, pAdditionalL);

	const ImGuiIO& io = ImGui::GetIO();

	if (pMessage == WM_KEYDOWN || pMessage == WM_SYSKEYDOWN)
	{
		int virtualKey = static_cast<int>(pAdditionalW);

		uint64_t e7_rawResult = ARC_E7();
		ArcModifiers modifiers;
		memcpy(&modifiers, &e7_rawResult, sizeof(modifiers));

		if ((modifiers._1 == 0 || io.KeysDown[modifiers._1] == true) &&
			(modifiers._2 == 0 || io.KeysDown[modifiers._2] == true))
		{
			std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);

			if (HEAL_TABLE_OPTIONS.HealTableHotkey > 0 &&
				HEAL_TABLE_OPTIONS.HealTableHotkey < sizeof(io.KeysDown) &&
				virtualKey == HEAL_TABLE_OPTIONS.HealTableHotkey)
			{
				assert(io.KeysDown[HEAL_TABLE_OPTIONS.HealTableHotkey] == true);

				HEAL_TABLE_OPTIONS.ShowHealWindow = !HEAL_TABLE_OPTIONS.ShowHealWindow;

				LOG("Key %i '%s' toggled window - new heal window state is %s", HEAL_TABLE_OPTIONS.HealTableHotkey, VirtualKeyToString(HEAL_TABLE_OPTIONS.HealTableHotkey).c_str(), BOOL_STR(HEAL_TABLE_OPTIONS.ShowHealWindow));

				return 0; // Don't process message by arcdps or game
			}
		}
	}

	return pMessage;
}