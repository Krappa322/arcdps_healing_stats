#include "AggregatedStatsCollection.h"

#include <cassert>

const static AggregatedVector EMPTY_STATS;

AggregatedStatsCollection::Player::Player(std::string&& pName, HealingStats&& pStats, const HealWindowOptions& pOptions, bool pDebugMode)
	: Name{std::move(pName)}
	, Stats{std::move(pStats), pOptions, pDebugMode}
{
}

AggregatedStatsCollection::AggregatedStatsCollection(std::map<uintptr_t, std::pair<std::string, HealingStats>>&& pPeerStates, uintptr_t pLocalUniqueId, const HealWindowOptions& pOptions, bool pDebugMode)
	: mOptions{pOptions}
	, mDebugMode{pDebugMode}
{
	mLocalState = mSourceData.end();
	for (auto& [id, state] : pPeerStates)
	{
		auto [iter, inserted] = mSourceData.try_emplace(id, std::move(state.first), std::move(state.second), pOptions, pDebugMode);
		assert(inserted == true);
		if (id == pLocalUniqueId)
		{
			mLocalState = iter;
		}
	}

	assert(mLocalState != mSourceData.end());
}

const AggregatedStatsEntry& AggregatedStatsCollection::GetTotal(DataSource pDataSource)
{
	if (pDataSource != DataSource::PeersOutgoing)
	{
		return mLocalState->second.Stats.GetTotal();
	}

	if (mPeersOutgoingTotal != nullptr)
	{
		return *mPeersOutgoingTotal;
	}

	uint64_t healing = 0;
	uint64_t hits = 0;
	uint64_t barrierGeneration = 0;
	const AggregatedVector& sourceStats = GetStats(pDataSource);

	for (const AggregatedStatsEntry& entry : sourceStats.Entries)
	{
		healing += entry.Healing;
		hits += entry.Hits;
		barrierGeneration += entry.BarrierGeneration;
	}

	mPeersOutgoingTotal = std::make_unique<AggregatedStatsEntry>(0, "__SUPERTOTAL__", mLocalState->second.Stats.GetCombatTime(), healing, hits, std::nullopt, barrierGeneration);
	return *mPeersOutgoingTotal;
}

const AggregatedVector& AggregatedStatsCollection::GetStats(DataSource pDataSource)
{
	if (pDataSource != DataSource::PeersOutgoing)
	{
		return mLocalState->second.Stats.GetStats(pDataSource);
	}

	if (mPeersOutgoingStats != nullptr)
	{
		return *mPeersOutgoingStats;
	}

	mPeersOutgoingStats = std::make_unique<AggregatedVector>();

	for (auto& [id, source] : mSourceData)
	{
		const AggregatedStatsEntry& entry = source.Stats.GetTotal();
		mPeersOutgoingStats->Add(id, std::string{source.Name}, source.Stats.GetCombatTime(), entry.Healing, entry.Hits, entry.Casts, entry.BarrierGeneration);
	}

	AggregatedStats::Sort(mPeersOutgoingStats->Entries, mOptions.SortOrderChoice);
	return *mPeersOutgoingStats;
}

const AggregatedVector& AggregatedStatsCollection::GetDetails(DataSource pDataSource, uint64_t pId)
{
	if (pDataSource != DataSource::PeersOutgoing)
	{
		return mLocalState->second.Stats.GetDetails(pDataSource, pId);
	}

	auto iter = mSourceData.find(pId);
	if (iter == mSourceData.end())
	{
		return EMPTY_STATS;
	}

	return iter->second.Stats.GetStats(DataSource::Skills);
}

const AggregatedVector& AggregatedStatsCollection::GetGroupFilterTotals()
{
	return mLocalState->second.Stats.GetGroupFilterTotals();
}

float AggregatedStatsCollection::GetCombatTime()
{
	return mLocalState->second.Stats.GetCombatTime();
}
