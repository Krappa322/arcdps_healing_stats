#include "AggregatedStats.h"

#include "Log.h"

#include <assert.h>
#include <Windows.h>

#include <algorithm>
#include <map>

AggregatedStatsEntry::AggregatedStatsEntry(std::string&& pName, float pPerSecond)
	: Name(pName)
	, PerSecond(pPerSecond)
{
}

AggregatedStats::AggregatedStats(HealingStats&& pSourceData, SortOrder pSortOrder, GroupFilter pGroupFilter, bool pExcludeUnknownAgents)
	: mySourceData(std::move(pSourceData))
	, mySortOrder(pSortOrder)
	, myGroupFilter(pGroupFilter)
	, myExcludeUnknownAgents(pExcludeUnknownAgents)
	, myAllAgents(nullptr)
	, myFilteredAgents(nullptr)
	, myLongestAgentName(0)
	, mySkills(nullptr)
	, myLongestSkillName(0)
{
	assert(mySortOrder < SortOrder::Max);
	assert(myGroupFilter < GroupFilter::Max);

	if (mySourceData.TimeInCombat == 0)
	{
		mySourceData.TimeInCombat = 1; // Can't handle zero time right now - divide by zero issues etc.
	}
}

const AggregatedStatsVector& AggregatedStats::GetAgents()
{
	if (myFilteredAgents != nullptr)
	{
		return *myFilteredAgents;
	}

	//LOG("Generating new agents vector");
	myLongestAgentName = 0;
	myFilteredAgents = std::make_unique<AggregatedStatsVector>();

	// Caching the result in a display friendly way
	for (const auto& [agentId, agent] : GetAllAgents())
	{
		std::string agentName;

		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);
		if (Filter(mapAgent) == true)
		{
			continue; // Exclude agent
		}

		if (mapAgent != mySourceData.Agents.end())
		{
			agentName = mapAgent->second.Name;
		}
		else
		{
			LOG("Couldn't find a name for agent %llu", agentId);
			agentName = std::to_string(agentId);
		}

		float perSecond = agent.TotalHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		uint32_t nameLength = static_cast<uint32_t>(agentName.size());
		if (nameLength > myLongestAgentName)
		{
			myLongestAgentName = nameLength;
		}

		myFilteredAgents->emplace_back(std::move(agentName), std::move(perSecond));
	}

	Sort(*myFilteredAgents);

	return *myFilteredAgents;
}

uint32_t AggregatedStats::GetLongestAgentName()
{
	return myLongestAgentName;
}

const AggregatedStatsVector& AggregatedStats::GetSkills()
{
	if (mySkills != nullptr)
	{
		return *mySkills;
	}

	//LOG("Generating new skills vector");
	myLongestSkillName = 0;
	mySkills = std::make_unique<AggregatedStatsVector>();

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

		std::string skillName(skill.Name);
		float perSecond = totalHealing / (static_cast<float>(mySourceData.TimeInCombat) / 1000);

		uint32_t nameLength = static_cast<uint32_t>(skillName.size());
		if (nameLength > myLongestSkillName)
		{
			myLongestSkillName = nameLength;
		}

		mySkills->emplace_back(std::move(skillName), std::move(perSecond));
	}

	Sort(*mySkills);

	return *mySkills;
}

uint32_t AggregatedStats::GetLongestSkillName()
{
	return myLongestSkillName;
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

void AggregatedStats::Sort(AggregatedStatsVector& pVector)
{
	switch (mySortOrder)
	{
	case SortOrder::AscendingAlphabetical:
		std::sort(pVector.begin(), pVector.end(),
			[](const AggregatedStatsEntry& pLeft, const AggregatedStatsEntry& pRight)
			{
				return pLeft.Name < pRight.Name;
			});
		break;

	case SortOrder::DescendingAlphabetical:
		std::sort(pVector.begin(), pVector.end(),
			[](const AggregatedStatsEntry& pLeft, const AggregatedStatsEntry& pRight)
			{
				return pLeft.Name > pRight.Name;
			});
		break;

	case SortOrder::AscendingSize:
		std::sort(pVector.begin(), pVector.end(),
			[](const AggregatedStatsEntry& pLeft, const AggregatedStatsEntry& pRight)
			{
				return pLeft.PerSecond < pRight.PerSecond;
			});
		break;

	case SortOrder::DescendingSize:
		std::sort(pVector.begin(), pVector.end(),
			[](const AggregatedStatsEntry& pLeft, const AggregatedStatsEntry& pRight)
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
	return FilterInternal(agent, myGroupFilter);
}

bool AggregatedStats::Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const
{
	return FilterInternal(pAgent, myGroupFilter);
}

bool AggregatedStats::FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, GroupFilter pFilter) const
{
	if (pAgent == mySourceData.Agents.end())
	{
		if (myExcludeUnknownAgents == true)
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
		if (pAgent->second.Subgroup == 0) // Subgroup 0 => not in squad
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
