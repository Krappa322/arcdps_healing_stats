#pragma once
#include "../arcdps_mock/arcdps-extension/arcdps_structs.h"

#pragma warning(push, 0)
#include "evtc_rpc.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#pragma warning(pop)

#include <cassert>
#include <deque>
#include <memory>

class evtc_rpc_client
{
	typedef void(*PeerCombatCallbackSignature)(cbtevent* pEvent);

	struct ConnectionContext
	{
		bool ForceDisconnected = false;
		bool WritePending = false;
		uint16_t RegisteredInstanceId = 0;

		grpc::ClientContext ClientContext;
		std::unique_ptr<grpc::ClientAsyncReaderWriter<evtc_rpc::Message, evtc_rpc::Message>> Stream;
		std::unique_ptr<evtc_rpc::evtc_rpc::Stub> Stub;
	};

	enum class CallDataType
	{
		Connect,
		WritesDone,
		Finish,
		ReadMessage,
		RegisterSelf,
		AddPeer,
		RemovePeer,
		CombatEvent,
		Disconnect,

		Invalid
	};

	struct CallDataBase
	{
		CallDataBase(CallDataType pType, std::shared_ptr<ConnectionContext>&& pContext)
			: Type{pType}
			, Context{std::move(pContext)}
		{
		}

		const CallDataType Type;
		std::shared_ptr<ConnectionContext> Context;

		bool IsWrite();
		void Destruct();
	};

	struct ConnectCallData : public CallDataBase
	{
		ConnectCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{CallDataType::Connect, std::move(pContext)}
		{
		}
	};

	struct FinishCallData : public CallDataBase
	{
		FinishCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{CallDataType::Finish, std::move(pContext)}
		{
		}

		grpc::Status ReturnedStatus;
	};

	struct WritesDoneCallData : public CallDataBase
	{
		WritesDoneCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{CallDataType::WritesDone, std::move(pContext)}
		{
		}
	};

	struct ReadMessageCallData : public CallDataBase
	{
		ReadMessageCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{CallDataType::ReadMessage, std::move(pContext)}
		{
		}

		evtc_rpc::Message Message;
	};

	struct RegisterSelfCallData : public CallDataBase
	{
		RegisterSelfCallData(std::shared_ptr<ConnectionContext>&& pContext, uint16_t pSelfInstanceId, std::string&& pSelfAccountName)
			: CallDataBase{CallDataType::RegisterSelf, std::move(pContext)}
			, SelfInstanceId{pSelfInstanceId}
			, SelfAccountName{std::move(pSelfAccountName)}
		{
		}

		const uint16_t SelfInstanceId;
		const std::string SelfAccountName;
	};

	struct AddPeerCallData : public CallDataBase
	{
		AddPeerCallData(uint16_t pPeerInstanceId, std::string&& pPeerAccountName)
			: CallDataBase{CallDataType::AddPeer, nullptr}
			, PeerInstanceId{pPeerInstanceId}
			, PeerAccountName{std::move(pPeerAccountName)}
		{
		}

		const uint16_t PeerInstanceId;
		const std::string PeerAccountName;
	};

	struct RemovePeerCallData : public CallDataBase
	{
		RemovePeerCallData(uint16_t pPeerInstanceId)
			: CallDataBase{CallDataType::RemovePeer, nullptr}
			, PeerInstanceId{pPeerInstanceId}
		{
		}

		const uint16_t PeerInstanceId;
	};


	struct CombatEventCallData : public CallDataBase
	{
		CombatEventCallData(const cbtevent& pEvent)
			: CallDataBase{CallDataType::CombatEvent, nullptr}
			, Event{pEvent}
		{
		}

		const cbtevent Event;
	};

	struct DisconnectCallData : public CallDataBase
	{
		DisconnectCallData(std::shared_ptr<ConnectionContext>&& pContext)
			: CallDataBase{CallDataType::Disconnect, std::move(pContext)}
		{
		}
	};

public:
	evtc_rpc_client(std::string&& pEndpoint, std::function<void(cbtevent*, uint16_t)>&& pCombatEventCallback);

	uintptr_t ProcessLocalEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	uintptr_t ProcessAreaEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);

	static void ThreadStartServe(void* pThis);
	void Serve();
	void Shutdown();

	void FlushEvents();

#ifndef TEST
private:
#endif
	void ForceDisconnect(const std::shared_ptr<ConnectionContext>& pContext, const char* pErrorMessage);
	void HandleReadMessage(ReadMessageCallData* pCallData);
	void QueueEvent(CallDataBase* pCallData);

	void HandleCombatEvent(cbtevent* pEvent);

	const std::function<void(cbtevent*, uint16_t)> mCombatEventCallback;

	std::mutex mQueuedEventsLock;
	std::deque<CallDataBase*> mQueuedEvents;

	std::atomic_bool mShouldShutdown = false;
	bool mShutdown = false;

	std::shared_ptr<ConnectionContext> mConnectionContext;
	grpc::CompletionQueue mCompletionQueue;
	std::string mEndpoint;

	std::mutex mSelfInfoLock;
	std::string mAccountName;
	uint16_t mInstanceId = 0;
};