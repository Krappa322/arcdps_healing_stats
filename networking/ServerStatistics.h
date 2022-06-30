#pragma once
#include "evtc_rpc_messages.h"

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <array>

enum class CallDataType : uint32_t
{
	Connect,
	Finish,
	ReadMessage,
	WriteEvent,
	Disconnect,
	WakeUp, // Sent internally only
	Max,
};

struct ServerStatisticsSample
{
	size_t RegisteredPlayers;
	size_t RegisteredPeers;
	size_t KnownPeers;
};

class evtc_rpc_server;
class ServerStatistics final : public prometheus::Collectable
{
public:
	ServerStatistics(evtc_rpc_server& pParent);

	std::vector<prometheus::MetricFamily> Collect() const;

	std::array<prometheus::Counter*, static_cast<size_t>(CallDataType::Max)> CallData = {};
	std::array<prometheus::Counter*, static_cast<size_t>(evtc_rpc::messages::Type::Max)> MessageTypeReceive = {};
	std::array<prometheus::Counter*, static_cast<size_t>(evtc_rpc::messages::Type::Max)> MessageTypeTransmit = {};
	std::shared_ptr<prometheus::Registry> PrometheusRegistry;

private:
	evtc_rpc_server& mParent; // Works around the fact that prometheus is trying to force usage of std::shared_ptr - very weird interface tbh
};

