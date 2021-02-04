/*
* arcdps combat api example
*/

#include "ArcDPS.h"
#include "GUI.h"
#include "Log.h"
#include "PersonalStats.h"

#include <atomic>

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
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext, IDirect3DDevice9 * id3dd9);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_imgui(uint32_t not_charsel_or_loading);
uintptr_t mod_options_end();
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);
uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision);

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
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext, IDirect3DDevice9 * id3dd9) {
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

	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = 0xFFFA;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "healing_stats";
	arc_exports.out_build = "0.2";
	arc_exports.combat = mod_combat;
	arc_exports.imgui = mod_imgui;
	arc_exports.options_end = mod_options_end;
	arc_exports.combat_local = mod_combat_local;
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
#ifdef DEBUG
	FreeConsole();
#endif

	return 0;
}

uintptr_t mod_imgui(uint32_t pNotCharselOrLoading)
{
	if (pNotCharselOrLoading == false)
	{
		return false;
	}

	Display_GUI();
	return 0;
}

uintptr_t mod_options_end()
{
	Display_ArcDpsOptions();
	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname, uint64_t pId, uint64_t pRevision)
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
			// Assume that the agent is not a minion. If it is a minion then we will find out once it enters combat
			PersonalStats::GlobalState.AddAgent(pSourceAgent->id, pSourceAgent->name, pDestinationAgent->team, false);
		}
		else
		{
			// We could remove agents here but that would make tracking tricky (for example, minions could die during combat or
			// someone could leave the instance).
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

	return 0;
}

static std::atomic<uint16_t> SELF_INSTANCE_ID = (uint16_t)66634;
/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pEvent == nullptr)
	{
		// Agent notification - not interesting
		return 0;
	}

	if (pSourceAgent->self == 1)
	{
		// Source is self - note down our instid
		SELF_INSTANCE_ID.store(pEvent->src_instid, std::memory_order_relaxed);
	}
	else if (pEvent->src_master_instid != SELF_INSTANCE_ID.load(std::memory_order_relaxed))
	{
		// Source is someone else - not interesting
		return 0;
	}
	else
	{
		//LOG("Source is our minion");
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		// Not a HP modifying event - not interesting
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

	PersonalStats::GlobalState.HealingEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname);
	return 0;
}