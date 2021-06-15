#include "PlayerStats.h"

#include "Log.h"
#include "Utilities.h"

#include <assert.h>
#include <Windows.h>

HealEvent::HealEvent(uint64_t pTime, uint64_t pSize, uintptr_t pAgentId, uint32_t pSkillId)
	: Time{pTime}
	, Size{pSize}
	, AgentId{pAgentId}
	, SkillId{pSkillId}
{
}

bool HealEvent::operator==(const HealEvent& pRight) const
{
	return std::tie(Time, Size, AgentId, SkillId) == std::tie(pRight.Time, pRight.Size, pRight.AgentId, pRight.SkillId);
}

bool HealEvent::operator!=(const HealEvent& pRight) const
{
	return (*this == pRight) == false;
}

void PlayerStats::EnteredCombat(uint64_t pTime, uint16_t pSubGroup)
{
	std::lock_guard<std::mutex> lock(myLock);

	if (myStats.ExitedCombatTime != 0 || myStats.EnteredCombatTime == 0)
	{
		myStats.EnteredCombatTime = pTime;
		myStats.ExitedCombatTime = 0;
		myStats.LastDamageEvent = 0;

		myStats.Events.clear();
		myStats.SubGroup = pSubGroup;

		LOG("Entered combat, time is %llu, subgroup is %hu", pTime, pSubGroup);
	}
	else
	{
		LOG("Tried to enter combat when already in combat (old time %llu, current time %llu)", myStats.EnteredCombatTime, pTime);
	}
}

void PlayerStats::ExitedCombat(uint64_t pTime)
{
	std::lock_guard<std::mutex> lock(myLock);

	if (myStats.ExitedCombatTime != 0 || myStats.EnteredCombatTime == 0)
	{
		LOG("Tried to exit combat when not in combat (current time %llu)", pTime);
		return;
	}

	if (pTime <= myStats.EnteredCombatTime)
	{
		LOG("Tried to exit combat with timestamp earlier than combat start (current time %llu, combat start %llu)", pTime, myStats.EnteredCombatTime);
		return;
	}

	myStats.ExitedCombatTime = pTime;

	LOG("Spent %llu ms in combat, collected %zu events", myStats.ExitedCombatTime - myStats.EnteredCombatTime, myStats.Events.size());
}

void PlayerStats::DamageEvent(cbtevent* pEvent)
{
	{
		std::lock_guard<std::mutex> lock(myLock);

		if (myStats.EnteredCombatTime == 0 || myStats.ExitedCombatTime != 0)
		{
			return;
		}

		myStats.LastDamageEvent = pEvent->time;
	}
}

void PlayerStats::HealingEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId)
{
	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}

	{
		std::lock_guard<std::mutex> lock(myLock);

		if (myStats.EnteredCombatTime == 0 || myStats.ExitedCombatTime != 0)
		{
			LOG("Event before combat enter %llu", pEvent->time);
			return;
		}

		myStats.Events.emplace_back(pEvent->time, healedAmount, pDestinationAgentId, pEvent->skillid);
	}
}

HealingStatsSlim PlayerStats::GetState()
{
	std::lock_guard<std::mutex> lock(myLock);

	HealingStatsSlim result{myStats};
	return result;
}
