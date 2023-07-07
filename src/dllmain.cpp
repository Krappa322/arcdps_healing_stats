#include "AddonVersion.h"
#include "arcdps_structs.h"
#include "Exports.h"
#include "GUI.h"
#include "Log.h"
#include "PlayerStats.h"
#include "Utilities.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "../resource.h"

#include <atomic>

#include <assert.h>
#include <d3d9helper.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

#include <grpcpp/grpcpp.h>
#include <cpr/cpr.h>

/* proto/globals */
arcdps_exports* mod_init();
uintptr_t mod_release();
uintptr_t mod_imgui(uint32_t pNotCharSelectionOrLoading, uint32_t pHideIfCombatOrOoc);
uintptr_t mod_options_end();
uintptr_t mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
uintptr_t mod_wnd(HWND pWindowHandle, UINT pMessage, WPARAM pAdditionalW, LPARAM pAdditionalL);

uintptr_t ProcessLocalEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
void ProcessPeerEvent(cbtevent* pEvent, uint16_t pPeerInstanceId);

void Hook_PostNewFrame(ImGuiContext* pImguiContext, ImGuiContextHook*);
void Hook_PreEndFrame(ImGuiContext* pImguiContext, ImGuiContextHook*);

static MallocSignature ARCDPS_MALLOC = nullptr;
static FreeSignature ARCDPS_FREE = nullptr;
static arcdps_exports ARC_EXPORTS;
static const char* ARCDPS_VERSION;

std::mutex HEAL_TABLE_OPTIONS_MUTEX;
static HealTableOptions HEAL_TABLE_OPTIONS;

static BOOL EnumNamesFunc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam)
{
	LOG("EnumNamesFunc - %p %p '%ls' %p '%ls' %llu", hModule, lpType, IS_INTRESOURCE(lpType) ? L"IS_RESOURCE" : lpType, lpName, IS_INTRESOURCE(lpName) ? L"IS_RESOURCE" : lpName, lParam);

	return TRUE;
}

static BOOL EnumTypesFunc(HMODULE hModule, LPTSTR lpType, LONG_PTR lParam)
{
	LOG("EnumTypesFunc - %p %p '%ls' %llu", hModule, lpType, IS_INTRESOURCE(lpType) ? L"IS_RESOURCE" : lpType, lParam);

	bool res = EnumResourceNames(hModule, lpType, &EnumNamesFunc, 12345);
	LOG("EnumResourceNames returned %s", BOOL_STR(res));

	return TRUE;
}

const char* LoadRootCertificatesFromResource()
{
	HMODULE dll_handle = GlobalObjects::SELF_HANDLE;
	bool res = EnumResourceTypes(dll_handle, &EnumTypesFunc, 12345);
	LOG("EnumResourceTypes returned %s", BOOL_STR(res));

	HRSRC root_certificates_handle = FindResource(dll_handle, MAKEINTRESOURCE(IDR_ROOT_CERTIFICATES), L"CERTIFICATE");
	if (root_certificates_handle == NULL)
	{
		return "Failed to find root certificates";
	}

	// Load the dialog box into global memory.
	HGLOBAL root_certificates_data_handle = LoadResource(dll_handle, root_certificates_handle);
	if (root_certificates_data_handle == NULL)
	{
		return "Failed to load root certificates";
	}

	// Lock the dialog box into global memory.
	void* root_certificates_data = LockResource(root_certificates_data_handle);
	if (root_certificates_data == nullptr)
	{
		return "Failed to lock root certificates";
	}

	DWORD root_certificates_size = SizeofResource(dll_handle, root_certificates_handle);
	if (root_certificates_size == 0)
	{
		return "Failed to get root certificates size";
	}

	GlobalObjects::ROOT_CERTIFICATES.assign(static_cast<const char*>(root_certificates_data), root_certificates_size);
	LOG("Loaded root certificates, size %zu from %p %u", GlobalObjects::ROOT_CERTIFICATES.size(), root_certificates_data, root_certificates_size);
	return nullptr;
}

