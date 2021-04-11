#include "AggregatedStats.h"

#include "Log.h"
#include "Skills.h"
#include "Utilities.h"

#include <assert.h>
#include <Windows.h>

#include <algorithm>
#include <map>

constexpr const char* GROUP_FILTER_STRING[] = { "Group", "Squad", "All (Excluding Summons)", "All (Including Summons)" };
static_assert((sizeof(GROUP_FILTER_STRING) / sizeof(GROUP_FILTER_STRING[0])) == static_cast<size_t>(GroupFilter::Max), "Added group filter option without updating gui?");

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
	, myFilteredAgents(nullptr)
	, mySkills(nullptr)
	, myGroupFilterTotals(nullptr)
	, myDebugMode(pDebugMode)
{
	assert(static_cast<SortOrder>(myOptions.SortOrderChoice) < SortOrder::Max);
	assert(static_cast<DataSource>(myOptions.DataSourceChoice) < DataSource::Max);
}

void AggregatedVector::Add(uint64_t pId, std::string&& pName, uint64_t pHealing, uint64_t pHits, std::optional<uint64_t> pCasts)
{
	const AggregatedStatsEntry& newEntry = Entries.emplace_back(pId, std::move(pName), pHealing, pHits, std::move(pCasts));
	HighestHealing = (std::max)(HighestHealing, newEntry.Healing);
}

const AggregatedStatsEntry& AggregatedStats::GetTotal()
{
	if (myTotal != nullptr)
	{
		return *myTotal;
	}

	uint64_t healing = 0;
	uint64_t hits = 0;
	for (const AggregatedStatsEntry& entry : GetSkills(std::nullopt).Entries)
	{
		healing += entry.Healing;
		hits += entry.Hits;
	}

	myTotal = std::make_unique<AggregatedStatsEntry>(0, "__TOTAL__", healing, hits, std::nullopt);
	return *myTotal;
}

const AggregatedVector& AggregatedStats::GetStats(DataSource pDataSource)
{
	switch (pDataSource)
	{
	case DataSource::Skills:
		return GetSkills(std::nullopt);
	case DataSource::Agents:
		return GetAgents(std::nullopt);
	case DataSource::Totals:
	default:
		return GetGroupFilterTotals();
	}
}

const AggregatedVector& AggregatedStats::GetDetails(DataSource pDataSource, uint64_t pId)
{
	switch (pDataSource)
	{
	case DataSource::Skills:
		return GetAgents(static_cast<uint32_t>(pId));
	case DataSource::Agents:
	default:
		return GetSkills(static_cast<uintptr_t>(pId));
	}
}


const AggregatedVector& AggregatedStats::GetGroupFilterTotals()
{
	if (myGroupFilterTotals != nullptr)
	{
		return *myGroupFilterTotals;
	}

	myGroupFilterTotals = std::make_unique<AggregatedVector>();
	for (uint32_t i = 0; i < static_cast<uint32_t>(GroupFilter::Max); i++)
	{
		myGroupFilterTotals->Add(0, GROUP_FILTER_STRING[i], 0, 0, std::nullopt);
	}

	HealWindowOptions fakeOptions;

	uint64_t combatEnd = GetCombatEnd();

	for (const HealEvent& curEvent : mySourceData.Events)
	{
		if (curEvent.Time > combatEnd)
		{
			continue;
		}

		auto mapAgent = std::as_const(mySourceData.Agents).find(curEvent.AgentId);

		// Loop through the array and pretend index is GroupFilter, if agent does not get filtered by that filter then add
		// the total healing to that agent to the total for that filter
		for (size_t i = 0; i < static_cast<uint32_t>(GroupFilter::Max); i++)
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
				myGroupFilterTotals->Entries[i].Hits += 1;
				myGroupFilterTotals->Entries[i].Healing += curEvent.Size;
			}
		}
	}

	for (const AggregatedStatsEntry& entry : myGroupFilterTotals->Entries)
	{
		myGroupFilterTotals->HighestHealing = (std::max)(myGroupFilterTotals->HighestHealing, entry.Healing);
	}

	return *myGroupFilterTotals;
}


