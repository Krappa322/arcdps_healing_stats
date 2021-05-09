#include "EventProcessor.h"
#include "Log.h"
#include "Skills.h"

#include <cassert>

EventProcessor::EventProcessor()
	: mSkillTable(std::make_shared<SkillTable>())
{
}

void EventProcessor::AreaCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pEvent == nullptr)
	{
		if (pSourceAgent->elite != 0)
		{
			// Not agent adding event, uninteresting
			return;
		}

		if (pSourceAgent->prof != 0)
		{
			LOG("Register agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			bool isPlayer = false;
			if (pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0')
			{
				isPlayer = true;
			}

			// name == nullptr here shouldn't be able to happen through arcdps, but it makes unit testing easier :)
			if (pSourceAgent->name != nullptr)
			{
				mAgentTable.AddAgent(pSourceAgent->id, pDestinationAgent->id, pSourceAgent->name, pDestinationAgent->team, std::nullopt, isPlayer);
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);
		}

		return;
	}

	if (pEvent->is_statechange == CBTS_ENTERCOMBAT)
	{
		LOG("EnterCombat agent %s %llu %hu %u %llu",
			pSourceAgent->name, pSourceAgent->id, pEvent->src_master_instid, pSourceAgent->self, pEvent->dst_agent);

		bool isMinion = false;
		if (pEvent->src_master_instid != 0)
		{
			isMinion = true;
		}
		bool isPlayer = false;
		if (pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0')
		{
			isPlayer = true;
		}

		// name == nullptr here shouldn't be able to happen through arcdps, but it makes unit testing easier :)
		if (pSourceAgent->name != nullptr)
		{
			mAgentTable.AddAgent(pSourceAgent->id, pEvent->src_instid, pSourceAgent->name, static_cast<uint16_t>(pEvent->dst_agent), isMinion, isPlayer);
		}

		return;
	}
	else if (pEvent->is_statechange == CBTS_EXITCOMBAT)
	{
		LOG("ExitCombat agent %s %llu %hu %u",
			pSourceAgent->name, pSourceAgent->id, pEvent->src_master_instid, pSourceAgent->self);
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		return;
	}

	if (pEvent->buff != 0 && pEvent->buff_dmg == 0)
	{
		// Buff application - not interesting
		return;
	}

	// Event actually did something
	if (pEvent->buff_dmg > 0 || pEvent->value > 0)
	{
		mSkillTable->RegisterDamagingSkill(pEvent->skillid, pSkillname);
		return;
	}
}

void EventProcessor::LocalCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pEvent == nullptr)
	{
		if (pSourceAgent->elite != 0)
		{
			// Not agent adding event, uninteresting
			return;
		}

		if (pSourceAgent->prof != 0)
		{
			LOG("Register agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			bool isPlayer = false;
			if (pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0')
			{
				isPlayer = true;
			}

			// name == nullptr here shouldn't be able to happen through arcdps, but it makes unit testing easier :)
			if (pSourceAgent->name != nullptr)
			{
				mAgentTable.AddAgent(pSourceAgent->id, pDestinationAgent->id, pSourceAgent->name, pDestinationAgent->team, std::nullopt, isPlayer);
			}

			if (pDestinationAgent->self != 0)
			{
				assert(pDestinationAgent->id <= UINT16_MAX);

				LOG("Storing self instance id %llu", pDestinationAgent->id);
				mSelfInstanceId.store(static_cast<uint16_t>(pDestinationAgent->id), std::memory_order_relaxed);
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			if (pDestinationAgent->id == mSelfInstanceId.load(std::memory_order_relaxed))
			{
				LOG("Exiting combat since self agent was deregistered");
				mLocalState.ExitedCombat(timeGetTime());
				return;
			}
		}
		// Agent notification - not interesting
		return;
	}

	if (pEvent->is_statechange == CBTS_ENTERCOMBAT)
	{
		LOG("EnterCombat agent %s %llu %hu %u %llu",
			pSourceAgent->name, pSourceAgent->id, pEvent->src_master_instid, pSourceAgent->self, pEvent->dst_agent);

		if (pSourceAgent->self != 0)
		{
			mLocalState.EnteredCombat(pEvent->time, static_cast<uint16_t>(pEvent->dst_agent));
		}

		return;
	}
	else if (pEvent->is_statechange == CBTS_EXITCOMBAT)
	{
		LOG("ExitCombat agent %s %llu %hu %u",
			pSourceAgent->name, pSourceAgent->id, pEvent->src_master_instid, pSourceAgent->self);

		if (pSourceAgent->self != 0)
		{
			mLocalState.ExitedCombat(pEvent->time);
		}

		return;
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		// Not a HP modifying event - not interesting
		return;
	}

	if (pEvent->value <= 0 && pEvent->buff_dmg <= 0)
	{
		LOG("Damage event %s %u %u (%llu %s %s)->(%llu %s %s) iff=%hhu", pSkillname, pEvent->value, pEvent->buff_dmg, pSourceAgent->id, pSourceAgent->name, BOOL_STR(pSourceAgent->self), pDestinationAgent->id, pDestinationAgent->name, BOOL_STR(pDestinationAgent->self), pEvent->iff);

		if ((pSourceAgent->self != 0 || pDestinationAgent->self != 0) && pEvent->iff == IFF_FOE)
		{
			mLocalState.DamageEvent(pEvent);
		}

		return;
	}

	if (pSourceAgent->self == 0 &&
		pEvent->src_master_instid != mSelfInstanceId.load(std::memory_order_relaxed))
	{
		// Source is someone else - not interesting
		return;
	}

	if (pEvent->is_shields != 0)
	{
		// Shield application - not tracking for now
		return;
	}

	// Register agent if it's not already known
	if (pDestinationAgent->name != nullptr)
	{
		mAgentTable.AddAgent(pDestinationAgent->id, pEvent->dst_instid, pDestinationAgent->name, std::nullopt, pEvent->dst_master_instid != 0, std::nullopt);
	}

	mSkillTable->RegisterSkillName(pEvent->skillid, pSkillname);

	mLocalState.HealingEvent(pEvent, pDestinationAgent->id);

	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}
	LOG("Registered heal event id %llu size %i from %s:%u to %s:%llu", pId, healedAmount, mSkillTable->GetSkillName(pEvent->skillid), pEvent->skillid, pDestinationAgent->name, pDestinationAgent->id);
}

void EventProcessor::PeerCombat(cbtevent* pEvent, uint16_t pPeerInstanceId)
{
	assert(pEvent != nullptr);

	std::optional<uintptr_t> peerUniqueId = mAgentTable.GetUniqueId(pPeerInstanceId, false);
	if (peerUniqueId.has_value() == false)
	{
		return;
	}

	std::shared_ptr<PlayerStats> state;
	{
		std::lock_guard lock(mPeerStatesLock);
		
		auto [iter, inserted] = mPeerStates.try_emplace(*peerUniqueId);
		if (inserted == true)
		{
			LOG("Inserted peer state for %hu %llu", pPeerInstanceId, *peerUniqueId);
			iter->second = std::make_shared<PlayerStats>();
		}

		state = std::shared_ptr(iter->second);
	}

	if (pEvent->is_statechange == CBTS_ENTERCOMBAT)
	{
		LOG("EnterCombat agent %llu %hu %hu %llu",
			pEvent->src_agent, pEvent->src_instid, pEvent->src_master_instid, pEvent->dst_agent);

		if (pEvent->src_instid == pPeerInstanceId)
		{
			state->EnteredCombat(pEvent->time, static_cast<uint16_t>(pEvent->dst_agent));
		}
		return;
	}
	else if (pEvent->is_statechange == CBTS_EXITCOMBAT)
	{
		LOG("EnterCombat agent %llu %hu %hu %llu",
			pEvent->src_agent, pEvent->src_instid, pEvent->src_master_instid, pEvent->dst_agent);

		if (pEvent->src_instid == pPeerInstanceId)
		{
			state->ExitedCombat(pEvent->time);
		}
		return;
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		// Not a HP modifying event - not interesting
		return;
	}

	if (pEvent->value <= 0 && pEvent->buff_dmg <= 0)
	{
		LOG("Damage event %hu->%hu iff=%hhu", pEvent->src_instid, pEvent->dst_instid, pEvent->iff);

		if ((pEvent->src_instid == pPeerInstanceId || pEvent->dst_instid == pPeerInstanceId) && pEvent->iff == IFF_FOE)
		{
			state->DamageEvent(pEvent);
		}

		return;
	}

	if (pEvent->src_instid != pPeerInstanceId &&
		pEvent->src_master_instid != pPeerInstanceId)
	{
		// Source is someone else - not interesting
		return;
	}

	if (pEvent->is_shields != 0)
	{
		// Shield application - not tracking for now
		return;
	}

	std::optional<uintptr_t> dstUniqueId = mAgentTable.GetUniqueId(pEvent->dst_instid, true);
	if (dstUniqueId.has_value() == false)
	{
		LOG("Dropping heal event to %hu since destination agent is unknown", pEvent->dst_instid);
		return;
	}

	bool isMinion = (pEvent->dst_master_instid != 0);
	state->HealingEvent(pEvent, *dstUniqueId);

	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}
	LOG("Registered heal event size %i from %s:%u to %llu", healedAmount, mSkillTable->GetSkillName(pEvent->skillid), pEvent->skillid, *dstUniqueId);
}

HealingStats EventProcessor::GetLocalState()
{
	HealingStats result;
	*static_cast<HealingStatsSlim*>(&result) = mLocalState.GetState();

	result.CollectionTime = timeGetTime();
	result.Agents = mAgentTable.GetState();
	result.Skills = std::shared_ptr(mSkillTable);

	return result;
}

std::map<uintptr_t, HealingStats> EventProcessor::GetPeerStates()
{
	std::map<uintptr_t, HealingStats> result;

	std::map<uintptr_t, std::shared_ptr<PlayerStats>> peerStates;
	{
		std::lock_guard lock(mPeerStatesLock);
		peerStates = mPeerStates;
	}

	uint64_t collectionTime = timeGetTime();
	for (const auto& [uniqueId, state] : peerStates)
	{
		auto [entry, inserted] = result.try_emplace(uniqueId);

		*static_cast<HealingStatsSlim*>(&entry->second) = state->GetState();

		entry->second.CollectionTime = collectionTime;
		entry->second.Agents = mAgentTable.GetState();
		entry->second.Skills = std::shared_ptr(mSkillTable);

		LOG("peer %llu, %zu events", uniqueId, entry->second.Events.size());
	}

	return result;
}