static const char* LoadRootCertificatesFromFile()
{
	std::ifstream filestream("roots.pem", std::ios::in);
	if (filestream.is_open() == false)
	{
		LOG("roots.pem doesn't exist");
		return "file doesn't exist";
	}

	filestream.seekg(0, std::ios::end);
	GlobalObjects::ROOT_CERTIFICATES.resize(filestream.tellg());
	filestream.seekg(0, std::ios::beg);
	filestream.read(&GlobalObjects::ROOT_CERTIFICATES[0], GlobalObjects::ROOT_CERTIFICATES.size());
	filestream.close();

	LOG("Loaded %s", GlobalObjects::ROOT_CERTIFICATES.c_str());
	return nullptr;
}

static void* MallocWrapper(size_t pSize, void* /*pUserData*/)
{
	return ARCDPS_MALLOC(pSize);
}

static void FreeWrapper(void* pPointer, void* /*pUserData*/)
{
	ARCDPS_FREE(pPointer);
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) ModInitSignature get_init_addr(const char* pArcdpsVersionString, void* pImguiContext, void*, HMODULE pArcModule, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree)
{
	GlobalObjects::ARC_E3 = reinterpret_cast<E3Signature>(GetProcAddress(pArcModule, "e3"));
	assert(GlobalObjects::ARC_E3 != nullptr);
	GlobalObjects::ARC_E7 = reinterpret_cast<E7Signature>(GetProcAddress(pArcModule, "e7"));
	assert(GlobalObjects::ARC_E7 != nullptr);
	GlobalObjects::ARC_E9 = reinterpret_cast<E9Signature>(GetProcAddress(pArcModule, "e9"));
	assert(GlobalObjects::ARC_E9 != nullptr);
	GlobalObjects::ARC_E10 = reinterpret_cast<E9Signature>(GetProcAddress(pArcModule, "e10"));
	assert(GlobalObjects::ARC_E10 != nullptr);

	ARCDPS_VERSION = pArcdpsVersionString;
	SetContext(pImguiContext);

	ARCDPS_MALLOC = pArcdpsMalloc;
	ARCDPS_FREE = pArcdpsFree;
	ImGui::SetAllocatorFunctions(MallocWrapper, FreeWrapper);

	if (pImguiContext != nullptr)
	{
		ImGuiContextHook contextHook;
		contextHook.Type = ImGuiContextHookType_NewFramePost;
		contextHook.Callback = Hook_PostNewFrame;
		ImGui::AddContextHook(reinterpret_cast<ImGuiContext*>(pImguiContext), &contextHook);

		contextHook.Type = ImGuiContextHookType_EndFramePost;
		contextHook.Callback = Hook_PreEndFrame;
		ImGui::AddContextHook(reinterpret_cast<ImGuiContext*>(pImguiContext), &contextHook);
	}

	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) ModReleaseSignature get_release_addr()
{
	LogD(">>");
	ARCDPS_VERSION = nullptr;

	LogI("<<");
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init()
{
	std::unique_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);

	if (GlobalObjects::IS_UNIT_TEST == false)
	{
		Log_::Init(false, "addons/logs/arcdps_healing_stats/arcdps_healing_stats.txt");
	}

	if (GlobalObjects::IS_SHUTDOWN == false)
	{
		LogW("mod_init called twice");
	}
	GlobalObjects::IS_SHUTDOWN = false;

	memset(&ARC_EXPORTS, 0, sizeof(arcdps_exports));
	ARC_EXPORTS.sig = HEALING_STATS_ADDON_SIGNATURE;
	ARC_EXPORTS.imguivers = IMGUI_VERSION_NUM;
	ARC_EXPORTS.size = sizeof(arcdps_exports);
	ARC_EXPORTS.out_name = "healing_stats";
	ARC_EXPORTS.out_build = &GlobalObjects::VERSION_STRING_FRIENDLY[0];
	ARC_EXPORTS.combat = mod_combat;
	ARC_EXPORTS.imgui = reinterpret_cast<ImguiCallbackSignature>(mod_imgui); // TODO: Remove cast when arcdps_extension is updated
	ARC_EXPORTS.options_end = mod_options_end;
	ARC_EXPORTS.combat_local = mod_combat_local;
	ARC_EXPORTS.wnd_nofilter = mod_wnd;

	GlobalObjects::UPDATE_CHECKER = std::make_unique<UpdateChecker>();
	const std::optional<UpdateChecker::Version> version_raw = GlobalObjects::UPDATE_CHECKER->GetCurrentVersion(GlobalObjects::SELF_HANDLE);
	if (version_raw.has_value() == false)
	{
		LogE("Failed startup - GetCurrentVersion failed");

		ARC_EXPORTS.sig = 0;
		ARC_EXPORTS.size = reinterpret_cast<uintptr_t>("GetCurrentVersion failed");
		return &ARC_EXPORTS;
	}

	GlobalObjects::VERSION = version_raw.value();
	fmt::format_to(GlobalObjects::VERSION_STRING_FRIENDLY, "{}.{}rc{}",
		GlobalObjects::VERSION[0], GlobalObjects::VERSION[1], GlobalObjects::VERSION[2]);

	const char* certificate_load_result = LoadRootCertificatesFromFile();
	if (certificate_load_result != nullptr)
	{
		certificate_load_result = LoadRootCertificatesFromResource();
	}

	if (certificate_load_result != nullptr)
	{
		LogI("Failed startup - {}", certificate_load_result);

		ARC_EXPORTS.sig = 0;
		ARC_EXPORTS.size = reinterpret_cast<uintptr_t>(certificate_load_result);
		return &ARC_EXPORTS;
	}

	auto getEndpoint = []() -> std::string
	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);

		return std::string{HEAL_TABLE_OPTIONS.EvtcRpcEndpoint};
	};
	auto getCertificates = []() -> std::string
	{
		return std::string{GlobalObjects::ROOT_CERTIFICATES};
	};

	GlobalObjects::EVENT_SEQUENCER = std::make_unique<EventSequencer>(ProcessLocalEvent);
	GlobalObjects::EVENT_PROCESSOR = std::make_unique<EventProcessor>();
	GlobalObjects::EVTC_RPC_CLIENT = std::make_unique<evtc_rpc_client>(std::move(getEndpoint), std::move(getCertificates), std::function{ProcessPeerEvent});

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		HEAL_TABLE_OPTIONS.Load(JSON_CONFIG_PATH);

		Log_::SetLevel(HEAL_TABLE_OPTIONS.LogLevel);
		GlobalObjects::EVENT_PROCESSOR->SetEvtcLoggingEnabled(HEAL_TABLE_OPTIONS.EvtcLoggingEnabled);
		GlobalObjects::EVENT_PROCESSOR->SetUseBarrier(HEAL_TABLE_OPTIONS.IncludeBarrier);
		GlobalObjects::EVTC_RPC_CLIENT->SetEnabledStatus(HEAL_TABLE_OPTIONS.EvtcRpcEnabled);

		if (HEAL_TABLE_OPTIONS.AutoUpdateSetting != AutoUpdateSettingEnum::Off)
		{
			const bool enablePreReleases = (HEAL_TABLE_OPTIONS.AutoUpdateSetting == AutoUpdateSettingEnum::PreReleases);
			GlobalObjects::UPDATE_CHECKER->ClearFiles(GlobalObjects::SELF_HANDLE);
			GlobalObjects::UPDATE_STATE = GlobalObjects::UPDATE_CHECKER->CheckForUpdate(GlobalObjects::SELF_HANDLE, GlobalObjects::VERSION, "Krappa322/arcdps_healing_stats", enablePreReleases);
		}
	}

	GlobalObjects::EVTC_RPC_CLIENT_THREAD = std::make_unique<std::thread>(evtc_rpc_client::ThreadStartServe, GlobalObjects::EVTC_RPC_CLIENT.get());

	LogI("Startup completed, arcdps_version={} healing_stats_version={} cpr_version={} curl_version={} grpc_version={} grpc_core_version={}",
		ARCDPS_VERSION, ARC_EXPORTS.out_build, CPR_VERSION, LIBCURL_VERSION, grpc::Version(), grpc_version_string());
	return &ARC_EXPORTS;
}

