#pragma once
#include "arcdps_structs.h"

typedef uint64_t(*E7Signature)();
static inline E7Signature ARC_E7 = nullptr;

typedef void (*E3Signature)(const char* pString);
static inline E3Signature ARC_E3 = nullptr;

typedef void* (*MallocSignature)(size_t);
typedef void (*FreeSignature)(void*);
extern "C" __declspec(dllexport) ModInitSignature get_init_addr(const char* pArcdpsVersionString, void* pImguiContext, IDirect3DDevice9 * pUnused, HMODULE pArcModule, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree);
extern "C" __declspec(dllexport) ModReleaseSignature get_release_addr();