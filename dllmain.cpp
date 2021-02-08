/*
* arcdps combat api example
*/

#include "ArcDPS.h"
#include "GUI.h"
#include "Log.h"
#include "PersonalStats.h"
#include "Skills.h"

#include "imgui/imgui.h"

#include <atomic>

#include <assert.h>
#include <d3d9helper.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

typedef void* (*MallocSignature)(size_t);
typedef void (*FreeSignature)(void*);

/* proto/globals */
void dll_init(HANDLE pModule);
void dll_exit();
extern "C" __declspec(dllexport) void* get_init_addr(char* pArcdpsVersionString, void* pImguiContext, IDirect3DDevice9* pUnused, HANDLE pUnused2, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_imgui(uint32_t pNotCharselOrLoading);
uintptr_t mod_options_end();
uintptr_t mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname, uint64_t pId, uint64_t pRevision);
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname, uint64_t pId, uint64_t pRevision);

static MallocSignature ARCDPS_MALLOC = nullptr;
static FreeSignature ARCDPS_FREE = nullptr;
static arcdps_exports ARC_EXPORTS;
static char* ARCDPS_VERSION;

std::mutex HEAL_TABLE_OPTIONS_MUTEX;
static HealTableOptions HEAL_TABLE_OPTIONS;

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE pModule, DWORD pReasonForCall, LPVOID pReserved)
{
	switch (pReasonForCall) {
	case DLL_PROCESS_ATTACH: dll_init(pModule); break;
	case DLL_PROCESS_DETACH: dll_exit(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* dll attach -- from winapi */
void dll_init(HANDLE pModule)
{
	return;
}

/* dll detach -- from winapi */
void dll_exit()
{
	return;
}

static void* MallocWrapper(size_t pSize, void* pUserData)
{
	return ARCDPS_MALLOC(pSize);
}

static void FreeWrapper(void* pPointer, void* pUserData)
{
	ARCDPS_FREE(pPointer);
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* pArcdpsVersionString, void* pImguiContext, IDirect3DDevice9* pUnused, HANDLE pUnused2, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree)
{
	ARCDPS_VERSION = pArcdpsVersionString;
	SetContext(pImguiContext);

	ARCDPS_MALLOC = pArcdpsMalloc;
	ARCDPS_FREE = pArcdpsFree;
	ImGui::SetAllocatorFunctions(MallocWrapper, FreeWrapper);

	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr()
{
	ARCDPS_VERSION = nullptr;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init()
{
#ifdef DEBUG
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);

	/* big buffer */
	char buff[4096];
	char* p = &buff[0];
	p += _snprintf(p, 400, "==== mod_init ====\n");
	p += _snprintf(p, 400, "arcdps: %s\n", ARCDPS_VERSION);

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], (DWORD)(p - &buff[0]), &written, 0);
#endif // DEBUG

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		ReadIni(HEAL_TABLE_OPTIONS);
	}

	memset(&ARC_EXPORTS, 0, sizeof(arcdps_exports));
	ARC_EXPORTS.sig = 0x9c9b3c99;
	ARC_EXPORTS.imguivers = IMGUI_VERSION_NUM;
	ARC_EXPORTS.size = sizeof(arcdps_exports);
	ARC_EXPORTS.out_name = "healing_stats";
	ARC_EXPORTS.out_build = "0.4-imgui180";
	ARC_EXPORTS.combat = mod_combat;
	ARC_EXPORTS.imgui = mod_imgui;
	ARC_EXPORTS.options_end = mod_options_end;
	ARC_EXPORTS.combat_local = mod_combat_local;
	return &ARC_EXPORTS;
}

/* release mod -- return ignored */
uintptr_t mod_release()
{
#ifdef DEBUG
	FreeConsole();
#endif

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		WriteIni(HEAL_TABLE_OPTIONS);
	}

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

static std::atomic<uint16_t> SELF_INSTANCE_ID = (uint16_t)0;
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
		RegisterDamagingSkill(pEvent->skillid, pSkillname);
		return 0;
	}

	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname, uint64_t pId, uint64_t pRevision)
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

	PersonalStats::GlobalState.HealingEvent(pEvent, pDestinationAgent->id, pDestinationAgent->name, pSkillname);
	return 0;
}
