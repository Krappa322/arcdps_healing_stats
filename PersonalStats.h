#pragma once
#include "ArcDPS.h"

#include <map>
#include <mutex>
#include <stdint.h>

struct HealedAgent
{
	char Name[128]; // Agent Name (UTF8)
	uint64_t TotalHealing;
	uint32_t Ticks; // Number of times the agent was healed
};

struct HealingSkill
{
	char* Name; // Skill Name
	uint64_t TotalHealing;
	uint32_t Ticks; // Number of healing events the skill was involved in (could be multiple when you cleave with healing)
};

class PersonalStats
{
public:
	PersonalStats();

	void EnteredCombat(uint64_t pTime);
	void ExitedCombat(uint64_t pTime);
	void HealingEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname);

private:
	std::mutex myLock;

	uint64_t myEnteredCombatTime;
	uint64_t myTotalHealing;
	std::map<uintptr_t, HealedAgent> myAgentsHealing; // <Agent Id, Stats>
	std::map<uint32_t, HealingSkill> mySkillsHealing; // <Skill Id, Stats>
};