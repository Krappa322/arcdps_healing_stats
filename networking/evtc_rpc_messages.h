#pragma once
#include "arcdps_structs.h"

#include <cstdint>

#pragma pack(push, 1)
namespace evtc_rpc
{
namespace messages
{
enum class Type : uint32_t
{
	Invalid = 0,
	RegisterSelf = 1,
	SetSelfId = 2,
	AddPeer = 3,
	RemovePeer = 4,
	CombatEvent = 5,
};

struct Header
{
	uint32_t MessageVersion;
	Type MessageType;
};
static_assert(sizeof(Header) == 8, "");

struct RegisterSelf
{
	uint16_t SelfId;
	uint8_t SelfAccountNameLength;
	//char SelfAccountName[];
};
static_assert(sizeof(RegisterSelf) == 3, "");

struct SetSelfId
{
	uint16_t SelfId;
};
static_assert(sizeof(SetSelfId) == 2, "");

struct AddPeer
{
	uint16_t PeerId;
	uint8_t PeerAccountNameLength;
	// char PeerAccountName[];
};
static_assert(sizeof(AddPeer) == 3, "");

struct RemovePeer
{
	uint16_t PeerId;
};
static_assert(sizeof(RemovePeer) == 2, "");

/*
struct CombatEvent
{
	cbtevent Event;

	uintptr_t SourceAgentId;
	uintptr_t DestinationAgentId;
	
	Prof SourceAgentProfession;
	Prof DestinationAgentProfession;
	
	uint32_t SourceAgentElite;
	uint32_t DestinationAgentElite;
	
	uint32_t SourceAgentSelf;
	uint32_t DestinationAgentSelf;
	
	uint16_t SourceAgentTeam;
	uint16_t DestinationAgentTeam;

	uint8_t SourceAgentNameLength; // UINT8_MAX => SourceAgentName is nullptr
	uint8_t DestinationAgentNameLength; // UINT8_MAX => DestinationAgentName is nullptr

	// char SourceAgentName[];
	// char DestinationAgentName[];
};
static_assert(sizeof(CombatEvent) == 110, "");
*/

struct CombatEvent
{
	cbtevent Event;
	uint16_t SenderInstanceId; // 0 when sent from client
};
static_assert(sizeof(CombatEvent) == 66, "");

};
};
#pragma pack(pop)