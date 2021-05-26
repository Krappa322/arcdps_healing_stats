// dllmain.cpp : Defines the entry point for the DLL application.
#include "Exports.h"

#include "Windows.h"

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//printf("DllMain: %p\n", hModule);
		GlobalObjects::SELF_HANDLE = hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// This triggers the linker to pick up the two exported functions from the static library
void UnusedFunctionToHelpTheLinker()
{
	get_init_addr(nullptr, nullptr, nullptr, NULL, nullptr, nullptr);
	get_release_addr();
}