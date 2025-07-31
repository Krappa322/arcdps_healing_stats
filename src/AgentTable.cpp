#include "AgentTable.h"
#include "Log.h"

#include <cassert>

HealedAgent::HealedAgent(uint16_t pInstanceId, const char* pAgentName, const char* pAccountName, uint16_t pSubgroup, bool pIsMinion, bool pIsPlayer, Prof pProfession, uint32_t pElite)
	: InstanceId{pInstanceId}
	, Name{pAgentName}
	, AccountName{pAccountName != nullptr ? pAccountName : ""}
	, Subgroup{pSubgroup}
	, IsMinion{pIsMinion}
	, IsPlayer{pIsPlayer}
	, Profession{pProfession}
	, Elite{pElite}
{
}

HealedAgent::HealedAgent(const char* pAgentName)
	: InstanceId{0}
	, Name{pAgentName}
	, AccountName{}
	, Subgroup{0}
	, IsMinion{false}
	, IsPlayer{false}
	, Profession{Prof::PROF_UNKNOWN}
	, Elite{0xFFFFFFFF}
{
}

HealedAgent::HealedAgent(std::string&& pAgentName)
	: InstanceId{0}
	, Name{std::move(pAgentName)}
	, AccountName{}
	, Subgroup{0}
	, IsMinion{false}
	, IsPlayer{false}
	, Profession{Prof::PROF_UNKNOWN}
	, Elite{0xFFFFFFFF}
{
}

bool HealedAgent::operator==(const HealedAgent& pRight) const
{
	return std::tie(InstanceId, Name, AccountName, Subgroup, IsMinion, IsPlayer, Profession, Elite) == std::tie(pRight.InstanceId, pRight.Name, pRight.AccountName, pRight.Subgroup, pRight.IsMinion, pRight.IsPlayer, pRight.Profession, pRight.Elite);
}

bool HealedAgent::operator!=(const HealedAgent& pRight) const
{
	return (*this == pRight) == false;
}

void AgentTable::AddAgent(uintptr_t pUniqueId, uint16_t pInstanceId, const char* pAgentName, const char* pAccountName, std::optional<uint16_t> pSubgroup, std::optional<bool> pIsMinion, std::optional<bool> pIsPlayer, Prof pProfession, uint32_t pElite)
{
	assert(pAgentName != nullptr);
	LOG("Inserting new agent %llu %hu %s %hu %s %s", pUniqueId, pInstanceId, pAgentName, pSubgroup.value_or(0), BOOL_STR(pIsMinion.value_or(false)), BOOL_STR(pIsPlayer.value_or(false)));

	std::lock_guard lock(mLock);

	auto [agent, agentInserted] = mAgents.try_emplace(pUniqueId, pInstanceId, pAgentName, pAccountName, pSubgroup.value_or(0), pIsMinion.value_or(false), pIsPlayer.value_or(false), pProfession, pElite);
	if (agentInserted == false)
	{
		if ((strcmp(agent->second.Name.c_str(), pAgentName) != 0)
			|| (pAccountName != nullptr && strcmp(agent->second.AccountName.c_str(), pAccountName) != 0)
			|| (agent->second.InstanceId != pInstanceId)
			|| (pSubgroup.has_value() && agent->second.Subgroup != *pSubgroup)
			|| (pIsMinion.has_value() && agent->second.IsMinion != *pIsMinion)
			|| (pIsPlayer.has_value() && agent->second.IsPlayer != *pIsPlayer)
			|| (agent->second.Profession != pProfession)
			|| (agent->second.Elite != pElite))
		{
			LOG("Unique id %llu already exists - replacing existing entry %hu %s %hu %s %s",
				pUniqueId, agent->second.InstanceId, agent->second.Name.c_str(), agent->second.Subgroup, BOOL_STR(agent->second.IsMinion), BOOL_STR(agent->second.IsPlayer));

			agent->second = HealedAgent{
				pInstanceId,
				pAgentName,
				pAccountName != nullptr ? pAccountName : agent->second.AccountName.c_str(),
				pSubgroup.value_or(agent->second.Subgroup),
				pIsMinion.value_or(agent->second.IsMinion),
				pIsPlayer.value_or(agent->second.IsPlayer),
				pProfession,
				pElite};
		}
	}

	auto [iter, instidInserted] = mInstanceIds.try_emplace(pInstanceId, agent);
	if (instidInserted == false && iter->second != agent)
	{
		LOG("Instance id %hu already exists - replacing existing entry %llu %s %hu %s %s",
			pInstanceId, iter->second->first, iter->second->second.Name.c_str(), iter->second->second.Subgroup, BOOL_STR(iter->second->second.IsMinion), BOOL_STR(iter->second->second.IsPlayer));
		iter->second = agent;
	}
}

std::optional<uintptr_t> AgentTable::GetUniqueId(uint16_t pInstanceId, bool pAllowNonPlayer)
{
	std::lock_guard lock(mLock);

	auto iter = mInstanceIds.find(pInstanceId);
	if (iter == mInstanceIds.end())
	{
		LOG("Couldn't find instance id %hu", pInstanceId);
		return std::nullopt;
	}

	if (pAllowNonPlayer == false && iter->second->second.IsPlayer == false)
	{
		LOG("Mapped %hu to %llu but it is not a player (%hu %s %hu %s %s)", pInstanceId, iter->second->first,
			iter->second->second.InstanceId, iter->second->second.Name.c_str(), iter->second->second.Subgroup, BOOL_STR(iter->second->second.IsMinion), BOOL_STR(iter->second->second.IsPlayer));
		return std::nullopt;
	}

	LOG("Mapping %hu to %llu", pInstanceId, iter->second->first);
	return iter->second->first;
}

std::optional<std::string> AgentTable::GetName(uintptr_t pUniqueId)
{
	std::lock_guard lock(mLock);

	auto iter = mAgents.find(pUniqueId);
	if (iter == mAgents.end())
	{
		LOG("Couldn't find unique id %llu", pUniqueId);
		return std::nullopt;
	}

	DEBUGLOG("Mapping %llu to %s", pUniqueId, iter->second.Name.c_str());
	return iter->second.Name;
}

std::optional<HealedAgent> AgentTable::GetAgentData(uintptr_t pUniqueId)
{
	std::lock_guard lock(mLock);

	auto iter = mAgents.find(pUniqueId);
	if (iter == mAgents.end())
	{
		LogD("Couldn't find unique id {}", pUniqueId);
		return std::nullopt;
	}

	LogT("Mapping {} to {}", pUniqueId, iter->second.Name.c_str());
	return iter->second;
}

std::map<uintptr_t, HealedAgent> AgentTable::GetState()
{
	std::map<uintptr_t, HealedAgent> result;
	{
		std::lock_guard lock(mLock);
		result = mAgents;
	}

	return result;
}
