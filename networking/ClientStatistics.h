#pragma once
#include <atomic>

class evtc_rpc_client;

class evtc_rpc_client_statistics
{
	friend evtc_rpc_client;
public:
	uint64_t GetTotalEventsQueued() const { return mTotalEventsQueued.load(); }
	uint64_t GetTotalEventsSent() const { return mTotalEventsSent.load(); }
	uint64_t GetTotalEventsDropped() const { return mTotalEventsDropped.load(); }
private:
	std::atomic<uint64_t> mTotalEventsQueued = 0;
	std::atomic<uint64_t> mTotalEventsSent = 0;
	std::atomic<uint64_t> mTotalEventsDropped = 0;
};
