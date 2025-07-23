#pragma once
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include "arcdps_structs_slim.h"

struct HealedAgent
{
	uint16_t InstanceId;
	std::string Name; // Agent Name (UTF8)
	uint16_t Subgroup;
	bool IsMinion;
	bool IsPlayer;
	Prof Profession = Prof::PROF_UNKNOWN;
	uint32_t Elite = 0xFFFFFFFF;

	HealedAgent() = default;
	HealedAgent(const char* pAgentName);
	HealedAgent(std::string&& pAgentName);
	HealedAgent(uint16_t pInstanceId, const char* pAgentName, uint16_t pSubgroup, bool pIsMinion, bool pIsPlayer, Prof pProfession, uint32_t pElite);
};

class AgentTable
{
public:
	void AddAgent(uintptr_t pUniqueId, uint16_t pInstanceId, const char* pAgentName, std::optional<uint16_t> pSubgroup, std::optional<bool> pIsMinion, std::optional<bool> pIsPlayer, Prof pProfession, uint32_t pElite);

	std::optional<uintptr_t> GetUniqueId(uint16_t pInstanceId, bool pAllowNonPlayer);
	std::optional<std::string> GetName(uintptr_t pUniqueId);
	std::optional<HealedAgent> GetAgentData(uintptr_t pUniqueId);

	std::map<uintptr_t, HealedAgent> GetState();

private:
	std::mutex mLock;
	std::map<uintptr_t, HealedAgent> mAgents; // <Unique Id, Agent>
	std::map<uint16_t, std::map<uintptr_t, HealedAgent>::iterator> mInstanceIds; // <Instance Id, mAgents iterator>
};