/* release mod -- return ignored */
uintptr_t mod_release()
{
	LogD("Shutting down, sequencer={}, processor={}, client={} client_thread={}",
		static_cast<void*>(GlobalObjects::EVENT_SEQUENCER.get()),
		static_cast<void*>(GlobalObjects::EVENT_PROCESSOR.get()),
		static_cast<void*>(GlobalObjects::EVTC_RPC_CLIENT.get()),
		static_cast<void*>(GlobalObjects::EVTC_RPC_CLIENT_THREAD.get()));

	{
		std::unique_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
		if (GlobalObjects::IS_SHUTDOWN == true)
		{
			LogW("mod_release called before mod_init");
			return 0;
		}
		GlobalObjects::IS_SHUTDOWN = true;
	}

	GlobalObjects::EVTC_RPC_CLIENT->Shutdown();

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		HEAL_TABLE_OPTIONS.Save(JSON_CONFIG_PATH);
	}

	if (GlobalObjects::UPDATE_STATE != nullptr)
	{
		GlobalObjects::UPDATE_STATE->FinishPendingTasks();
	}
	GlobalObjects::UPDATE_STATE = nullptr;
	GlobalObjects::UPDATE_CHECKER = nullptr;

	GlobalObjects::EVTC_RPC_CLIENT_THREAD->join();
	GlobalObjects::EVTC_RPC_CLIENT_THREAD = nullptr;
	GlobalObjects::EVTC_RPC_CLIENT = nullptr;
	GlobalObjects::EVENT_PROCESSOR = nullptr;
	GlobalObjects::EVENT_SEQUENCER = nullptr;

	LogI("Shutdown completed");
	Log_::FlushLogFile();

	if (GlobalObjects::IS_UNIT_TEST == false)
	{
		Log_::LOGGER = nullptr;
		spdlog::shutdown();
	}

	return 0;
}

