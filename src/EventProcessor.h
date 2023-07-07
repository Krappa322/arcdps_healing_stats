#pragma once
#include "arcdps_structs.h"
#include "AgentTable.h"
#include "PlayerStats.h"
#include "Skills.h"

#include <optional>

struct HealingStats : HealingStatsSlim
{
	uint64_t CollectionTime = 0;

	std::map<uintptr_t, HealedAgent> Agents; // <Unique Id, Agent>
	std::shared_ptr<SkillTable> Skills; // <Skill Id, Skillname>
};

class EventProcessor
{
public:
	EventProcessor();

	void SetEvtcLoggingEnabled(bool pEnabled);
	void SetUseBarrier(bool pEnabled);

	void AreaCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	void LocalCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision, std::optional<cbtevent>* pModifiedEvent = nullptr);
	void PeerCombat(cbtevent* pEvent, uint16_t pPeerInstanceId);

	// Returns <local unique id, map<unique id, <name, agent state>>
	// pSelfUniqueId is only specified in testing
	std::pair<uintptr_t, std::map<uintptr_t, std::pair<std::string, HealingStats>>> GetState(uintptr_t pSelfUniqueId = 0);

#ifndef TEST
private:
#endif
	PlayerStats mLocalState;
	std::atomic<uint32_t> mSelfInstanceId = UINT32_MAX;
	std::atomic<uintptr_t> mSelfUniqueId = UINT64_MAX;

	AgentTable mAgentTable;
	std::shared_ptr<SkillTable> mSkillTable;

	std::mutex mPeerStatesLock;
	std::map<uintptr_t, std::shared_ptr<PlayerStats>> mPeerStates;

	std::atomic_bool mEvtcLoggingEnabled = false;
	std::atomic_bool useBarrier = false;
};