#pragma once
#ifdef _WIN32
#pragma warning(disable : 4702)
#pragma warning(push, 0)
#endif
#include <evtc_rpc.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#ifdef _WIN32
#pragma warning(pop)
#pragma warning(default : 4702)
#endif

#include "evtc_rpc_messages.h"

#include <deque>

class evtc_rpc_server
{
	struct ConnectionContext
	{
		std::map<std::string, std::shared_ptr<ConnectionContext>>::iterator Iterator{}; // Protected by mRegisteredAgentsLock on the server that owns this ConnectionContext
		uint16_t InstanceId = 0; // Protected by mRegisteredAgentsLock on the server that owns this ConnectionContext
		std::map<std::string, uint16_t> Peers; // Protected by mRegisteredAgentsLock on the server that owns this ConnectionContext

		grpc::ServerContext ServerContext;
		// Reads against the stream are protected since they are serialized, writes are protected with WriteLock
		grpc::ServerAsyncReaderWriter<evtc_rpc::Message, evtc_rpc::Message> Stream{&ServerContext};

		std::mutex WriteLock;
		bool ForceDisconnected = false; // Protected by WriteLock
		bool WritePending = false; // Protected by WriteLock
		std::deque<evtc_rpc::messages::CombatEvent> QueuedEvents; // Protected by WriteLock
	};

	enum class CallDataType
	{
		Connect,
		Finish,
		ReadMessage,
		WriteEvent,
		Disconnect,

		Invalid
	};

	struct CallDataBase
	{
		CallDataBase(std::shared_ptr<ConnectionContext>&& pContext)
			: Context{pContext}
		{
		}

		CallDataType Type = CallDataType::Invalid;
		std::shared_ptr<ConnectionContext> Context;
	};

	struct FinishCallData : public CallDataBase
	{
		FinishCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{std::move(pContext)}
		{
			Type = CallDataType::Finish;
		}
	};

	struct ConnectCallData : public CallDataBase
	{
		ConnectCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{std::move(pContext)}
		{
			Type = CallDataType::Connect;
		}
	};

	struct ReadMessageCallData : public CallDataBase
	{
		ReadMessageCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{std::move(pContext)}
		{
			Type = CallDataType::ReadMessage;
		}

		evtc_rpc::Message Message;
	};

	struct WriteEventCallData : public CallDataBase
	{
		WriteEventCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{std::move(pContext)}
		{
			Type = CallDataType::WriteEvent;
		}
	};

	struct DisconnectCallData : public CallDataBase
	{
		DisconnectCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{std::move(pContext)}
		{
			Type = CallDataType::Disconnect;
		}
	};

public:
	evtc_rpc_server(const char* pListeningEndpoint);
	~evtc_rpc_server();

	static void ThreadStartServe(void* pThis);
	void Serve();
	void Shutdown();

#ifndef TEST
private:
#endif
	void HandleConnect(ConnectCallData* pCallData);
	void HandleReadMessage(ReadMessageCallData* pCallData);
	void HandleWriteEvent(WriteEventCallData* pCallData);

	const char* HandleRegisterSelf(uint16_t pInstanceId, std::string_view pAccountName, std::shared_ptr<ConnectionContext>& pClient);
	const char* HandleSetSelfId(uint16_t pInstanceId, std::shared_ptr<ConnectionContext>& pClient);
	const char* HandleAddPeer(uint16_t pInstanceId, std::string_view pAccountName, std::shared_ptr<ConnectionContext>& pClient);
	const char* HandleRemovePeer(uint16_t pInstanceId, std::shared_ptr<ConnectionContext>& pClient);
	const char* HandleCombatEvent(const cbtevent& pEvent, std::shared_ptr<ConnectionContext>& pClient);

	void SendEvent(const evtc_rpc::messages::CombatEvent& pEvent, WriteEventCallData* pCallData, const std::shared_ptr<ConnectionContext>& pClient);
	void ForceDisconnect(const char* pErrorMessage, const std::shared_ptr<ConnectionContext>& pClient);

	std::mutex mRegisteredAgentsLock;
	std::map<std::string, std::shared_ptr<ConnectionContext>> mRegisteredAgents;
	
	evtc_rpc::evtc_rpc::AsyncService mService;
	std::unique_ptr<grpc::Server> mServer;
	std::unique_ptr<grpc::ServerCompletionQueue> mCompletionQueue;
};