uintptr_t mod_imgui(uint32_t pNotCharSelectionOrLoading, uint32_t pHideIfCombatOrOoc)
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return 1;
	}

	if (pNotCharSelectionOrLoading == 0 || pHideIfCombatOrOoc != 0)
	{
		return 0;
	}

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		HEAL_TABLE_OPTIONS.AnchoringHighlightedWindows.clear();
		Display_UpdateWindow();
		Display_GUI(HEAL_TABLE_OPTIONS);
	}

	return 0;
}

uintptr_t mod_options_end()
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return 1;
	}

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		Display_AddonOptions(HEAL_TABLE_OPTIONS);
	}

	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return 1;
	}

	GlobalObjects::EVENT_PROCESSOR->AreaCombat(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	GlobalObjects::EVTC_RPC_CLIENT->ProcessAreaEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	return 0;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return 1;
	}

	GlobalObjects::EVENT_SEQUENCER->ProcessEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	return 0;
}

uintptr_t ProcessLocalEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	std::optional<cbtevent> modifiedEvent = std::nullopt;
	GlobalObjects::EVENT_PROCESSOR->LocalCombat(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision, &modifiedEvent);
	if (modifiedEvent.has_value())
	{
		pEvent = &modifiedEvent.value();
	}

	GlobalObjects::EVTC_RPC_CLIENT->ProcessLocalEvent(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
	return 0;
}

