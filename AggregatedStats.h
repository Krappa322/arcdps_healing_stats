#pragma once

#include "PersonalStats.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

enum class SortOrder
{
	AscendingAlphabetical,
	DescendingAlphabetical,
	AscendingSize,
	DescendingSize,
	Max
};

enum class GroupFilter
{
	Group,
	Squad,
	AllExcludingMinions,
	All,
	Max
};

struct AggregatedStatsEntry
{
	std::string Name;
	float PerSecond;

	AggregatedStatsEntry(std::string&& pName, float pPerSecond);
};

using AggregatedStatsVector = std::vector<AggregatedStatsEntry>;
using TotalHealingStats = std::array<float, static_cast<size_t>(GroupFilter::Max)>;

class AggregatedStats
{
public:
	AggregatedStats(HealingStats&& pSourceData, SortOrder pSortOrder, GroupFilter pGroupFilter, bool pExcludeUnknownAgents);

	const AggregatedStatsVector& GetAgents();
	uint32_t GetLongestAgentName();

	const AggregatedStatsVector& GetSkills();
	uint32_t GetLongestSkillName();

	TotalHealingStats GetTotalHealing();

private:
	const std::map<uintptr_t, AgentStats>& GetAllAgents();

	void Sort(AggregatedStatsVector& pVector);

	bool Filter(uintptr_t pAgentId) const; // Returns true if agent should be filtered out
	bool Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const; // Returns true if agent should be filtered out
	bool FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, GroupFilter pFilter) const; // Returns true if agent should be filtered out

	HealingStats mySourceData;

	SortOrder mySortOrder;
	GroupFilter myGroupFilter;
	bool myExcludeUnknownAgents;

	std::unique_ptr<std::map<uintptr_t, AgentStats>> myAllAgents;

	std::unique_ptr<AggregatedStatsVector> myFilteredAgents;
	uint32_t myLongestAgentName;

	std::unique_ptr<AggregatedStatsVector> mySkills;
	uint32_t myLongestSkillName;
};