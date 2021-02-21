#pragma once

#include "Options.h"
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

struct AggregatedStatsSkill
{
	uint32_t Id;
	std::string Name;
	float PerSecond;

	AggregatedStatsSkill(uint32_t pSkillId, std::string&& pName, float pPerSecond);
};

struct AggregatedStatsAgent
{
	uintptr_t Id;
	std::string Name;
	float PerSecond;

	AggregatedStatsAgent(uintptr_t pAgentId, std::string&& pName, float pPerSecond);
};

using AggregatedVectorSkills = std::vector<AggregatedStatsSkill>;
using AggregatedVectorAgents = std::vector<AggregatedStatsAgent>;
using TotalHealingStats = std::array<float, static_cast<size_t>(GroupFilter::Max)>;

constexpr static uint32_t IndirectHealingSkillId = 0;

class AggregatedStats
{
public:
	AggregatedStats(HealingStats&& pSourceData, const HealTableOptions& pOptions);

	const AggregatedVectorAgents& GetAgents();
	const AggregatedVectorSkills& GetSkills();

	const AggregatedVectorSkills& GetAgentDetails(uintptr_t pAgentId);
	const AggregatedVectorAgents& GetSkillDetails(uint32_t pSkillId);

	TotalHealingStats GetTotalHealing();

private:
	const std::map<uintptr_t, AgentStats>& GetAllAgents();

	template<typename VectorType>
	void Sort(VectorType& pVector);

	bool Filter(uintptr_t pAgentId) const; // Returns true if agent should be filtered out
	bool Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const; // Returns true if agent should be filtered out
	bool FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, GroupFilter pFilter) const; // Returns true if agent should be filtered out

	HealingStats mySourceData;

	HealTableOptions myOptions;

	std::unique_ptr<std::map<uintptr_t, AgentStats>> myAllAgents; // uintptr_t => agent id

	std::unique_ptr<AggregatedVectorAgents> myFilteredAgents;
	std::unique_ptr<AggregatedVectorSkills> mySkills;

	std::map<uintptr_t, AggregatedVectorSkills> myAgentsDetailed; // uintptr_t => agent id
	std::map<uint32_t, AggregatedVectorAgents> mySkillsDetailed; // uint32_t => skill id
};