float AggregatedStats::GetCombatTime()
{
	if (mySourceData.EnteredCombatTime == 0)
	{
		return 0.0f;
	}

	uint64_t end = GetCombatEnd();
	assert(end >= mySourceData.EnteredCombatTime);
	return (end - mySourceData.EnteredCombatTime) / 1000;
}

uint64_t AggregatedStats::GetCombatEnd()
{
	uint64_t end = 0;
	CombatEndCondition endCondition = static_cast<CombatEndCondition>(myOptions.CombatEndConditionChoice);
	if (endCondition == CombatEndCondition::CombatExit)
	{
		end = mySourceData.ExitedCombatTime;
	}
	else if (endCondition == CombatEndCondition::LastHealEvent)
	{
		if (mySourceData.Events.size() != 0)
		{
			end = mySourceData.Events.back().Time;
		}
	}
	else if (endCondition == CombatEndCondition::LastDamageEvent)
	{
		end = mySourceData.LastDamageEvent;
	}
	else
	{
		assert(endCondition == CombatEndCondition::LastDamageOrHealEvent);

		if (mySourceData.Events.size() != 0)
		{
			end = (std::max)(mySourceData.LastDamageEvent, mySourceData.Events.back().Time);
		}
		else
		{
			end = mySourceData.LastDamageEvent;
		}
	}

	if (end == 0)
	{
		if (mySourceData.ExitedCombatTime != 0)
		{
			end = mySourceData.ExitedCombatTime;
		}
		else
		{
			end = mySourceData.CollectionTime;
		}
	}

	return end;
}

