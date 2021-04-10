#pragma once
#include "arcdps_structs.h"

#include <atomic>
#include <shared_mutex>
#include <vector>

class EventHandler
{
private:
	const static uint32_t MAX_QUEUED_EVENTS = 256;

	struct Event
	{
		struct : cbtevent
		{
			bool present;
		} ev;

		struct
		{
			uintptr_t id;
			Prof prof;
			uint32_t elite;
			uint32_t self;
			std::string name;
			uint16_t team;

			bool present;
		} source_ag;

		struct
		{
			uintptr_t id;
			Prof prof;
			uint32_t elite;
			uint32_t self;
			std::string name;
			uint16_t team;

			bool present;
		} destination_ag;

		const char* skillname;
		uint64_t id;
		uint64_t revision;
	};
public:
	EventHandler(const CombatCallbackSignature pCallback);

	uintptr_t ProcessEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	bool QueueIsEmpty();

private:
	void TryFlushEvents();

	const CombatCallbackSignature mCallback = nullptr;

	std::shared_mutex mLock;
	std::atomic_uint64_t mHighestId = UINT64_MAX;
	std::atomic_uint32_t mQueuedEventCount = 0;
	std::atomic_uint32_t mHighestQueueSize = 0;
	Event mQueuedEvents[MAX_QUEUED_EVENTS];
};