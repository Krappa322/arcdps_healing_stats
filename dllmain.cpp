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
	/* demo */
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

	/* for arcdps */
	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = 0xFFFA;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "personal_stats";
	arc_exports.out_build = "0.1";
	arc_exports.combat = mod_combat;
	arc_exports.imgui = mod_imgui;
	arc_exports.options_end = mod_options_end;
	arc_exports.combat_local = mod_combat_local;
	//arc_exports.size = (uintptr_t)"error message if you decide to not load, sig must be 0";
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
	FreeConsole();
	return 0;
}

uintptr_t mod_imgui(uint32_t not_charsel_or_loading)
{
	if (not_charsel_or_loading == false)
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

	/*
	if (pSkillname != nullptr && strcmp(pSkillname, "Regeneration") == 0)
	{
		if (!pSourceAgent->name || !strlen(pSourceAgent->name)) pSourceAgent->name = "(area)";
		if (!pDestinationAgent->name || !strlen(pDestinationAgent->name)) pDestinationAgent->name = "(area)";

		char buffer[1024 * 16];
		char* p = buffer;

		p += _snprintf(p, 400, "==== cbtevent AREA %u at %llu ====\n", cbtcount, pEvent->time);
		p += _snprintf(p, 400, "source agent: %s (%0llx:%u, %lx:%lx:%hu), master: %u\n", pSourceAgent->name, pEvent->src_agent, pEvent->src_instid, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pEvent->src_master_instid);
		if (pEvent->dst_agent) p += _snprintf(p, 400, "target agent: %s (%0llx:%u, %lx:%lx:%hu)\n", pDestinationAgent->name, pEvent->dst_agent, pEvent->dst_instid, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team);
		else p += _snprintf(p, 400, "target agent: n/a\n");

		p += _snprintf(p, 400, "is_buff: %u\n", pEvent->buff);
		p += _snprintf(p, 400, "skill: %s:%u\n", pSkillname, pEvent->skillid);
		p += _snprintf(p, 400, "dmg: %d\n", pEvent->value);
		p += _snprintf(p, 400, "buff_dmg: %d\n", pEvent->buff_dmg);
		p += _snprintf(p, 400, "is_moving: %u\n", pEvent->is_moving);
		p += _snprintf(p, 400, "is_ninety: %u\n", pEvent->is_ninety);
		p += _snprintf(p, 400, "is_flanking: %u\n", pEvent->is_flanking);
		p += _snprintf(p, 400, "is_shields: %u\n", pEvent->is_shields);

		p += _snprintf(p, 400, "iff: %u\n", pEvent->iff);
		p += _snprintf(p, 400, "result: %u\n", pEvent->result);
		cbtcount += 1;

		DWORD written = 0;
		p[0] = 0;
		wchar_t buffw[1024 * 16];
		int32_t rc = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, buffw, 1024 * 16);
		if (rc == 0)
		{
			char errString[2048];
			snprintf(errString, sizeof(errString), "MultiByteToWideChar failed, rc==%i error==%u", rc, GetLastError());
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), errString, strlen(errString), &written, nullptr);

			return 0;
		}

		buffw[rc] = 0;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffw, (DWORD)lstrlenW(buffw), &written, nullptr);
	}*/

	// Not interesting
	return 0;
}

