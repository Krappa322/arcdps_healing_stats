#include "EventSequencer.h"
#include "Log.h"

#include <cassert>

EventSequencer::EventSequencer(const CombatCallbackSignature pCallback)
	: mCallback(pCallback)
{
	for (uint32_t i = 0; i < MAX_QUEUED_EVENTS; i++)
	{
		mQueuedEvents[i].id = 0;
	}
}

uintptr_t EventSequencer::ProcessEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	if (pId == 0) // id 0 can occur multiple times and is unordered
	{
		DEBUGLOG("Id0 event");

		mCallback(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
		return 0;
	}

	uint64_t current = mHighestId.load(std::memory_order_acquire);
	while (true)
	{
		DEBUGLOG(">> %llu", pId);
		assert(current != pId);

		if (current == UINT64_MAX)
		{
			bool result = mHighestId.compare_exchange_weak(current, pId - 1, std::memory_order_acq_rel);

			LOG("Registered first event (%llu) - result %s", pId, BOOL_STR(result));

			current = mHighestId.load(std::memory_order_acquire);
			continue;
		}
		else if (current > (pId - 1)) // Race condition after registering first event
		{
			LOG("Got event lower than current highest seen (%llu vs %llu)", pId, current);

			mCallback(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
			return 0;
		}
		else if (current == (pId - 1)) // Fast path (most common)
		{
			mCallback(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);

			if (mHighestId.compare_exchange_strong(current, pId, std::memory_order_acq_rel) == false)
			{
				assert(false);
			}

			TryFlushEvents();

			return 0;
		}
		else
		{
			std::shared_lock lock(mLock);

			// change if current changed since waiting for lock could potentially take quite long
			uint64_t current2 = mHighestId.load(std::memory_order_acquire);
			if (current != current2)
			{
				LOG("Race1 current %llu current2 %llu", current, current2);

				current = current2;
				continue;
			}

			// Add first so that another racing thread is sure to read mQueuedLocalEventCount != 0 after changing
			// the value (or it is not able to change the value).
			uint32_t index = mQueuedEventCount.fetch_add(1, std::memory_order_acq_rel);
			if (index >= MAX_QUEUED_EVENTS)
			{
				LOG("More than max events queued - %u %u %llu %llu", index, MAX_QUEUED_EVENTS, pId, current);
				// Subtracting here has the same issue as described below in Race2

				assert(false);

				mCallback(pEvent, pSourceAgent, pDestinationAgent, pSkillname, pId, pRevision);
				return 0;
			}

			current2 = mHighestId.load(std::memory_order_acquire);
			if (current != current2)
			{
				// We can't decrement because of the race condition
				// Thread2: fetch_add (0)
				// Thread3: fetch_add (1)
				// Thread3: read mHighestId
				// Thread1: change mHighestId
				// Thread2: read mHighestId
				//
				// If we were to decrement at that point, then index 1 would be given out twice
				// So instead, we just add the event as completely empty.
				mQueuedEvents[index].id = 0;

				LOG("Race2 current %llu current2 %llu index %u", current, current2, index);

				current = current2;
				continue;
			}

			if (pEvent != nullptr)
			{
				*static_cast<cbtevent*>(&mQueuedEvents[index].ev) = *pEvent;
				mQueuedEvents[index].ev.present = true;
			}
			else
			{
				mQueuedEvents[index].ev.present = false;
			}

			if (pSourceAgent != nullptr)
			{
				mQueuedEvents[index].source_ag.id = pSourceAgent->id;
				mQueuedEvents[index].source_ag.prof = pSourceAgent->prof;
				mQueuedEvents[index].source_ag.elite = pSourceAgent->elite;
				mQueuedEvents[index].source_ag.self = pSourceAgent->self;
				mQueuedEvents[index].source_ag.team = pSourceAgent->team;

				if (pSourceAgent->name != nullptr)
				{
					mQueuedEvents[index].source_ag.name_storage = pSourceAgent->name;
					mQueuedEvents[index].source_ag.name = mQueuedEvents[index].source_ag.name_storage.c_str();
				}
				else
				{
					mQueuedEvents[index].source_ag.name = nullptr;
				}

				mQueuedEvents[index].source_ag.present = true;
			}
			else
			{
				mQueuedEvents[index].source_ag.present = false;
			}

			if (pDestinationAgent != nullptr)
			{
				mQueuedEvents[index].destination_ag.id = pDestinationAgent->id;
				mQueuedEvents[index].destination_ag.prof = pDestinationAgent->prof;
				mQueuedEvents[index].destination_ag.elite = pDestinationAgent->elite;
				mQueuedEvents[index].destination_ag.self = pDestinationAgent->self;
				mQueuedEvents[index].destination_ag.team = pDestinationAgent->team;

				if (pDestinationAgent->name != nullptr)
				{
					mQueuedEvents[index].destination_ag.name_storage = pDestinationAgent->name;
					mQueuedEvents[index].destination_ag.name = mQueuedEvents[index].destination_ag.name_storage.c_str();
				}
				else
				{
					mQueuedEvents[index].destination_ag.name = nullptr;
				}

				mQueuedEvents[index].destination_ag.present = true;
			}
			else
			{
				mQueuedEvents[index].destination_ag.present = false;
			}

			mQueuedEvents[index].skillname = pSkillname;
			mQueuedEvents[index].id = pId;
			mQueuedEvents[index].revision = pRevision;

			DEBUGLOG("Queued %llu at index %u, current %llu", pId, index, current);
			return 0;
		}
	}
}

bool EventSequencer::QueueIsEmpty()
{
	bool isEmpty = (mQueuedEventCount.load(std::memory_order_acquire) == 0);

	LOG("isEmpty=%s, highestQueueSize=%u", BOOL_STR(isEmpty), mHighestQueueSize.load(std::memory_order_relaxed));
	return isEmpty;
}

void EventSequencer::TryFlushEvents()
{
	if (mQueuedEventCount.load(std::memory_order_acquire) == 0)
	{
		return;
	}

	std::unique_lock lock(mLock);

	uint32_t eventCount = mQueuedEventCount.load(std::memory_order_acquire);
	if (eventCount > MAX_QUEUED_EVENTS)
	{
		// This is possible because of Race1 above
		LOG("Truncating event count %u %u", eventCount, MAX_QUEUED_EVENTS);
		eventCount = MAX_QUEUED_EVENTS;
	}

	bool removed = false;
	size_t nonZeroEvents = 0;
	do
	{
		removed = false;
		nonZeroEvents = 0;

		for (uint32_t i = 0; i < eventCount; i++)
		{
			if (mQueuedEvents[i].id == 0)
			{
				continue;
			}

			uint64_t current = mHighestId.load(std::memory_order_acquire);
			if (current != (mQueuedEvents[i].id - 1))
			{
				nonZeroEvents++;
				DEBUGLOG("Skipping %llu", mQueuedEvents[i].id);
				continue;
			}

			DEBUGLOG(">> Delayed %llu", mQueuedEvents[i].id);

			ag source;
			ag destination;

			ag* source_arg = nullptr;
			ag* destination_arg = nullptr;
			cbtevent* ev_arg = nullptr;
			if (mQueuedEvents[i].source_ag.present == true)
			{
				source_arg = &source;
				source = *static_cast<ag*>(&mQueuedEvents[i].source_ag);
			}

			if (mQueuedEvents[i].destination_ag.present == true)
			{
				destination_arg = &destination;
				destination = *static_cast<ag*>(&mQueuedEvents[i].destination_ag);
			}

			if (mQueuedEvents[i].ev.present == true)
			{
				ev_arg = &mQueuedEvents[i].ev;
			}

			mCallback(ev_arg, source_arg, destination_arg, mQueuedEvents[i].skillname, mQueuedEvents[i].id, mQueuedEvents[i].revision);

			if (mHighestId.compare_exchange_strong(current, mQueuedEvents[i].id, std::memory_order_acq_rel) == false)
			{
				assert(false);
			}

			removed = true;
			mQueuedEvents[i].id = 0;
		}
	} while (removed == true);

	if (nonZeroEvents == 0)
	{
		for (uint32_t i = 0; i < MAX_QUEUED_EVENTS; i++)
		{
			assert(mQueuedEvents[i].id == 0);
		}

		uint32_t oldSize = mQueuedEventCount.exchange(0, std::memory_order_acq_rel);

		// No need to worry about races here, this is the only place it's written to and that's done under lock
		if (mHighestQueueSize.load(std::memory_order_relaxed) < oldSize)
		{
			mHighestQueueSize.store(oldSize, std::memory_order_relaxed);
		}

		DEBUGLOG("Clearing size %u", oldSize);
	}
}