const AggregatedVector& AggregatedStats::GetAgents(std::optional<uint32_t> pSkillId)
{
	AggregatedVector* entry = nullptr;

	if (pSkillId.has_value() == true)
	{
		const auto [mapEntry, inserted] = mySkillsDetailed.emplace(std::piecewise_construct,
			std::forward_as_tuple(*pSkillId),
			std::forward_as_tuple());
		if (inserted == false)
		{
			return mapEntry->second; // Return cached value
		}

		entry = &mapEntry->second;
	}
	else
	{
		if (myFilteredAgents != nullptr)
		{
			return *myFilteredAgents; // Return cached value
		}

		myFilteredAgents = std::make_unique<AggregatedVector>();
		entry = myFilteredAgents.get();
	}

	struct TempAgent
	{
		std::map<uintptr_t, HealedAgent>::const_iterator Iterator;
		uint64_t Ticks;
		uint64_t Healing;

		TempAgent(std::map<uintptr_t, HealedAgent>::const_iterator&& pIterator, uint64_t pTicks, uint64_t pHealing)
			: Iterator{std::move(pIterator)}
			, Ticks{pTicks}
			, Healing{pHealing}
		{
		}
	};
	std::map<uintptr_t, TempAgent> tempMap;

	uint64_t combatEnd = GetCombatEnd();

	for (const HealEvent& curEvent : mySourceData.Events)
	{
		if (curEvent.Time > combatEnd)
		{
			continue;
		}

		if (pSkillId.has_value() == true)
		{
			if (curEvent.SkillId != *pSkillId)
			{
				continue;
			}
		}

		auto mapAgent = std::as_const(mySourceData.Agents).find(curEvent.AgentId);
		if (Filter(mapAgent) == true)
		{
			continue;
		}

		auto [agent, _inserted] = tempMap.try_emplace(curEvent.AgentId, std::move(mapAgent), 0, 0);

		agent->second.Ticks += 1;
		agent->second.Healing += curEvent.Size;
	}

	// Caching the result in a display friendly way
	for (const auto& [agentId, agent] : tempMap)
	{
		std::string agentName;

		if (myDebugMode == false)
		{
			if (agent.Iterator != mySourceData.Agents.end())
			{
				agentName = agent.Iterator->second.Name;
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
			if (agent.Iterator != mySourceData.Agents.end())
			{
				snprintf(buffer, sizeof(buffer), "%llu ; %u ; %u ; %s", agentId, agent.Iterator->second.Subgroup, agent.Iterator->second.IsMinion, agent.Iterator->second.Name.c_str());
			}
			else
			{
				snprintf(buffer, sizeof(buffer), "%llu ; (UNMAPPED)", agentId);
			}

			agentName = buffer;
		}

		entry->Add(agentId, std::move(agentName), agent.Healing, agent.Ticks, std::nullopt);
	}

	Sort(entry->Entries);

	return *entry;
}

const AggregatedVector& AggregatedStats::GetSkills(std::optional<uintptr_t> pAgentId)
{
	AggregatedVector* entry = nullptr;

	if (pAgentId.has_value() == true)
	{
		const auto [mapEntry, inserted] = myAgentsDetailed.emplace(std::piecewise_construct,
			std::forward_as_tuple(*pAgentId),
			std::forward_as_tuple());
		if (inserted == false)
		{
			return mapEntry->second; // Return cached value
		}

		entry = &mapEntry->second;
	}
	else
	{
		if (mySkills != nullptr)
		{
			return *mySkills; // Return cached value
		}

		mySkills = std::make_unique<AggregatedVector>();
		entry = mySkills.get();
	}

	struct TempSkill
	{
		uint64_t Ticks;
		uint64_t Healing;

		TempSkill(uint64_t pTicks, uint64_t pHealing)
			: Ticks{pTicks}
			, Healing{pHealing}
		{
		}
	};
	std::map<uint32_t, TempSkill> tempMap;

	uint64_t combatEnd = GetCombatEnd();
	uint64_t totalIndirectHealing = 0;
	uint64_t totalIndirectTicks = 0;

	for (const HealEvent& curEvent : mySourceData.Events)
	{
		if (curEvent.Time > combatEnd)
		{
			continue;
		}

		if (pAgentId.has_value() == true)
		{
			if (curEvent.AgentId != *pAgentId)
			{
				continue;
			}
		}
		else
		{
			auto mapAgent = std::as_const(mySourceData.Agents).find(curEvent.AgentId);
			if (Filter(mapAgent) == true)
			{
				continue;
			}
		}

		auto [skill, _inserted] = tempMap.try_emplace(curEvent.SkillId, 0, 0);

		skill->second.Ticks += 1;
		skill->second.Healing += curEvent.Size;
	}

	for (const auto& [skillId, skill] : tempMap)
	{
		char buffer[1024];
		char buffer2[1024];

		const char* skillName = nullptr;

		auto mapSkill = std::as_const(mySourceData.Skills).find(skillId);
		if (mapSkill != mySourceData.Skills.end())
		{
			skillName = mapSkill->second.Name;
		}
		else
		{
			LOG("Couldn't map skill %u", skillId);
			snprintf(buffer2, sizeof(buffer2), "%u", skillId);
			skillName = buffer2;
		}

		bool isIndirectHealing = false;
		if (SkillTable::GlobalState.IsSkillIndirectHealing(skillId, skillName) == true)
		{
			LOG("Translating skill %hu %s to indirect healing", skillId, skillName);

			totalIndirectHealing += skill.Healing;
			totalIndirectTicks += skill.Ticks;
			isIndirectHealing = true;

			if (myDebugMode == false)
			{
				continue;
			}
		}

		if (myDebugMode == true)
		{
			snprintf(buffer, sizeof(buffer), "%s%u ; %s", isIndirectHealing ? "(INDIRECT) ; " : "", skillId, skillName);
			skillName = buffer;
		}

		entry->Add(skillId, std::string{skillName}, skill.Healing, skill.Ticks, std::nullopt);
	}

	if (totalIndirectHealing != 0 || totalIndirectTicks != 0)
	{
		std::string skillName("Healing by Damage Dealt");

		entry->Add(IndirectHealingSkillId, std::move(skillName), totalIndirectHealing, totalIndirectTicks, std::nullopt);
	}

	Sort(entry->Entries);

	return *entry;
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
