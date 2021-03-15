#pragma once
#include "ArcDPS.h"

#include <stdint.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

struct HealedAgent
{
	std::string Name; // Agent Name (UTF8)
	uint16_t Subgroup;
	bool IsMinion;

	HealedAgent(const char* pAgentName, uint16_t pSubGroup, bool pIsMinion);
};

struct HealingSkill
{
	const char* Name; // Skill Name

	explicit HealingSkill(const char* pName);
};

struct HealEvent
{
	const uint64_t Time = 0;
	const uint64_t Size = 0;
	const uintptr_t AgentId = 0;
	const uint32_t SkillId = 0;

	HealEvent(uint64_t pTime, uint64_t pSize, uintptr_t pAgentId, uint32_t pSkillId);
};

struct HealingStats
{
	uint64_t EnteredCombatTime = 0;
	uint64_t ExitedCombatTime = 0;
	uint64_t LastDamageEvent = 0;
	uint64_t CollectionTime = 0;
	uint16_t SubGroup = 0;

	std::map<uintptr_t, HealedAgent> Agents; // <Agent Id, Agent>
	std::map<uint32_t, HealingSkill> Skills; // <Skill Id, Skill>
	std::vector<HealEvent> Events; // Really this should be some segmented vector, will probably write one if it becomes a pain point
};

class PersonalStats
{
public:
	PersonalStats() = default;

	void AddAgent(uintptr_t pAgentId, const char* pAgentName, uint16_t pAgentSubGroup, bool pIsMinion);

	void EnteredCombat(uint64_t pTime, uint16_t pSubGroup);
	void ExitedCombat(uint64_t pTime);

	void DamageEvent(cbtevent* pEvent);
	void HealingEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId, const char* pDestinationAgentName, bool pDestinationAgentIsMinion, const char* pSkillname); // pDestinationAgentName can be nullptr


	static PersonalStats GlobalState;

	static HealingStats GetGlobalState();

private:
	std::mutex myLock;

	HealingStats myStats;
};