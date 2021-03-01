#include "AggregatedStats.h"

#include "Log.h"
#include "Skills.h"
#include "Utilities.h"

#include <assert.h>
#include <Windows.h>

#include <algorithm>
#include <map>

AggregatedStatsEntry::AggregatedStatsEntry(uint64_t pId, std::string&& pName, uint64_t pHealing, uint64_t pHits, std::optional<uint64_t> pCasts)
	: Id{pId}
	, Name{pName}
	, Healing{pHealing}
	, Hits{pHits}
	, Casts{pCasts}
{
}

AggregatedStats::AggregatedStats(HealingStats&& pSourceData, const HealWindowOptions& pOptions, bool pDebugMode)
	: mySourceData(std::move(pSourceData))
	, myOptions(pOptions)
	, myAllAgents(nullptr)
	, myFilteredAgents(nullptr)
	, mySkills(nullptr)
	, myDebugMode(pDebugMode)
{
	assert(static_cast<SortOrder>(myOptions.SortOrderChoice) < SortOrder::Max);
	assert(static_cast<DataSource>(myOptions.DataSourceChoice) < DataSource::Max);

	if (mySourceData.TimeInCombat == 0)
	{
		mySourceData.TimeInCombat = 1; // Can't handle zero time right now - divide by zero issues etc.
	}
}

const AggregatedStatsEntry& AggregatedStats::GetTotal()
{
	if (myTotal != nullptr)
	{
		return *myTotal;
	}

	uint64_t healing = 0;
	uint64_t hits = 0;
	for (const AggregatedStatsEntry& entry : GetStats())
	{
		healing += entry.Healing;
		hits += entry.Hits;
	}

	myTotal = std::make_unique<AggregatedStatsEntry>(0, "__TOTAL__", healing, hits, std::nullopt);
	return *myTotal;
}

const AggregatedVector& AggregatedStats::GetStats()
{
	if (static_cast<DataSource>(myOptions.DataSourceChoice) == DataSource::Skills)
	{
		return GetSkills();
	}
	else
	{
		return GetAgents();
	}
}

const AggregatedVector& AggregatedStats::GetDetails(uint64_t pId)
{
	if (static_cast<DataSource>(myOptions.DataSourceChoice) == DataSource::Skills)
	{
		return GetSkillDetails(static_cast<uint32_t>(pId));
	}
	else
	{
		return GetAgentDetails(pId);
	}
}

uint64_t AggregatedStats::GetCombatTime()
{
	return mySourceData.TimeInCombat / 1000;
}

const AggregatedVector& AggregatedStats::GetAgents()
{
	if (myFilteredAgents != nullptr)
	{
		return *myFilteredAgents;
	}

	//LOG("Generating new agents vector");
	myFilteredAgents = std::make_unique<AggregatedVector>();

	// Caching the result in a display friendly way
	for (const auto& [agentId, agent] : GetAllAgents())
	{
		std::string agentName;

		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);
		if (Filter(mapAgent) == true)
		{
			continue; // Exclude agent
		}

		if (myDebugMode == false)
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

		myFilteredAgents->emplace_back(agentId, std::move(agentName), agent.TotalHealing, agent.Ticks, std::nullopt);
	}

	Sort(*myFilteredAgents);

	return *myFilteredAgents;
}

const AggregatedVector& AggregatedStats::GetSkills()
{
	if (mySkills != nullptr)
	{
		return *mySkills;
	}

	//LOG("Generating new skills vector");
	mySkills = std::make_unique<AggregatedVector>();

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
			LOG("Translating skill %hu %s to indirect healing", skillId, skill.Name);

			totalIndirectHealing += totalHealing;
			totalIndirectTicks += ticks;
			isIndirectHealing = true;

			if (myDebugMode == false)
			{
				continue;
			}
		}

		std::string skillName;
		if (myDebugMode == false)
		{
			skillName = skill.Name;
		}
		else
		{
			char buffer[1024];
			snprintf(buffer, sizeof(buffer), "%s%u ; %s", isIndirectHealing ? "(INDIRECT) ; " : "", skillId, skill.Name);
			skillName = buffer;
		}

		mySkills->emplace_back(skillId, std::move(skillName), totalHealing, ticks, std::nullopt);
	}

	if (totalIndirectHealing != 0 || totalIndirectTicks != 0)
	{
		std::string skillName("Healing by Damage Dealt");

		mySkills->emplace_back(IndirectHealingSkillId, std::move(skillName), totalIndirectHealing, totalIndirectTicks, std::nullopt);
	}

	Sort(*mySkills);

	return *mySkills;
}

