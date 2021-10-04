#pragma once

#include <cstdint>

#define HEALING_STATS_ADDON_SIGNATURE 0x9c9b3c99
#define HEALING_STATS_EVTC_REVISION 2
#define HEALING_STATS_VERSION "2.2rc1"

#define VERSION_EVENT_SIGNATURE 0x00000000
struct EvtcVersionHeader
{
	uint32_t Signature;
	uint32_t EvtcRevision : 24;
	uint32_t VersionStringLength : 8;
};

enum HealingEventFlags
{
	HealingEventFlags_IsOffcycle = 1 << 0,
	HealingEventFlags_EventCameFromDestination = 1 << 6,
	HealingEventFlags_EventCameFromSource = 1 << 7
};
