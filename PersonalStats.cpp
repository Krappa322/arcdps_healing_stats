#include "PersonalStats.h"

#include "Log.h"
#include "Utilities.h"

#include <assert.h>
#include <Windows.h>

PersonalStats PersonalStats::GlobalState;

HealedAgent::HealedAgent(const char* pAgentName, uint16_t pSubGroup, bool pIsMinion)
	: Name(pAgentName)
	, Subgroup(pSubGroup)
	, IsMinion(pIsMinion)
{
}

AgentStats::AgentStats(uint64_t pAmountHealed, uint32_t pTicks)
	: TotalHealing(pAmountHealed)
	, Ticks(pTicks)
{
}

HealingSkill::HealingSkill(const char* pName)
	: Name(pName)
{
}


PersonalStats::PersonalStats()
	: myEnteredCombatTime(0)
{
}

void PersonalStats::AddAgent(uintptr_t pAgentId, const char* pAgentName, uint16_t pAgentSubGroup, bool pIsMinion)
{
	assert(pAgentName != nullptr);

	LOG("Inserting new agent %llu %s(%llu) %hu %s", pAgentId, pAgentName, utf8_strlen(pAgentName), pAgentSubGroup, BOOL_STR(pIsMinion));

	std::lock_guard<std::mutex> lock(myLock);

	auto emplaceResult = myStats.Agents.emplace(std::piecewise_construct,
		std::forward_as_tuple(pAgentId),
		std::forward_as_tuple(pAgentName, pAgentSubGroup, pIsMinion));

	if (emplaceResult.second == false)
	{
		if (strcmp(emplaceResult.first->second.Name.c_str(), pAgentName) != 0)
		{
			LOG("Already exists - replacing existing entry %s %hu %s", emplaceResult.first->second.Name.c_str(), emplaceResult.first->second.Subgroup, BOOL_STR(emplaceResult.first->second.IsMinion));
		}

		emplaceResult.first->second.Name = pAgentName;
		emplaceResult.first->second.Subgroup = pAgentSubGroup;
		emplaceResult.first->second.IsMinion = pIsMinion;
	}
}

void PersonalStats::EnteredCombat(uint64_t pTime, uint16_t pSubGroup)
{
	std::lock_guard<std::mutex> lock(myLock);

	if (myEnteredCombatTime == 0)
	{
		myEnteredCombatTime = pTime;

		// Clearing agents here is problematic because the healed agent could've entered combat before us. Not clearing it at
		// all is also problematic because eventually the map will grow very big. Not clearing it for now and will see if
		// that becomes a real issue.

		myStats.SkillsHealing.clear();
		myStats.SubGroup = pSubGroup;

		LOG("Entered combat, time is %llu, subgroup is %hu", pTime, pSubGroup);
	}
	else
	{
		LOG("Tried to enter combat when already in combat (old time %llu, current time %llu)", myEnteredCombatTime, pTime);
	}
}

void PersonalStats::ExitedCombat(uint64_t pTime)
{
	std::lock_guard<std::mutex> lock(myLock);

	if (myEnteredCombatTime == 0)
	{
		LOG("Tried to exit combat when not in combat (current time %llu)", pTime);
		return;
	}

	if (pTime <= myEnteredCombatTime)
	{
		LOG("Tried to exit combat with timestamp earlier than combat start (current time %llu, combat start %llu)", pTime, myEnteredCombatTime);
		return;
	}

	myStats.TimeInCombat = pTime - myEnteredCombatTime;
	myEnteredCombatTime = 0;

	LOG("Spent %llu ms in combat", myStats.TimeInCombat);
}

void PersonalStats::HealingEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, char* pSkillname)
{
	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}

	{
		std::lock_guard<std::mutex> lock(myLock);

		if (myEnteredCombatTime == 0)
		{
			//LOG("Tried to register healing event but not in combat - size %i from %s:%u to %s:%llu", healedAmount, pSkillname, pEvent->skillid, pDestinationAgent->name, pDestinationAgent->id);
			return;
		}

		// Attribute the heal to skill (we don't care if insertion happened or not, behavior is the same)
		auto [skill, insertedSkill] = myStats.SkillsHealing.emplace(std::piecewise_construct,
			std::forward_as_tuple(pEvent->skillid),
			std::forward_as_tuple(pSkillname));

		auto [agent, insertedAgent] = skill->second.AgentsHealing.emplace(std::piecewise_construct,
			std::forward_as_tuple(pDestinationAgent->id),
			std::forward_as_tuple(healedAmount, 1));
		if (insertedAgent == false) // Didn't insert new entry (otherwise it gets initialized with healedAmount)
		{
			agent->second.TotalHealing += healedAmount;
			agent->second.Ticks += 1;
		}
	}

	//LOG("Registered heal event size %i from %s:%u(%hu) to %s:%llu(%hu)", healedAmount, pSkillname, pEvent->skillid, pSourceAgent->team, pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->team);
}

HealingStats PersonalStats::GetGlobalState()
{
	std::lock_guard<std::mutex> lock(PersonalStats::GlobalState.myLock);

	HealingStats result;
	if (PersonalStats::GlobalState.myEnteredCombatTime == 0)
	{
		result.TimeInCombat = PersonalStats::GlobalState.myStats.TimeInCombat;
	}
	else
	{
		result.TimeInCombat = timeGetTime() - PersonalStats::GlobalState.myEnteredCombatTime;
	}

	result.Agents = PersonalStats::GlobalState.myStats.Agents;
	result.SkillsHealing = PersonalStats::GlobalState.myStats.SkillsHealing;
	result.SubGroup = PersonalStats::GlobalState.myStats.SubGroup;

	return result;
}
