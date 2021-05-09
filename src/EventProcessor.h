#pragma once
#include "arcdps_structs.h"
#include "AgentTable.h"
#include "PlayerStats.h"
#include "Skills.h"


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

	void AreaCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	void LocalCombat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	void PeerCombat(cbtevent* pEvent, uint16_t pPeerInstanceId);

	HealingStats GetLocalState();
	std::map<uintptr_t, HealingStats> GetPeerStates();

private:
	PlayerStats mLocalState;
	std::atomic<uint32_t> mSelfInstanceId = UINT32_MAX;

	AgentTable mAgentTable;
	std::shared_ptr<SkillTable> mSkillTable;

	std::mutex mPeerStatesLock;
	std::map<uintptr_t, std::shared_ptr<PlayerStats>> mPeerStates;
};