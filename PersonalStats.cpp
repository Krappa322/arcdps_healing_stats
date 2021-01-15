#include "PersonalStats.h"

#include "Log.h"

#include <assert.h>

PersonalStats::PersonalStats()
	: myEnteredCombatTime(0)
	, myTotalHealing(0)
{
}

void PersonalStats::EnteredCombat(uint64_t pTime)
{
	std::unique_lock<std::mutex> lock(myLock);

	if (myEnteredCombatTime == 0)
	{
		myEnteredCombatTime = pTime;
		LOG("Entered combat, time is %llu", pTime);
	}
	else
	{
		LOG("Tried to enter combat when already in combat (old time %llu, current time %llu)", myEnteredCombatTime, pTime);
	}
}

void PersonalStats::ExitedCombat(uint64_t pTime)
{
	std::unique_lock<std::mutex> lock(myLock);

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

	float secondsInCombat = static_cast<float>(pTime - myEnteredCombatTime) / 1000;
	LOG("Exited combat, combat started at %llu, time is %llu - spent %.2f seconds in combat", myEnteredCombatTime, pTime, secondsInCombat);

	for (const auto& agent : myAgentsHealing)
	{
		LOG("Agent %s:%llu - Healed for %llu over %u ticks (%.2f/s, %.2f per tick)",
			agent.second.Name, agent.first,
			agent.second.TotalHealing, agent.second.Ticks,
			agent.second.TotalHealing / secondsInCombat, static_cast<float>(agent.second.TotalHealing) / agent.second.Ticks);
	}
	LOG("");
	for (const auto& skill : mySkillsHealing)
	{
		LOG("Skill %s:%u - Healed for %llu over %u ticks (%.2f/s, %.2f per tick)",
			skill.second.Name, skill.first,
			skill.second.TotalHealing, skill.second.Ticks,
			skill.second.TotalHealing / secondsInCombat, static_cast<float>(skill.second.TotalHealing) / skill.second.Ticks);
	}
	myAgentsHealing.clear();
	mySkillsHealing.clear();

	myEnteredCombatTime = 0;
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
		std::unique_lock<std::mutex> lock(myLock);

		if (myEnteredCombatTime == 0)
		{
			LOG("Tried to register healing event but not in combat - size %i from %s:%u to %s:%llu", healedAmount, pSkillname, pEvent->skillid, pDestinationAgent->name, pDestinationAgent->id);
			return;
		}

		// Attribute the heal to agent
		auto agent = myAgentsHealing.find(pDestinationAgent->id);
		if (agent == myAgentsHealing.end())
		{
			LOG("Registering new agent %s:%llu", pDestinationAgent->name, pDestinationAgent->id);

			HealedAgent newAgent;
			snprintf(newAgent.Name, sizeof(newAgent.Name), "%s", pDestinationAgent->name);
			newAgent.TotalHealing = healedAmount;
			newAgent.Ticks = 1;
			myAgentsHealing.emplace(std::make_pair(pDestinationAgent->id, std::move(newAgent)));
		}
		else
		{
			agent->second.TotalHealing += healedAmount;
			agent->second.Ticks += 1;
		}

		// Attribute the heal to skill
		auto skill = mySkillsHealing.find(pEvent->skillid);
		if (skill == mySkillsHealing.end())
		{
			LOG("Registering new skill %s:%u", pSkillname, pEvent->skillid);

			HealingSkill newSkill;
			newSkill.Name = pSkillname; // The skill name should be valid forever
			newSkill.TotalHealing = healedAmount;
			newSkill.Ticks = 1;
			mySkillsHealing.emplace(std::make_pair(pEvent->skillid, std::move(newSkill)));
		}
		else
		{
			skill->second.TotalHealing += healedAmount;
			skill->second.Ticks += 1;
		}
	}

	LOG("Registered heal event size %i from %s:%u to %s:%llu", healedAmount, pSkillname, pEvent->skillid, pDestinationAgent->name, pDestinationAgent->id);
}
