#pragma once
#ifdef STANDALONE

#include <stdint.h>
#include <string>
#include <vector>

class CombatMock
{
public:
	void AddAgent(const char* pAgentName, const char* pAccountName, uint32_t pProfession, uint32_t pElite, uint8_t pSubgroup, uint16_t pMasterInstanceId);
	void EnterCombat();

	void DisplayWindow();

private:
	struct Agent
	{
		std::string AgentName;
		std::string AccountName;
		uint32_t Profession = 0;
		uint32_t Elite = 0;
		uint8_t Subgroup = 0;
		uint16_t MasterInstanceId = 0;

		uint64_t UniqueId = 0;
		uint16_t InstanceId = 0;
	};

	struct Event
	{
		uint64_t Time = 0;
		uintptr_t SourceAgent = 0;
		uintptr_t DestinationAgent = 0;
		uint32_t SkillId = 0;
		uint32_t Value = 0;
		bool IsBuff = 0;

		uint64_t time;
		uintptr_t src_agent;
		uintptr_t dst_agent;
		int32_t value;
		int32_t buff_dmg;
		uint32_t overstack_value;
		uint32_t skillid;
		uint16_t src_instid;
		uint16_t dst_instid;
		uint16_t src_master_instid;
		uint16_t dst_master_instid;
		uint8_t iff;
		uint8_t buff;
		uint8_t result;
		uint8_t is_activation;
		uint8_t is_buffremove;
		uint8_t is_ninety;
		uint8_t is_fifty;
		uint8_t is_moving;
		uint8_t is_statechange;
		uint8_t is_flanking;
	};

	std::vector<Agent> myAgents;
	uint64_t myNextUniqueId = 100000;
	uint16_t myNextInstanceId = 1;
	uint64_t mySelfId = 0;

	char myInputAgentName[256] = {};
	char myInputAccountName[256] = {};
	int myInputProfession = 0;
	int myInputElite = 0;
	int myInputSubgroup = 0;
	int myInputMasterInstanceId = 0;


};
#endif