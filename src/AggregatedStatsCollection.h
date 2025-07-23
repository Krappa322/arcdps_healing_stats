#pragma once
#include "AggregatedStats.h"

class AggregatedStatsCollection
{
	struct Player
	{
		Player(HealedAgent&& pAgent, HealingStats&& pStats, const HealWindowOptions& pOptions, bool pDebugMode);

		AggregatedStats Stats;
		HealedAgent Agent;
	};

public:
	AggregatedStatsCollection(std::map<uintptr_t, std::pair<HealedAgent, HealingStats>>&& pPeerStates, uintptr_t pLocalUniqueId, const HealWindowOptions& pOptions, bool pDebugMode);

	const AggregatedStatsEntry& GetTotal(DataSource pDataSource);
	const AggregatedVector& GetStats(DataSource pDataSource);
	const AggregatedVector& GetDetails(DataSource pDataSource, uint64_t pId);

	const AggregatedVector& GetGroupFilterTotals();

	float GetCombatTime();

private:
	std::unique_ptr<AggregatedVector> mPeersOutgoingStats;
	std::unique_ptr<AggregatedStatsEntry> mPeersOutgoingTotal;

	std::map<uintptr_t, Player>::iterator mLocalState;
	std::map<uintptr_t, Player> mSourceData;
	const HealWindowOptions mOptions;
	const bool mDebugMode;
};