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

#include <deque>
#include <cassert>

class evtc_rpc_client
{
	typedef void(*PeerCombatCallbackSignature)(cbtevent* pEvent);

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
		CallDataType Type = CallDataType::Invalid;

		bool IsWrite();
		void Destruct();
	};

	struct ConnectCallData : public CallDataBase
	{
		ConnectCallData()
		{
			Type = CallDataType::Connect;
		}
	};

	struct FinishCallData : public CallDataBase
	{
		FinishCallData()
		{
			Type = CallDataType::Finish;
		}

		grpc::Status ReturnedStatus;
	};

	struct WritesDoneCallData : public CallDataBase
	{
		WritesDoneCallData()
		{
			Type = CallDataType::WritesDone;
		}
	};

	struct ReadMessageCallData : public CallDataBase
	{
		ReadMessageCallData()
		{
			Type = CallDataType::ReadMessage;
		}

		evtc_rpc::Message Message;
	};

	struct RegisterSelfCallData : public CallDataBase
	{
		RegisterSelfCallData(uint16_t pSelfInstanceId, std::string&& pSelfAccountName)
			: SelfInstanceId{pSelfInstanceId}
			, SelfAccountName{std::move(pSelfAccountName)}
		{
			Type = CallDataType::RegisterSelf;
		}

		const uint16_t SelfInstanceId;
		const std::string SelfAccountName;
	};

	struct AddPeerCallData : public CallDataBase
	{
		AddPeerCallData(uint16_t pPeerInstanceId, std::string&& pPeerAccountName)
			: PeerInstanceId{pPeerInstanceId}
			, PeerAccountName{std::move(pPeerAccountName)}
		{
			Type = CallDataType::AddPeer;
		}

		const uint16_t PeerInstanceId;
		const std::string PeerAccountName;
	};

	struct RemovePeerCallData : public CallDataBase
	{
		RemovePeerCallData(uint16_t pPeerInstanceId)
			: PeerInstanceId{pPeerInstanceId}
		{
			Type = CallDataType::RemovePeer;
		}

		const uint16_t PeerInstanceId;
	};


	struct CombatEventCallData : public CallDataBase
	{
		CombatEventCallData(const cbtevent& pEvent)
			: Event{pEvent}
		{
			Type = CallDataType::CombatEvent;
		}

		const cbtevent Event;
	};

	struct DisconnectCallData : public CallDataBase
	{
		DisconnectCallData()
		{
			Type = CallDataType::Disconnect;
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
	void EnsureConnected();
	void ForceDisconnect(const char* pErrorMessage);
	void HandleReadMessage(ReadMessageCallData* pCallData);
	void QueueEvent(CallDataBase* pCallData);

	void HandleCombatEvent(cbtevent* pEvent);

	const std::function<void(cbtevent*, uint16_t)> mCombatEventCallback;

	std::mutex mQueuedEventsLock;
	std::deque<CallDataBase*> mQueuedEvents;

	bool mForceDisconnected = false;
	bool mRegistered = false;
	std::atomic_bool mShouldShutdown = false;
	bool mShutdown = false;
	bool mWritePending = false;

	std::unique_ptr<grpc::ClientContext> mClientContext;
	std::unique_ptr<grpc::ClientAsyncReaderWriter<evtc_rpc::Message, evtc_rpc::Message>> mStream;

	grpc::CompletionQueue mCompletionQueue;
	std::unique_ptr<evtc_rpc::evtc_rpc::Stub> mStub;
	std::string mEndpoint;
};