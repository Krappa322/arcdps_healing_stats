#pragma once
#include "arcdps_structs.h"

#include <stdint.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

struct HealEvent
{
	const uint64_t Time = 0;
	const uint64_t Size = 0;
	const uintptr_t AgentId = 0;
	const uint32_t SkillId = 0;
	const bool IsBarrierGeneration = false;

	HealEvent(uint64_t pTime, uint64_t pSize, uintptr_t pAgentId, uint32_t pSkillId, bool pIsBarrierGeneration);

	bool operator==(const HealEvent& pRight) const;
	bool operator!=(const HealEvent& pRight) const;
};

struct HealingStatsSlim
{
	uint64_t EnteredCombatTime = 0;
	uint64_t ExitedCombatTime = 0;
	uint64_t LastDamageEvent = 0;
	uint16_t SubGroup = 0;

	std::vector<HealEvent> Events; // Really this should be some segmented vector, will probably write one if it becomes a pain point

	bool IsOutOfCombat();
};

class PlayerStats
{
public:
	PlayerStats() = default;

	void EnteredCombat(uint64_t pTime, uint16_t pSubGroup);

	// Returns last damage event time (or 0 if combat wasn't really exited)
	uint64_t ExitedCombat(uint64_t pTime, uint64_t pLastDamageEventTime = 0);
	bool ResetIfNotInCombat(); // Returns true if peer was reset

	void DamageEvent(uint64_t pTime);
	void HealingEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId);
	void BarrierGenerationEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId);

	HealingStatsSlim GetState();

private:
	std::mutex myLock;

	HealingStatsSlim myStats;
};