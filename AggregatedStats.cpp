#include "AggregatedStats.h"

#include "Log.h"
#include "Skills.h"
#include "Utilities.h"

#include <assert.h>
#include <Windows.h>

#include <algorithm>
#include <map>

AggregatedStatsSkill::AggregatedStatsSkill(uint32_t pSkillId, std::string&& pName, float pPerSecond)
	: Id{pSkillId}
	, Name{pName}
	, PerSecond{pPerSecond}
{
}

AggregatedStatsAgent::AggregatedStatsAgent(uintptr_t pAgentId, std::string&& pName, float pPerSecond)
	: Id{pAgentId}
	, Name{pName}
	, PerSecond{pPerSecond}
{
}

AggregatedStats::AggregatedStats(HealingStats&& pSourceData, const HealTableOptions& pOptions)
	: mySourceData(std::move(pSourceData))
	, myOptions(pOptions)
	, myAllAgents(nullptr)
	, myFilteredAgents(nullptr)
	, mySkills(nullptr)
{
	assert(static_cast<SortOrder>(myOptions.SortOrderChoice) < SortOrder::Max);
	assert(static_cast<GroupFilter>(myOptions.GroupFilterChoice) < GroupFilter::Max);

	if (mySourceData.TimeInCombat == 0)
	{
		mySourceData.TimeInCombat = 1; // Can't handle zero time right now - divide by zero issues etc.
	}
}

const AggregatedVectorAgents& AggregatedStats::GetAgents()
{
	if (myFilteredAgents != nullptr)
	{
		return *myFilteredAgents;
	}

	//LOG("Generating new agents vector");
	myFilteredAgents = std::make_unique<AggregatedVectorAgents>();

	// Caching the result in a display friendly way
	for (const auto& [agentId, agent] : GetAllAgents())
	{
		std::string agentName;

		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);
		if (Filter(mapAgent) == true)
		{
			continue; // Exclude agent
		}

		if (myOptions.DebugMode == false)
		{
			if (mapAgent != mySourceData.Agents.end())
			{
				agentName = mapAgent->second.Name;
			}
			else
			{
				LOG("Couldn't find a name for agent %llu", agentId);
				agentName = std::to_string(agentId);
			}
		}
		else
		{
			char buffer[1024];
			if (mapAgent != mySourceData.Agents.end())
			{
				snprintf(buffer, sizeof(buffer), "%llu ; %u ; %u ; %s", agentId, mapAgent->second.Subgroup, mapAgent->second.IsMinion, mapAgent->second.Name.c_str());
			}
			else
			{
				snprintf(buffer, sizeof(buffer), "%llu ; (UNMAPPED)", agentId);
			}

			agentName = buffer;
		}

		float perSecond = agent.TotalHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		myFilteredAgents->emplace_back(agentId, std::move(agentName), perSecond);
	}

	Sort(*myFilteredAgents);

	return *myFilteredAgents;
}

const AggregatedVectorSkills& AggregatedStats::GetSkills()
{
	if (mySkills != nullptr)
	{
		return *mySkills;
	}

	//LOG("Generating new skills vector");
	mySkills = std::make_unique<AggregatedVectorSkills>();

	uint64_t totalIndirectHealing = 0;
	uint64_t totalIndirectTicks = 0;

	for (const auto& [skillId, skill] : mySourceData.SkillsHealing)
	{
		uint64_t totalHealing = 0;
		uint64_t ticks = 0;

		for (const auto& [agentId, agent] : skill.AgentsHealing)
		{
			if (Filter(agentId) == true)
			{
				continue; // Skip this agent since it doesn't match filter
			}

			totalHealing += agent.TotalHealing;
			ticks += agent.Ticks;
		}

		bool isIndirectHealing = false;
		if (SkillTable::GlobalState.IsSkillIndirectHealing(skillId, skill.Name) == true)
		{
			totalIndirectHealing += totalHealing;
			totalIndirectTicks += ticks;

			LOG("Translating skill %hu %s to indirect healing", skillId, skill.Name);
			isIndirectHealing = true;

			if (myOptions.DebugMode == false)
			{
				continue;
			}
		}

		float perSecond = totalHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		std::string skillName;
		if (myOptions.DebugMode == false)
		{
			skillName = skill.Name;
		}
		else
		{
			char buffer[1024];
			snprintf(buffer, sizeof(buffer), "%s%u ; %s", isIndirectHealing ? "(INDIRECT) ; " : "", skillId, skill.Name);
			skillName = buffer;
		}

		mySkills->emplace_back(skillId, std::move(skillName), perSecond);
	}

	if (totalIndirectHealing != 0 || totalIndirectTicks != 0)
	{
		float perSecond = totalIndirectHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		std::string skillName("Healing by Damage Dealt");

		mySkills->emplace_back(IndirectHealingSkillId, std::move(skillName), perSecond);
	}

	Sort(*mySkills);

	return *mySkills;
}

