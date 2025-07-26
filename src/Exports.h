#pragma once

#include "EventProcessor.h"
#include "EventSequencer.h"
#include "UpdateGUI.h"
#include "../networking/Client.h"

#include <ArcdpsExtension/arcdps_structs.h>
#include <imgui/imgui.h>

#include <memory>
#include <shared_mutex>

typedef void (*E3Signature)(const char* pString);
typedef void (*E5Signature)(ImVec4** pColors);
typedef uint64_t(*E7Signature)();
typedef void (*E9Signature)(cbtevent* pEvent, uint32_t pSignature);

class GlobalObjects
{
public:
	static inline bool IS_UNIT_TEST = false;

	static inline HMODULE SELF_HANDLE = NULL;
	static inline E3Signature ARC_E3 = nullptr;
	static inline E5Signature ARC_E5 = nullptr;
	static inline E7Signature ARC_E7 = nullptr;
	static inline E9Signature ARC_E9 = nullptr;
	static inline E9Signature ARC_E10 = nullptr;
	static inline std::unique_ptr<EventSequencer> EVENT_SEQUENCER = nullptr;
	static inline std::unique_ptr<EventProcessor> EVENT_PROCESSOR = nullptr;
	static inline std::unique_ptr<evtc_rpc_client> EVTC_RPC_CLIENT = nullptr;
	static inline std::unique_ptr<std::thread> EVTC_RPC_CLIENT_THREAD = nullptr;

	static inline UpdateChecker::Version VERSION = {};
	static inline char VERSION_STRING_FRIENDLY[128] = {};
	static inline std::unique_ptr<UpdateChecker> UPDATE_CHECKER = nullptr;
	static inline std::unique_ptr<UpdateChecker::UpdateState> UPDATE_STATE = nullptr;

	static inline std::string ROOT_CERTIFICATES = "";

	static inline ImVec4* COLORS[5] = {};

	static inline std::shared_mutex SHUTDOWN_LOCK;
	static inline bool IS_SHUTDOWN = true;
};

typedef void* (*MallocSignature)(size_t);
typedef void (*FreeSignature)(void*);
extern "C" __declspec(dllexport) ModInitSignature get_init_addr(const char* pArcdpsVersionString, void* pImguiContext, void* pID3DPtr, HMODULE pArcModule, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree, uint32_t pD3DVersion);
extern "C" __declspec(dllexport) ModReleaseSignature get_release_addr();