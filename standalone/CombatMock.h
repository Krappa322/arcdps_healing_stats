#pragma once
#ifdef STANDALONE

#include "../ArcDPS.h"

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

class CombatMock
{
private:
	struct Agent
	{
		std::string AgentName;
		std::string AccountName;
		uint32_t Profession = 0;
		uint32_t Elite = 0;
		uint8_t Subgroup = 0;
		uint64_t MasterUniqueId = 0;

		uint64_t UniqueId = 0;
		uint16_t InstanceId = 0;
	};

	enum class CombatEventType : int
	{
		Direct = 0,
		EnterCombat = 1,
		ExitCombat = 2,

		Max
	};

	static constexpr const char* const CombatEventTypeString[] = { "Direct", "EnterCombat", "ExitCombat" };

	struct CombatEvent
	{
		CombatEventType Type = CombatEventType::Direct;
		uint64_t Time = 0;
		uint64_t SourceAgentUniqueId = 0;
		uint64_t DestinationAgentUniqueId = 0;
		uint32_t SkillId = 0;
		int32_t Value = 0;
		bool IsBuff = false; // For now, this indicates whether the direct event came from condition/regeneration or from direct hit
	};

public:
	CombatMock(const arcdps_exports* pCallbacks);

	void AddAgent(const char* pAgentName, const char* pAccountName, uint32_t pProfession, uint32_t pElite, uint8_t pSubgroup, uint64_t pMasterUniqueId);
	void AddEvent(CombatEventType pType, uint64_t pTime, uint64_t pSourceAgentUniqueId, uint64_t pDestinationAgentUniqueId, uint32_t pSkillId, int32_t pValue, bool pIsBuff);

	uint32_t SaveToFile(const char* pFilePath);
	uint32_t LoadFromFile(const char* pFilePath);
	void Execute();

	void DisplayWindow();

private:
	void FillAgent(const Agent* pAgent, uint64_t pSelfId, ag& pResult);
	void FillAgentEvent(const Agent& pAgent, uint64_t pSelfId, ag& pSource, ag& pDestination);

	// Pointer is invalidated by the same things as a vector iterator
	const Agent* GetAgent(uint64_t pAgentUniqueId);

	void DisplayAgents();
	void DisplayEvents();
	void DisplayAddAgent();
	void DisplayAddEvent();
	void DisplayActions();

	const arcdps_exports* const myCallbacks;

	std::vector<Agent> myAgents;
	std::vector<CombatEvent> myEvents;
	uint64_t myNextUniqueId = 100000;
	uint16_t myNextInstanceId = 1;
	uint64_t mySelfId = 0;

	char myInputAgentName[256] = {};
	char myInputAccountName[256] = {};
	int myInputProfession = 0;
	int myInputElite = 0;
	int myInputSubgroup = 0;
	uint64_t myInputMasterUniqueId = 0;

	int myInputCombatType = 0;
	uint64_t myInputTime = 0;
	uint64_t myInputSourceAgentUniqueId = 0;
	uint64_t myInputDestinationAgentUniqueId = 0;
	uint32_t myInputSkillId = 0;
	int32_t myInputValue = 0;
	bool myInputIsBuff = false;

	char myInputFilePath[512] = {};

	std::map<uint32_t, std::string> mySkillNames;
};
#endif