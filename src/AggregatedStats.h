#pragma once

#include "State.h"
#include "EventProcessor.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class GroupFilter
{
	Group = 0,
	Squad = 1,
	AllExcludingMinions = 2,
	All = 3,
	Max
};

struct AggregatedStatsEntry
{
	uint64_t Id;
	HealedAgent Agent;
	float TimeInCombat;

	uint64_t Healing;
	uint64_t Hits;
	std::optional<uint64_t> Casts;
	uint64_t BarrierGeneration;

	AggregatedStatsEntry(uint64_t pId, HealedAgent pAgent, float pTimeInCombat, uint64_t pHealing, uint64_t pHits, std::optional<uint64_t> pCasts, uint64_t pBarrierGeneration);

	auto GetTie() const
	{
		return std::tie(Id, Agent, TimeInCombat, Healing, Hits, Casts, BarrierGeneration);
	}
};

struct AggregatedVector
{
	std::vector<AggregatedStatsEntry> Entries;
	uint64_t HighestHealing{0};

	void Add(uint64_t pId, HealedAgent pAgent, float pTimeInCombat, uint64_t pHealing, uint64_t pHits, std::optional<uint64_t> pCasts, uint64_t pBarrierGeneration);
};


using TotalHealingStats = std::array<uint64_t, static_cast<size_t>(GroupFilter::Max)>;

constexpr static uint32_t IndirectHealingSkillId = 0;

class AggregatedStatsCollection;
class AggregatedStats
{
	friend AggregatedStatsCollection;
public:
	AggregatedStats(HealingStats&& pSourceData, const HealWindowOptions& pOptions, bool pDebugMode);

	const AggregatedStatsEntry& GetTotal();
	const AggregatedVector& GetStats(DataSource pDataSource);
	const AggregatedVector& GetDetails(DataSource pDataSource, uint64_t pId);

	const AggregatedVector& GetGroupFilterTotals();

	float GetCombatTime();

private:
	uint64_t GetCombatEnd();

	const AggregatedVector& GetAgents(std::optional<uint32_t> pSkillId);
	const AggregatedVector& GetSkills(std::optional<uintptr_t> pAgentId);

	static void Sort(std::vector<AggregatedStatsEntry>& pVector, SortOrder pSortOrder);

	bool Filter(uintptr_t pAgentId) const; // Returns true if agent should be filtered out
	bool Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const; // Returns true if agent should be filtered out
	bool FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, const HealWindowOptions& pFilter) const; // Returns true if agent should be filtered out

	HealingStats mySourceData;

	HealWindowOptions myOptions;
	bool myDebugMode;

	float myCombatTime = NAN;
	std::unique_ptr<AggregatedStatsEntry> myTotal;
	std::unique_ptr<AggregatedVector> myFilteredAgents;
	std::unique_ptr<AggregatedVector> mySkills;
	std::unique_ptr<AggregatedVector> myGroupFilterTotals;

	std::map<uintptr_t, AggregatedVector> myAgentsDetailed; // uintptr_t => agent id
	std::map<uint32_t, AggregatedVector> mySkillsDetailed; // uint32_t => skill id
};