const AggregatedVector& AggregatedStats::GetAgentDetails(uintptr_t pAgentId)
{
	const auto [entry, inserted] = myAgentsDetailed.emplace(std::piecewise_construct,
		std::forward_as_tuple(pAgentId),
		std::forward_as_tuple());
	if (inserted == false)
	{
		return entry->second; // Return cached value
	}

	uint64_t totalIndirectHealing = 0;
	uint64_t totalIndirectTicks = 0;

	for (const auto& [skillId, skill] : mySourceData.SkillsHealing)
	{
		for (const auto& [agentId, agent] : skill.AgentsHealing)
		{
			if (agentId != pAgentId)
			{
				continue;
			}

			bool isIndirectHealing = false;
			if (SkillTable::GlobalState.IsSkillIndirectHealing(skillId, skill.Name) == true)
			{
				LOG("Translating skill %hu %s to indirect healing", skillId, skill.Name);

				totalIndirectHealing += agent.TotalHealing;
				totalIndirectTicks += agent.Ticks;
				isIndirectHealing = true;

				if (myDebugMode == false)
				{
					continue;
				}
			}

			std::string skillName;
			if (myDebugMode == false)
			{
				skillName = skill.Name;
			}
			else
			{
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "%s%u ; %s", isIndirectHealing ? "(INDIRECT) ; " : "", skillId, skill.Name);
				skillName = buffer;
			}

			entry->second.emplace_back(skillId, std::move(skillName), agent.TotalHealing, agent.Ticks, std::nullopt);
		}
	}

	if (totalIndirectHealing != 0 || totalIndirectTicks != 0)
	{
		std::string skillName("Healing by Damage Dealt");

		entry->second.emplace_back(IndirectHealingSkillId, std::move(skillName), totalIndirectHealing, totalIndirectTicks, std::nullopt);
	}

	Sort(entry->second);

	return entry->second;
}

const AggregatedVector& AggregatedStats::GetSkillDetails(uint32_t pSkillId)
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

		if (myDebugMode == false)
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

		entry->second.emplace_back(agentId, std::move(agentName), agent.TotalHealing, agent.Ticks, std::nullopt);
	}

	Sort(entry->second);

	return entry->second;
}

TotalHealingStats AggregatedStats::GetTotalHealing()
{
	TotalHealingStats stats;
	for (auto& i : stats)
	{
		i = 0.0;
	}

	HealWindowOptions fakeOptions;

	for (const auto& [agentId, agent] : GetAllAgents())
	{
		auto mapAgent = std::as_const(mySourceData.Agents).find(agentId);

		// Loop through the array and pretend index is GroupFilter, if agent does not get filtered by that filter then add
		// the total healing to that agent to the total for that filter
		for (size_t i = 0; i < stats.size(); i++)
		{
			switch (static_cast<GroupFilter>(i))
			{
			case GroupFilter::Group:
				fakeOptions.ExcludeGroup = false;
				fakeOptions.ExcludeOffGroup = true;
				fakeOptions.ExcludeOffSquad = true;
				fakeOptions.ExcludeMinions = true;
				fakeOptions.ExcludeUnmapped = true;
				break;
			case GroupFilter::Squad:
				fakeOptions.ExcludeGroup = false;
				fakeOptions.ExcludeOffGroup = false;
				fakeOptions.ExcludeOffSquad = true;
				fakeOptions.ExcludeMinions = true;
				fakeOptions.ExcludeUnmapped = true;
				break;
			case GroupFilter::AllExcludingMinions:
				fakeOptions.ExcludeGroup = false;
				fakeOptions.ExcludeOffGroup = false;
				fakeOptions.ExcludeOffSquad = false;
				fakeOptions.ExcludeMinions = true;
				fakeOptions.ExcludeUnmapped = true;
				break;
			case GroupFilter::All:
				fakeOptions.ExcludeGroup = false;
				fakeOptions.ExcludeOffGroup = false;
				fakeOptions.ExcludeOffSquad = false;
				fakeOptions.ExcludeMinions = false;
				fakeOptions.ExcludeUnmapped = true;
				break;
			default:
				assert(false);
			}

			if (FilterInternal(mapAgent, fakeOptions) == false)
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
				return pLeft.Healing < pRight.Healing;
			});
		break;

	case SortOrder::DescendingSize:
		std::sort(pVector.begin(), pVector.end(),
			[](const auto& pLeft, const auto& pRight)
			{
				return pLeft.Healing > pRight.Healing;
			});
		break;

	default:
		assert(false);
	}
}

bool AggregatedStats::Filter(uintptr_t pAgentId) const
{
	std::map<uintptr_t, HealedAgent>::const_iterator agent = mySourceData.Agents.find(pAgentId);
	return FilterInternal(agent, myOptions);
}

bool AggregatedStats::Filter(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent) const
{
	return FilterInternal(pAgent, myOptions);
}

bool AggregatedStats::FilterInternal(std::map<uintptr_t, HealedAgent>::const_iterator& pAgent, const HealWindowOptions& pFilter) const
{
	if (pAgent == mySourceData.Agents.end())
	{
		if (pFilter.ExcludeUnmapped == true)
		{
			return true;
		}
		else
		{
			return false; // Include unmapped agents regardless of filter
		}
	}

	if (pAgent->second.IsMinion == true && pFilter.ExcludeMinions == true)
	{
		return true;
	}

	if (pAgent->second.Subgroup == 0 && mySourceData.SubGroup != 0 && pFilter.ExcludeOffSquad == true)
	{
		return true;
	}

	if (pAgent->second.Subgroup != 0 && mySourceData.SubGroup != pAgent->second.Subgroup && pFilter.ExcludeOffGroup == true)
	{
		return true;
	}

	if (pAgent->second.Subgroup == mySourceData.SubGroup && pFilter.ExcludeGroup == true)
	{
		return true;
	}

	return false;
}
