#pragma once
#include "ArcDPS.h"

#include <stdint.h>

#include <map>
#include <mutex>
#include <string>

struct HealedAgent
{
	std::string Name; // Agent Name (UTF8)
	uint16_t Subgroup;
	bool IsMinion;

	HealedAgent(const char* pAgentName, uint16_t pSubGroup, bool pIsMinion);
};

struct AgentStats
{
	uint64_t TotalHealing = 0;
	uint32_t Ticks = 0; // Number of times the agent was healed

	AgentStats(uint64_t pAmountHealed, uint32_t pTicks);
};

struct HealingSkill
{
	const char* Name; // Skill Name
	std::map<uintptr_t, AgentStats> AgentsHealing; // <Agent Id, Stats>

	explicit HealingSkill(const char* pName);
};

struct HealingStats
{
	uint64_t TimeInCombat = 0;
	std::map<uint32_t, HealingSkill> SkillsHealing; // <Skill Id, Stats>
	std::map<uintptr_t, HealedAgent> Agents; // <Agent Id, Agent>
	uint16_t SubGroup = 0;
};

class PersonalStats
{
public:
	PersonalStats();

	void AddAgent(uintptr_t pAgentId, const char* pAgentName, uint16_t pAgentSubGroup, bool pIsMinion);

	void EnteredCombat(uint64_t pTime, uint16_t pSubGroup);
	void ExitedCombat(uint64_t pTime);
	void HealingEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId, const char* pDestinationAgentName, char* pSkillname); // pDestinationAgentName can be nullptr


	static PersonalStats GlobalState;

	static HealingStats GetGlobalState();

private:
	std::mutex myLock;

	uint64_t myEnteredCombatTime;
	HealingStats myStats;
};