void ProcessPeerEvent(cbtevent* pEvent, uint16_t pPeerInstanceId)
{
	assert(GlobalObjects::IS_SHUTDOWN == false); // Not atomic but that's fine, this is more of a sanity check

	GlobalObjects::EVENT_PROCESSOR->PeerCombat(pEvent, pPeerInstanceId);
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
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return pMessage;
	}

	ImGui_ProcessKeyEvent(pWindowHandle, pMessage, pAdditionalW, pAdditionalL);

	const ImGuiIO& io = ImGui::GetIO();

	if (pMessage == WM_KEYDOWN || pMessage == WM_SYSKEYDOWN)
	{
		int virtualKey = static_cast<int>(pAdditionalW);

		uint64_t e7_rawResult = GlobalObjects::ARC_E7();
		ArcModifiers modifiers;
		memcpy(&modifiers, &e7_rawResult, sizeof(modifiers));

		if ((modifiers._1 == 0 || io.KeysDown[modifiers._1] == true) &&
			(modifiers._2 == 0 || io.KeysDown[modifiers._2] == true))
		{
			std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);

			bool triggeredKey = false;
			for (uint32_t i = 0; i < HEAL_WINDOW_COUNT; i++)
			{
				if (HEAL_TABLE_OPTIONS.Windows[i].Hotkey > 0 &&
					static_cast<size_t>(HEAL_TABLE_OPTIONS.Windows[i].Hotkey) < sizeof(io.KeysDown) &&
					virtualKey == HEAL_TABLE_OPTIONS.Windows[i].Hotkey)
				{
					assert(io.KeysDown[HEAL_TABLE_OPTIONS.Windows[i].Hotkey] == true);

					HEAL_TABLE_OPTIONS.Windows[i].Shown = !HEAL_TABLE_OPTIONS.Windows[i].Shown;
					triggeredKey = true;

					LOG("Key %i '%s' toggled window %u - new heal window state is %s", HEAL_TABLE_OPTIONS.Windows[i].Hotkey, VirtualKeyToString(HEAL_TABLE_OPTIONS.Windows[i].Hotkey).c_str(), i, BOOL_STR(HEAL_TABLE_OPTIONS.Windows[i].Shown));
				}
			}
			if (HEAL_TABLE_OPTIONS.EvtcRpcEnabledHotkey > 0 &&
				static_cast<size_t>(HEAL_TABLE_OPTIONS.EvtcRpcEnabledHotkey) < sizeof(io.KeysDown) &&
				virtualKey == HEAL_TABLE_OPTIONS.EvtcRpcEnabledHotkey)
			{
				HEAL_TABLE_OPTIONS.EvtcRpcEnabled = !HEAL_TABLE_OPTIONS.EvtcRpcEnabled;
				GlobalObjects::EVTC_RPC_CLIENT->SetEnabledStatus(HEAL_TABLE_OPTIONS.EvtcRpcEnabled);
				triggeredKey = true;

				LogI("Key {} {} toggled evtc rpc enabled - new state is {}", virtualKey, VirtualKeyToString(virtualKey).c_str(), BOOL_STR(HEAL_TABLE_OPTIONS.EvtcRpcEnabled));
			}

			if (triggeredKey == true)
			{
				return 0; // Don't process message by arcdps or game
			}
		}
	}

	return pMessage;
}

void Hook_PostNewFrame(ImGuiContext* pImguiContext, ImGuiContextHook*)
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return;
	}

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		Display_PostNewFrame(pImguiContext, HEAL_TABLE_OPTIONS);
	}
}

void Hook_PreEndFrame(ImGuiContext* pImguiContext, ImGuiContextHook*)
{
	std::shared_lock shutdown_lock(GlobalObjects::SHUTDOWN_LOCK);
	if (GlobalObjects::IS_SHUTDOWN == true)
	{
		DEBUGLOG("already shutdown");
		return;
	}

	{
		std::lock_guard lock(HEAL_TABLE_OPTIONS_MUTEX);
		Display_PreEndFrame(pImguiContext, HEAL_TABLE_OPTIONS);
	}
}