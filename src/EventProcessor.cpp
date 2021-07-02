#include "AddonVersion.h"
#include "EventProcessor.h"
#include "Exports.h"
#include "Log.h"
#include "Skills.h"
#include "Utilities.h"

#include <cassert>
#include <cstddef>

EventProcessor::EventProcessor()
	: mSkillTable(std::make_shared<SkillTable>())
{
}

void EventProcessor::AreaCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t /*pId*/, uint64_t /*pRevision*/)
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
				mAgentTable.AddAgent(pSourceAgent->id, static_cast<uint16_t>(pDestinationAgent->id), pSourceAgent->name, pDestinationAgent->team, std::nullopt, isPlayer);
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			// If it's a player, process implicit combat exit
			if (pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0')
			{
				std::lock_guard lock(mPeerStatesLock);
				const auto iter = mPeerStates.find(pSourceAgent->id);
				if (iter != mPeerStates.end())
				{
					iter->second->ExitedCombat(timeGetTime());
					LogI("Implicit exit combat for peer unique_id={} character_name='{}' account_name='{}'", pSourceAgent->id, pSourceAgent->name, pDestinationAgent->name);
				}
			}
		}

		return;
	}

	if (pEvent->is_statechange == CBTS_LOGSTART)
	{
		LogI("CBTS_LOGSTART - server_timestamp={} local_timestamp={}, species_id={}", pEvent->value, pEvent->buff_dmg, pEvent->src_agent);

		cbtevent logEvent = {};

		constexpr char versionString[] = HEALING_STATS_VERSION;
		constexpr size_t versionStringLength = constexpr_strlen(versionString);

		EvtcVersionHeader versionHeader = {};
		static_assert(sizeof(versionHeader) == sizeof(logEvent.src_agent), "");
		versionHeader.Signature = HEALING_STATS_ADDON_SIGNATURE;

		versionHeader.EvtcRevision = HEALING_STATS_EVTC_REVISION;
		static_assert(HEALING_STATS_EVTC_REVISION <= 0x00ffffff, "Revision does not fit in cbtevent");

		versionHeader.VersionStringLength = versionStringLength;
		static_assert(versionStringLength <= UINT8_MAX, "Version string length does not fit in cbtevent");

		memcpy(&logEvent.src_agent, &versionHeader, sizeof(versionHeader));

		static_assert(versionStringLength <= (offsetof(cbtevent, is_statechange) - offsetof(cbtevent, dst_agent)), "Version string does not fit in cbtevent");
		memcpy(&logEvent.dst_agent, &versionString, versionStringLength);

		GlobalObjects::ARC_E9(&logEvent, VERSION_EVENT_SIGNATURE);

		return;
	}
	else if (pEvent->is_statechange == CBTS_EXTENSION)
	{
		uint32_t pad;
		memcpy(&pad, &pEvent->pad61, sizeof(pad));
		LogD("Extension event addon_signature={:#010x}", pad);
		if (pad == VERSION_EVENT_SIGNATURE)
		{
			EvtcVersionHeader versionHeader;
			memcpy(&versionHeader, &pEvent->src_agent, sizeof(pEvent->src_agent));
			LogI("Version event addon_signature={:#010x} addon_evtc_revision={} addon_version_string_length={} addon_version={}",
				+versionHeader.Signature, +versionHeader.EvtcRevision, +versionHeader.VersionStringLength, std::string_view{reinterpret_cast<char*>(&pEvent->dst_agent), versionHeader.VersionStringLength});
		}

		return;
	}
	else if (pEvent->is_statechange == CBTS_ENTERCOMBAT)
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

	if (pEvent->skillid != 0)
	{
		if (pSkillname != nullptr)
		{
			mSkillTable->RegisterSkillName(pEvent->skillid, pSkillname);
		}
		else
		{
			LogD("Got event with skillid {} but skillname is nullptr - is_statechange={} is_activation={} is_buffremove={} buff={} buff_dmg={} value={}",
				pEvent->skillid, pEvent->is_statechange, pEvent->is_activation, pEvent->is_buffremove, pEvent->buff, pEvent->buff_dmg, pEvent->value);
		}
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

void EventProcessor::LocalCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t /*pRevision*/)
{
	UNREFERENCED_PARAMETER(pId);
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
				mAgentTable.AddAgent(pSourceAgent->id, static_cast<uint16_t>(pDestinationAgent->id), pSourceAgent->name, pDestinationAgent->team, std::nullopt, isPlayer);
			}

			if (pDestinationAgent->self != 0)
			{
				assert(pDestinationAgent->id <= UINT16_MAX);

				LOG("Storing self instid=%llu uniqueid=%llu", pDestinationAgent->id, pSourceAgent->id);
				mSelfInstanceId.store(static_cast<uint16_t>(pDestinationAgent->id), std::memory_order_relaxed);
				mSelfUniqueId.store(pSourceAgent->id, std::memory_order_relaxed);
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

		std::lock_guard lock(mPeerStatesLock);
		for (auto& peer : mPeerStates)
		{
			std::optional<std::string> name_str = mAgentTable.GetName(peer.first);
			const char* name = "(unknown name)";
			if (name_str.has_value() == true)
			{
				name = name_str->c_str();
			}

			if (peer.second->ResetIfNotInCombat() == true)
			{
				LogD("Cleared stats for {} {} since self entered combat", peer.first, name);
			}
			else
			{
				LogD("Didn't clear stats for {} {} even though self entered combat - they are already in combat", peer.first, name);
			}
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

	if (pEvent->skillid != 0)
	{
		if (pSkillname != nullptr)
		{
			mSkillTable->RegisterSkillName(pEvent->skillid, pSkillname);
		}
		else
		{
			LogD("Got event with skillid {} but skillname is nullptr - is_statechange={} is_activation={} is_buffremove={} buff={} buff_dmg={} value={}",
				pEvent->skillid, pEvent->is_statechange, pEvent->is_activation, pEvent->is_buffremove, pEvent->buff, pEvent->buff_dmg, pEvent->value);
		}
	}

	if (pEvent->is_statechange != 0 || pEvent->is_activation != 0 || pEvent->is_buffremove != 0)
	{
		// Not a HP modifying event - not interesting
		return;
	}

	if (pEvent->value <= 0 && pEvent->buff_dmg <= 0)
	{
		LOG("Damage event %s %u %u (%llu %s %u)->(%llu %s %u) iff=%hhu", pSkillname, pEvent->value, pEvent->buff_dmg, pSourceAgent->id, pSourceAgent->name, pSourceAgent->self, pDestinationAgent->id, pDestinationAgent->name, pDestinationAgent->self, pEvent->iff);

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

	mLocalState.HealingEvent(pEvent, pDestinationAgent->id);

	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}
	LOG("Registered heal event id %llu size %i from %s:%u to %s:%llu", pId, healedAmount, mSkillTable->GetSkillName(pEvent->skillid), pEvent->skillid, pDestinationAgent->name, pDestinationAgent->id);

	cbtevent logEvent = *pEvent;

	// Flip event values so healed amount is negative
	logEvent.value *= -1;
	logEvent.buff_dmg *= -1;

	GlobalObjects::ARC_E9(&logEvent, 0x9c9b3c99);
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

	state->HealingEvent(pEvent, *dstUniqueId);

	uint32_t healedAmount = pEvent->value;
	if (healedAmount == 0)
	{
		healedAmount = pEvent->buff_dmg;
		assert(healedAmount != 0);
	}
	LOG("Registered heal event size %i from %s:%u to %llu", healedAmount, mSkillTable->GetSkillName(pEvent->skillid), pEvent->skillid, *dstUniqueId);

	cbtevent logEvent = *pEvent;

	// Fix unique ids (they are invalid when coming from a peer)
	logEvent.src_agent = *peerUniqueId;
	logEvent.dst_agent = *dstUniqueId;

	// Flip event values so healed amount is negative
	logEvent.value *= -1;
	logEvent.buff_dmg *= -1;

	GlobalObjects::ARC_E9(&logEvent, 0x9c9b3c99);
}

std::pair<uintptr_t, std::map<uintptr_t, std::pair<std::string, HealingStats>>> EventProcessor::GetState()
{
	std::map<uintptr_t, std::pair<std::string, HealingStats>> result;
	uint64_t collectionTime = timeGetTime();
	uintptr_t selfUniqueId = mSelfUniqueId.load(std::memory_order_relaxed);

	auto [localEntry, localInserted] = result.try_emplace(selfUniqueId);
	assert(localInserted == true);


	*static_cast<HealingStatsSlim*>(&localEntry->second.second) = mLocalState.GetState();
	localEntry->second.second.CollectionTime = collectionTime;
	localEntry->second.second.Agents = mAgentTable.GetState();
	localEntry->second.second.Skills = std::shared_ptr(mSkillTable);

	std::optional<std::string> localName = mAgentTable.GetName(selfUniqueId);
	if (localName.has_value())
	{
		localEntry->second.first = std::move(*localName);
	}
	else
	{
		localEntry->second.first = "local (unmapped)";
	}


	std::map<uintptr_t, std::shared_ptr<PlayerStats>> peerStates;
	{
		std::lock_guard lock(mPeerStatesLock);
		peerStates = mPeerStates;
	}

	for (const auto& [uniqueId, state] : peerStates)
	{
		auto [entry, inserted] = result.try_emplace(uniqueId);
		if (inserted == false)
		{
			LOG("Skipping agent %llu - emplacing failed", uniqueId);
			continue; // Only happens in unit tests when using the same agent twice
		}

		*static_cast<HealingStatsSlim*>(&entry->second.second) = state->GetState();

		entry->second.second.CollectionTime = collectionTime;
		entry->second.second.Agents = mAgentTable.GetState();
		entry->second.second.Skills = std::shared_ptr(mSkillTable);
		
		std::optional<std::string> name = mAgentTable.GetName(uniqueId);
		if (name.has_value())
		{
			entry->second.first = std::move(*name);
		}
		else
		{
			entry->second.first = "peer (unmapped)";
		}

		DEBUGLOG("peer %llu %s, %zu events", uniqueId, entry->second.first.c_str(), entry->second.second.Events.size());
	}

	DEBUGLOG("self %llu, %zu entries", selfUniqueId, result.size());
	return {selfUniqueId, result};
}
