#include "ServerStatistics.h"
#include "Server.h"
#include "../src/Log.h"

namespace
{
constexpr const char* CallDataTypeToString(CallDataType pType)
{
	switch (pType)
	{
	case CallDataType::Connect:
		return "Connect";
	case CallDataType::Finish:
		return "Finish";
	case CallDataType::ReadMessage:
		return "ReadMessage";
	case CallDataType::WriteEvent:
		return "WriteEvent";
	case CallDataType::Disconnect:
		return "Disconnect";
	case CallDataType::WakeUp:
		return "WakeUp";
	default:
		return "<invalid>";
	};
}

constexpr const char* EvtcRpcMessageTypeToString(evtc_rpc::messages::Type pType)
{
	using evtc_rpc::messages::Type;

	switch (pType)
	{
	case Type::Invalid:
		return "Invalid";
	case Type::RegisterSelf:
		return "RegisterSelf";
	case Type::SetSelfId:
		return "SetSelfId";
	case Type::AddPeer:
		return "AddPeer";
	case Type::RemovePeer:
		return "RemovePeer";
	case Type::CombatEvent:
		return "CombatEvent";
	default:
		return "<invalid>";
	};
}
}; // anonymous namespace

ServerStatistics::ServerStatistics(evtc_rpc_server& pParent)
	: PrometheusRegistry(std::make_shared<prometheus::Registry>())
	, mParent(pParent)
{
	auto& call_data = prometheus::BuildCounter()
		.Name("evtc_rpc_server_call_data")
		.Register(*PrometheusRegistry);
	auto& message_type = prometheus::BuildCounter()
		.Name("evtc_rpc_server_message_type")
		.Register(*PrometheusRegistry);

	for (size_t i = 0; i < CallData.size(); i++)
	{
		CallData[i] = &call_data.Add({{"type", CallDataTypeToString(static_cast<CallDataType>(i))}});
	}
	
	for (size_t i = 0; i < MessageTypeReceive.size(); i++)
	{
		MessageTypeReceive[i] = &message_type.Add({
			{"type", EvtcRpcMessageTypeToString(static_cast<evtc_rpc::messages::Type>(i))},
			{"direction", "receive"}});
	}

	for (size_t i = 0; i < MessageTypeTransmit.size(); i++)
	{
		MessageTypeTransmit[i] = &message_type.Add({
			{"type", EvtcRpcMessageTypeToString(static_cast<evtc_rpc::messages::Type>(i))},
			{"direction", "transmit"}});
	}
}

std::vector<prometheus::MetricFamily> ServerStatistics::Collect() const
{
	using namespace std::chrono;

	LogI("ServerStatistics::Collect!");

	auto data = mParent.GetStatistics();
	int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

	std::vector<prometheus::MetricFamily> result;
	{
		auto& family = result.emplace_back();
		family.name = "evtc_rpc_server_registered_players";
		family.help = "";
		family.type = prometheus::MetricType::Gauge;

		auto& metric = family.metric.emplace_back();
		metric.gauge.value = static_cast<double>(data.RegisteredPlayers);
		metric.timestamp_ms = now;
	}

	{
		auto& family = result.emplace_back();
		family.name = "evtc_rpc_server_known_peers";
		family.help = "";
		family.type = prometheus::MetricType::Gauge;

		auto& metric = family.metric.emplace_back();
		metric.gauge.value = static_cast<double>(data.KnownPeers);
		metric.timestamp_ms = now;
	}

	{
		auto& family = result.emplace_back();
		family.name = "evtc_rpc_server_registered_peers";
		family.help = "";
		family.type = prometheus::MetricType::Gauge;

		auto& metric = family.metric.emplace_back();
		metric.gauge.value = static_cast<double>(data.RegisteredPeers);
		metric.timestamp_ms = now;
	}


	return result;
}
