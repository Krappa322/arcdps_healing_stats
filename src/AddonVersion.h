#pragma once

#include <cstdint>

#define HEALING_STATS_ADDON_SIGNATURE 0x9c9b3c99U
#define HEALING_STATS_EVTC_REVISION 2U
#define LEGACY_INI_CONFIG_PATH "addons\\arcdps\\arcdps_healing_stats.ini"
#define JSON_CONFIG_PATH "addons\\arcdps\\arcdps_healing_stats.json"

#define VERSION_EVENT_SIGNATURE 0x00000000U
struct EvtcVersionHeader
{
	uint32_t Signature;
	uint32_t EvtcRevision : 24;
	uint32_t VersionStringLength : 8;
};

enum HealingEventFlags : uint8_t
{
	// The target of the event is downed. Only gets set for buff damage/healing events since arcdps overrides pad61, for
	// direct damage/healing, look at the lower bits of is_offcycle instead (as documented in arcdps evtc documentation)
	HealingEventFlags_TargetIsDowned = 1 << 5,
	// Signifies that the agent identified by dst_agent/dst_instid generated the event. This lets an event parser figure
	// out which agents are sending events, and as a result also for which players the healing data is complete
	HealingEventFlags_EventCameFromDestination = 1 << 6,
	// Signifies that the agent identified by src_agent/src_instid generated the event. This lets an event parser figure
	// out which agents are sending events, and as a result also for which players the healing data is complete
	HealingEventFlags_EventCameFromSource = 1 << 7
};
