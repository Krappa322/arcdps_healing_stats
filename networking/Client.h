#pragma once
#include "arcdps_structs_slim.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif _WIN32
#pragma warning(push, 0)
#pragma warning(disable : 4127)
#pragma warning(disable : 4702)
#pragma warning(disable : 5054)
#pragma warning(disable : 6385)
#pragma warning(disable : 6387)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#endif
#include <evtc_rpc.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#ifdef __clang__
#pragma clang diagnostic pop
#elif _WIN32
#pragma warning(pop)
#pragma warning(default : 4702)
#endif

#include <cassert>
#include <chrono>
#include <queue>
#include <memory>

struct evtc_rpc_client_status
{
	bool Connected = false;
	bool Encrypted = false;
	std::chrono::steady_clock::time_point ConnectTime;
	std::string Endpoint;
};

class evtc_rpc_client
{
	typedef void(*PeerCombatCallbackSignature)(cbtevent* pEvent);

	struct PeerInfo
	{
		uint16_t InstanceId = 0;
		std::string AccountName;
	};

	struct ConnectionContext
	{
		bool ForceDisconnected = false;
		bool WritePending = false;
		uint16_t RegisteredInstanceId = 0;
		std::map<uintptr_t /*UniqueId*/, PeerInfo> RegisteredPeers;

		grpc::ClientContext ClientContext;
		std::shared_ptr<grpc::Channel> Channel;
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
		AddPeerCallData(std::shared_ptr<ConnectionContext>&& pContext, uint16_t pPeerInstanceId, std::string&& pPeerAccountName)
			: CallDataBase{CallDataType::AddPeer, std::move(pContext)}
			, PeerInstanceId{pPeerInstanceId}
			, PeerAccountName{std::move(pPeerAccountName)}
		{
		}

		const uint16_t PeerInstanceId;
		const std::string PeerAccountName;
	};

	struct RemovePeerCallData : public CallDataBase
	{
		RemovePeerCallData(std::shared_ptr<ConnectionContext>&& pContext, uint16_t pPeerInstanceId)
			: CallDataBase{CallDataType::RemovePeer, std::move(pContext)}
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
	evtc_rpc_client(std::function<std::string()>&& pEndpointCallback, std::function<std::string()>&& pRootCertsCallback, std::function<void(cbtevent*, uint16_t)>&& pCombatEventCallback);

	evtc_rpc_client_status GetStatus();
	void SetEnabledStatus(bool pEnabledStatus);
	void SetBudgetMode(bool pBudgetMode);
	void SetDisableEncryption(bool pDisableEncryption);

	uintptr_t ProcessLocalEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
	uintptr_t ProcessAreaEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);

	static void ThreadStartServe(void* pThis);
	void Serve();
	void Shutdown();

	void FlushEvents(size_t pAcceptableQueueSize = 0);

#ifndef TEST
private:
#endif
	bool QueueEvent(CallDataBase* pCallData, bool pIsImportant);
	CallDataBase* TryGetPeerEvent();

	void ForceDisconnect(const std::shared_ptr<ConnectionContext>& pContext, const char* pErrorMessage);
	void HandleReadMessage(ReadMessageCallData* pCallData);
	void SendEvent(CallDataBase* pCallData);

	const std::function<std::string()> mEndpointCallback;
	const std::function<std::string()> mRootCertificatesCallback;
	const std::function<void(cbtevent*, uint16_t)> mCombatEventCallback;

	std::mutex mQueuedEventsLock;
	std::queue<CallDataBase*> mQueuedEvents;

	std::atomic_bool mDisabled{false};
	std::atomic_bool mBudgetMode{false};
	std::atomic_bool mDisableEncryption{false};
	std::atomic_bool mShouldShutdown{false};
	bool mShutdown = false;
	std::chrono::steady_clock::time_point mLastConnectionAttempt;

	std::shared_ptr<ConnectionContext> mConnectionContext;
	grpc::CompletionQueue mCompletionQueue;

	std::mutex mSelfInfoLock;
	std::string mAccountName;
	uint16_t mInstanceId = 0;

	std::mutex mPeerInfoLock;
	std::map<uintptr_t /*UniqueId*/, PeerInfo> mPeers;

	std::mutex mStatusLock;
	evtc_rpc_client_status mStatus;
};