#ifdef false
/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat_local(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) {
	/* big buffer */
	char buff[1024 * 16];
	char* p = &buff[0];

	/* ev is null. dst will only be valid on tracking add. skillname will also be null */
	if (!ev) {

		/* notify tracking change */
		if (!src->elite) {

			/* add */
			if (src->prof) {
				p += _snprintf(p, 400, "==== cbtnotify ====\n");
				p += _snprintf(p, 400, "agent added: %s:%s (%0llx), instid: %llu, prof: %u, elite: %u, self: %u, team: %u, subgroup: %u\n", src->name, dst->name, src->id, dst->id, dst->prof, dst->elite, dst->self, src->team, dst->team);
			}

			/* remove */
			else {
				p += _snprintf(p, 400, "==== cbtnotify ====\n");
				p += _snprintf(p, 400, "agent removed: %s (%0llx)\n", src->name, src->id);
			}
		}

		/* notify target change */
		else if (src->elite == 1) {
			p += _snprintf(p, 400, "==== cbtnotify ====\n");
			p += _snprintf(p, 400, "new target: %0llx\n", src->id);
		}
	}

	/* combat event. skillname may be null. non-null skillname will remain static until module is unloaded. refer to evtc notes for complete detail */
	else {

		/* default names */
		if (!src->name || !strlen(src->name)) src->name = "(area)";
		if (!dst->name || !strlen(dst->name)) dst->name = "(area)";

		/* common */
		p += _snprintf(p, 400, "==== cbtevent %u at %llu ====\n", cbtcount, ev->time);
		p += _snprintf(p, 400, "source agent: %s (%0llx:%u, %lx:%lx), master: %u\n", src->name, ev->src_agent, ev->src_instid, src->prof, src->elite, ev->src_master_instid);
		if (ev->dst_agent) p += _snprintf(p, 400, "target agent: %s (%0llx:%u, %lx:%lx)\n", dst->name, ev->dst_agent, ev->dst_instid, dst->prof, dst->elite);
		else p += _snprintf(p, 400, "target agent: n/a\n");

		/* statechange */
		if (ev->is_statechange) {
			p += _snprintf(p, 400, "is_statechange: %u\n", ev->is_statechange);
		}

		/* activation */
		else if (ev->is_activation) {
			p += _snprintf(p, 400, "is_activation: %u\n", ev->is_activation);
			p += _snprintf(p, 400, "skill: %s:%u\n", skillname, ev->skillid);
			p += _snprintf(p, 400, "ms_expected: %d\n", ev->value);
		}

		/* buff remove */
		else if (ev->is_buffremove) {
			p += _snprintf(p, 400, "is_buffremove: %u\n", ev->is_buffremove);
			p += _snprintf(p, 400, "skill: %s:%u\n", skillname, ev->skillid);
			p += _snprintf(p, 400, "ms_duration: %d\n", ev->value);
			p += _snprintf(p, 400, "ms_intensity: %d\n", ev->buff_dmg);
		}

		/* buff */
		else if (ev->buff) {

			/* damage */
			if (ev->buff_dmg) {
				p += _snprintf(p, 400, "is_buff: %u\n", ev->buff);
				p += _snprintf(p, 400, "skill: %s:%u\n", skillname, ev->skillid);
				p += _snprintf(p, 400, "dmg: %d\n", ev->buff_dmg);
				p += _snprintf(p, 400, "is_shields: %u\n", ev->is_shields);
			}

			/* application */
			else {
				p += _snprintf(p, 400, "is_buff: %u\n", ev->buff);
				p += _snprintf(p, 400, "skill: %s:%u\n", skillname, ev->skillid);
				p += _snprintf(p, 400, "raw ms: %d\n", ev->value);
				p += _snprintf(p, 400, "overstack ms: %u\n", ev->overstack_value);
			}
		}

		/* physical */
		else {
			p += _snprintf(p, 400, "is_buff: %u\n", ev->buff);
			p += _snprintf(p, 400, "skill: %s:%u\n", skillname, ev->skillid);
			p += _snprintf(p, 400, "dmg: %d\n", ev->value);
			p += _snprintf(p, 400, "is_moving: %u\n", ev->is_moving);
			p += _snprintf(p, 400, "is_ninety: %u\n", ev->is_ninety);
			p += _snprintf(p, 400, "is_flanking: %u\n", ev->is_flanking);
			p += _snprintf(p, 400, "is_shields: %u\n", ev->is_shields);
		}

		/* common */
		p += _snprintf(p, 400, "iff: %u\n", ev->iff);
		p += _snprintf(p, 400, "result: %u\n", ev->result);
		cbtcount += 1;
	}

	/* print */
	DWORD written = 0;
	p[0] = 0;
	wchar_t buffw[1024*16];
	int32_t rc = MultiByteToWideChar(CP_UTF8, 0, buff, -1, buffw, 1024 * 16);
	if (rc == 0)
	{
		char errString[2048];
		snprintf(errString, sizeof(errString), "MultiByteToWideChar failed, rc==%i error==%u", rc, GetLastError());
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), errString, strlen(errString), &written, nullptr);

		return 0;
	}

	buffw[rc] = 0;
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffw, (DWORD)lstrlenW(buffw), &written, nullptr);

	return 0;
}
#endif

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

	/*
	if (pSkillname != nullptr && strcmp(pSkillname, "Regeneration") == 0)
	{
		if (!pSourceAgent->name || !strlen(pSourceAgent->name)) pSourceAgent->name = "(area)";
		if (!pDestinationAgent->name || !strlen(pDestinationAgent->name)) pDestinationAgent->name = "(area)";

		char buffer[1024 * 16];
		char* p = buffer;

		p += _snprintf(p, 400, "==== cbtevent LOCAL %u at %llu ====\n", cbtcount, pEvent->time);
		p += _snprintf(p, 400, "source agent: %s (%0llx:%u, %lx:%lx:%hu), master: %u\n", pSourceAgent->name, pEvent->src_agent, pEvent->src_instid, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pEvent->src_master_instid);
		if (pEvent->dst_agent) p += _snprintf(p, 400, "target agent: %s (%0llx:%u, %lx:%lx:%hu)\n", pDestinationAgent->name, pEvent->dst_agent, pEvent->dst_instid, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team);
		else p += _snprintf(p, 400, "target agent: n/a\n");

		p += _snprintf(p, 400, "is_buff: %u\n", pEvent->buff);
		p += _snprintf(p, 400, "skill: %s:%u\n", pSkillname, pEvent->skillid);
		p += _snprintf(p, 400, "dmg: %d\n", pEvent->value);
		p += _snprintf(p, 400, "buff_dmg: %d\n", pEvent->buff_dmg);
		p += _snprintf(p, 400, "is_moving: %u\n", pEvent->is_moving);
		p += _snprintf(p, 400, "is_ninety: %u\n", pEvent->is_ninety);
		p += _snprintf(p, 400, "is_flanking: %u\n", pEvent->is_flanking);
		p += _snprintf(p, 400, "is_shields: %u\n", pEvent->is_shields);

		p += _snprintf(p, 400, "iff: %u\n", pEvent->iff);
		p += _snprintf(p, 400, "result: %u\n", pEvent->result);
		cbtcount += 1;

		DWORD written = 0;
		p[0] = 0;
		wchar_t buffw[1024 * 16];
		int32_t rc = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, buffw, 1024 * 16);
		if (rc == 0)
		{
			char errString[2048];
			snprintf(errString, sizeof(errString), "MultiByteToWideChar failed, rc==%i error==%u", rc, GetLastError());
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), errString, strlen(errString), &written, nullptr);

			return 0;
		}

		buffw[rc] = 0;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffw, (DWORD)lstrlenW(buffw), &written, nullptr);
	}*/

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

	/*
	if (!pSourceAgent->name || !strlen(pSourceAgent->name)) pSourceAgent->name = "(area)";
	if (!pDestinationAgent->name || !strlen(pDestinationAgent->name)) pDestinationAgent->name = "(area)";

	char buffer[1024 * 16];
	char* p = buffer;

	p += _snprintf(p, 400, "==== cbtevent %u at %llu ====\n", cbtcount, pEvent->time);
	p += _snprintf(p, 400, "source agent: %s (%0llx:%u, %lx:%lx), master: %u\n", pSourceAgent->name, pEvent->src_agent, pEvent->src_instid, pSourceAgent->prof, pSourceAgent->elite, pEvent->src_master_instid);
	if (pEvent->dst_agent) p += _snprintf(p, 400, "target agent: %s (%0llx:%u, %lx:%lx)\n", pDestinationAgent->name, pEvent->dst_agent, pEvent->dst_instid, pDestinationAgent->prof, pDestinationAgent->elite);
	else p += _snprintf(p, 400, "target agent: n/a\n");

	p += _snprintf(p, 400, "is_buff: %u\n", pEvent->buff);
	p += _snprintf(p, 400, "skill: %s:%u\n", pSkillname, pEvent->skillid);
	p += _snprintf(p, 400, "dmg: %d\n", pEvent->value);
	p += _snprintf(p, 400, "is_moving: %u\n", pEvent->is_moving);
	p += _snprintf(p, 400, "is_ninety: %u\n", pEvent->is_ninety);
	p += _snprintf(p, 400, "is_flanking: %u\n", pEvent->is_flanking);
	p += _snprintf(p, 400, "is_shields: %u\n", pEvent->is_shields);

	p += _snprintf(p, 400, "iff: %u\n", pEvent->iff);
	p += _snprintf(p, 400, "result: %u\n", pEvent->result);
	cbtcount += 1;

	DWORD written = 0;
	p[0] = 0;
	wchar_t buffw[1024 * 16];
	int32_t rc = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, buffw, 1024 * 16);
	if (rc == 0)
	{
		char errString[2048];
		snprintf(errString, sizeof(errString), "MultiByteToWideChar failed, rc==%i error==%u", rc, GetLastError());
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), errString, strlen(errString), &written, nullptr);

		return 0;
	}

	buffw[rc] = 0;
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffw, (DWORD)lstrlenW(buffw), &written, nullptr);

	return 0;
	*/
}