const AggregatedVectorAgents& AggregatedStats::GetSkillDetails(uint32_t pSkillId)
{
	const auto [entry, inserted] = mySkillsDetailed.emplace(std::piecewise_construct,
		std::forward_as_tuple(pSkillId),
		std::forward_as_tuple());
	if (inserted == false)
	{
		return entry->second; // Return cached value
	}

	const auto sourceData = mySourceData.SkillsHealing.find(pSkillId);
	if (sourceData == mySourceData.SkillsHealing.end())
	{
		// This should never happen
		LOG("Couldn't find source data for skill %u", pSkillId);

		return entry->second; // Just return the empty vector
	}

	for (const auto& [agentId, agent] : sourceData->second.AgentsHealing)
	{
		std::string agentName;

		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);
		if (Filter(mapAgent) == true)
		{
			continue; // Exclude agent
		}

		if (myOptions.DebugMode == false)
		{
			if (mapAgent != mySourceData.Agents.end())
			{
				agentName = mapAgent->second.Name;
			}
			else
			{
				LOG("Couldn't find a name for agent %llu", agentId);
				agentName = std::to_string(agentId);
			}
		}
		else
		{
			char buffer[1024];
			if (mapAgent != mySourceData.Agents.end())
			{
				snprintf(buffer, sizeof(buffer), "%llu ; %u ; %u ; %s", agentId, mapAgent->second.Subgroup, mapAgent->second.IsMinion, mapAgent->second.Name.c_str());
			}
			else
			{
				snprintf(buffer, sizeof(buffer), "%llu ; (UNMAPPED)", agentId);
			}

			agentName = buffer;
		}

		float perSecond = agent.TotalHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		entry->second.emplace_back(agentId, std::move(agentName), std::move(perSecond));
	}

	return entry->second;
}

TotalHealingStats AggregatedStats::GetTotalHealing()
{
	TotalHealingStats stats;
	for (auto& i : stats)
	{
		i = 0.0;
	}

	for (const auto& [agentId, agent] : GetAllAgents())
	{
		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);

		// Loop through the array and pretend index is GroupFilter, if agent does not get filtered by that filter then add
		// the total healing to that agent to the total for that filter
		for (size_t i = 0; i < stats.size(); i++)
		{
			if (FilterInternal(mapAgent, static_cast<GroupFilter>(i)) == false)
			{
				stats[i] += agent.TotalHealing;
			}
		}
	}

	// Divide the total number by time in combat to make it per second instead
	for (auto& i : stats)
	{
		i /= (static_cast<float>(mySourceData.TimeInCombat) / 1000);
	}

	return stats;
}

const std::map<uintptr_t, AgentStats>& AggregatedStats::GetAllAgents()
{
	if (myAllAgents != nullptr)
	{
		return *myAllAgents;
	}

	//LOG("Generating new skills vector");
	myAllAgents = std::make_unique<std::map<uintptr_t, AgentStats>>();

	for (const auto& [skillId, skill] : mySourceData.SkillsHealing)
	{
		for (const auto& [agentId, agent] : skill.AgentsHealing)
		{
			auto result = myAllAgents->emplace(std::piecewise_construct,
				std::forward_as_tuple(agentId),
				std::forward_as_tuple(agent.TotalHealing, agent.Ticks));
			if (result.second == false) // Didn't insert new entry
			{
				result.first->second.TotalHealing += agent.TotalHealing;
				result.first->second.Ticks += agent.Ticks;
			}
		}
	}

	return *myAllAgents;
}

template<typename VectorType>
void AggregatedStats::Sort(VectorType& pVector)
{
	switch (static_cast<SortOrder>(myOptions.SortOrderChoice))
	{
	case SortOrder::AscendingAlphabetical:
		std::sort(pVector.begin(), pVector.end(),
			[](const auto& pLeft, const auto& pRight)
			{
				return pLeft.Name < pRight.Name;
			});
		break;

	case SortOrder::DescendingAlphabetical:
		std::sort(pVector.begin(), pVector.end(),
			[](const auto& pLeft, const auto& pRight)
			{
				return pLeft.Name > pRight.Name;
			});
		break;

	case SortOrder::AscendingSize:
		std::sort(pVector.begin(), pVector.end(),
			[](const auto& pLeft, const auto& pRight)
			{
				return pLeft.PerSecond < pRight.PerSecond;
			});
		break;

	case SortOrder::DescendingSize:
		std::sort(pVector.begin(), pVector.end(),
			[](const auto& pLeft, const auto& pRight)
			{
				return pLeft.PerSecond > pRight.PerSecond;
			});
		break;

	default:
		assert(false);
	}
}

bool AggregatedStats::Filter(uintptr_t pAgentId) const
{
	std::map<uintptr_t, HealedAgent>::const_iterator agent = mySourceData.Agents.find(pAgentId);
	return FilterInternal(agent, static_cast<GroupFilter>(myOptions.GroupFilterChoice));
}

bool AggregatedStats::Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const
{
	return FilterInternal(pAgent, static_cast<GroupFilter>(myOptions.GroupFilterChoice));
}

bool AggregatedStats::FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, GroupFilter pFilter) const
{
	if (pAgent == mySourceData.Agents.end())
	{
		if (myOptions.ExcludeUnmappedAgents == true)
		{
			return true;
		}
		else
		{
			return false; // Include unmapped agents regardless of filter
		}
	}

	switch (pFilter)
	{
	case GroupFilter::Group:
		if (pAgent->second.Subgroup != mySourceData.SubGroup)
		{
			return true;
		}
		break;

	case GroupFilter::Squad:
		if (pAgent->second.Subgroup == 0 && mySourceData.SubGroup != 0) // Subgroup 0 => not in squad
		{
			return true;
		}
		break;

	case GroupFilter::AllExcludingMinions:
		if (pAgent->second.IsMinion == true)
		{
			return true;
		}
		break;

	case GroupFilter::All:
		break; // No filtering

	default:
		assert(false);
	}

	return false;
}
