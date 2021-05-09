#pragma once
#include <map>
#include <mutex>
#include <optional>
#include <string>

struct HealedAgent
{
	uint16_t InstanceId;
	std::string Name; // Agent Name (UTF8)
	uint16_t Subgroup;
	bool IsMinion;
	bool IsPlayer;

	HealedAgent(uint16_t pInstanceId, const char* pAgentName, uint16_t pSubgroup, bool pIsMinion, bool pIsPlayer);
};

class AgentTable
{
public:
	void AddAgent(uintptr_t pUniqueId, uint16_t pInstanceId, const char* pAgentName, std::optional<uint16_t> pSubgroup, std::optional<bool> pIsMinion, std::optional<bool> pIsPlayer);

	std::optional<uintptr_t> GetUniqueId(uint16_t pInstanceId, bool pAllowNonPlayer);

	std::map<uintptr_t, HealedAgent> GetState();

private:
	std::mutex mLock;
	std::map<uintptr_t, HealedAgent> mAgents; // <Unique Id, Agent>
	std::map<uint16_t, std::map<uintptr_t, HealedAgent>::iterator> mInstanceIds; // <Instance Id, mAgents iterator>
};