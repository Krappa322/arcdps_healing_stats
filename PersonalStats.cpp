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

HealingSkill::HealingSkill(const char* pName)
	: Name(pName)
{
}

HealEvent::HealEvent(uint64_t pTime, uint64_t pSize, uintptr_t pAgentId, uint32_t pSkillId)
	: Time{pTime}
	, Size{pSize}
	, AgentId{pAgentId}
	, SkillId{pSkillId}
{
}


// We can't just implicitly add all agents because agent subgroup is only known through this event
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
		if ((strcmp(emplaceResult.first->second.Name.c_str(), pAgentName) != 0)
			|| (emplaceResult.first->second.Subgroup != pAgentSubGroup)
			|| (emplaceResult.first->second.IsMinion != pIsMinion))
		{
			LOG("Already exists - replacing existing entry %s %hu %s", emplaceResult.first->second.Name.c_str(), emplaceResult.first->second.Subgroup, BOOL_STR(emplaceResult.first->second.IsMinion));

			emplaceResult.first->second.Name = pAgentName;
			emplaceResult.first->second.Subgroup = pAgentSubGroup;
			emplaceResult.first->second.IsMinion = pIsMinion;
		}
	}
}

void PersonalStats::EnteredCombat(uint64_t pTime, uint16_t pSubGroup)
{
	std::lock_guard<std::mutex> lock(myLock);

	if (myStats.ExitedCombatTime != 0 || myStats.EnteredCombatTime == 0)
	{
		myStats.EnteredCombatTime = pTime;
		myStats.ExitedCombatTime = 0;
		myStats.LastDamageEvent = 0;

		//myStats.Agents.clear(); // Clearing agents here doesn't work because then agents can get combat entered events before we do, meaning their subgroup is not mapped
		myStats.Skills.clear();
		myStats.Events.clear();
		myStats.SubGroup = pSubGroup;

		LOG("Entered combat, time is %llu, subgroup is %hu", pTime, pSubGroup);
	}
	else
	{
		LOG("Tried to enter combat when already in combat (old time %llu, current time %llu)", myStats.EnteredCombatTime, pTime);
	}
}

void PersonalStats::ExitedCombat(uint64_t pTime)
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

	LOG("Spent %llu ms in combat", myStats.ExitedCombatTime - myStats.EnteredCombatTime);
}

void PersonalStats::DamageEvent(cbtevent* pEvent)
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

void PersonalStats::HealingEvent(cbtevent* pEvent, uintptr_t pDestinationAgentId, const char* pDestinationAgentName, bool pDestinationAgentIsMinion, const char* pSkillname)
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
			return;
		}

		// Register agent if it's not already known
		if (pDestinationAgentName != nullptr)
		{
			auto [_unused, insertedAgentTable] = myStats.Agents.emplace(std::piecewise_construct,
				std::forward_as_tuple(pDestinationAgentId),
				std::forward_as_tuple(pDestinationAgentName, static_cast<uint16_t>(0), pDestinationAgentIsMinion));
			if (insertedAgentTable == true)
			{
				LOG("Implicitly added agent %llu %s(%llu) %s", pDestinationAgentId, pDestinationAgentName, utf8_strlen(pDestinationAgentName), BOOL_STR(pDestinationAgentIsMinion));
			}
		}

		// Register skill if it's not already known
		if (pSkillname != nullptr)
		{
			auto [_unused, insertedSkillTable] = myStats.Skills.emplace(std::piecewise_construct,
				std::forward_as_tuple(pEvent->skillid),
				std::forward_as_tuple(pSkillname));
			if (insertedSkillTable == true)
			{
				LOG("Implicitly added skill %u %s", pEvent->skillid, pSkillname);
			}
		}

		// Attribute the heal to skill (we don't care if insertion happened or not, behavior is the same)
		myStats.Events.emplace_back(pEvent->time, healedAmount, pDestinationAgentId, pEvent->skillid);
	}

	//LOG("Registered heal event size %i from %s:%u to %s:%llu", healedAmount, pSkillname, pEvent->skillid, pDestinationAgentName, pDestinationAgentId);
}

HealingStats PersonalStats::GetGlobalState()
{
	std::lock_guard<std::mutex> lock(PersonalStats::GlobalState.myLock);

	HealingStats result{PersonalStats::GlobalState.myStats};
	result.CollectionTime = timeGetTime();
	return result;
}
