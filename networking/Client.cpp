#include "Client.h"

#include "evtc_rpc_messages.h"
#include "../src/Common.h"
#include "../src/Log.h"

#include <algorithm>
#ifndef _WIN32
#include <unistd.h>
#endif

bool evtc_rpc_client::CallDataBase::IsWrite()
{
	switch (Type)
	{
	case CallDataType::Connect:
	case CallDataType::RegisterSelf:
	case CallDataType::AddPeer:
	case CallDataType::RemovePeer:
	case CallDataType::CombatEvent:
		return true;

	case CallDataType::WritesDone:
	case CallDataType::Finish:
	case CallDataType::ReadMessage:
	case CallDataType::Disconnect:
		return false;

	default:
		LOG("Invalid CallDataType %i", Type);
		assert(false);
		return false;
	}
}

void evtc_rpc_client::CallDataBase::Destruct()
{
	switch (Type)
	{
		case CallDataType::Connect:
		{
			ConnectCallData* message = static_cast<ConnectCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::WritesDone:
		{
			WritesDoneCallData* message = static_cast<WritesDoneCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::Finish:
		{
			FinishCallData* message = static_cast<FinishCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::ReadMessage:
		{
			ReadMessageCallData* message = static_cast<ReadMessageCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::RegisterSelf:
		{
			RegisterSelfCallData* message = static_cast<RegisterSelfCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::AddPeer:
		{
			AddPeerCallData* message = static_cast<AddPeerCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::RemovePeer:
		{
			RemovePeerCallData* message = static_cast<RemovePeerCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::CombatEvent:
		{
			CombatEventCallData* message = static_cast<CombatEventCallData*>(this);
			delete message;
			break;
		}
		case CallDataType::Disconnect:
		{
			DisconnectCallData* message = static_cast<DisconnectCallData*>(this);
			delete message;
			break;
		}

		default:
			LOG("Invalid CallDataType %i", Type);
			assert(false);
			break;
	}
}


evtc_rpc_client::evtc_rpc_client(std::function<std::string()>&& pEndpointCallback, std::function<std::string()>&& pRootCertificatesCallback, std::function<void(cbtevent*, uint16_t)>&& pCombatEventCallback)
	: mEndpointCallback{std::move(pEndpointCallback)}
	, mRootCertificatesCallback{std::move(pRootCertificatesCallback)}
	, mCombatEventCallback{std::move(pCombatEventCallback)}
	, mLastConnectionAttempt{std::chrono::steady_clock::time_point(std::chrono::seconds(0))}
{
}

evtc_rpc_client_status evtc_rpc_client::GetStatus()
{
	evtc_rpc_client_status status;

	{
		std::lock_guard status_lock{mStatusLock};
		status = mStatus;
	}

	return status;
}

void evtc_rpc_client::SetEnabledStatus(bool pEnabledStatus)
{
	mDisabled = !pEnabledStatus;
	LogI("Changed enabled status to {}", pEnabledStatus);
}

void evtc_rpc_client::SetBudgetMode(bool pBudgetMode)
{
	mBudgetMode = pBudgetMode;
	LogI("Changed budget mode to {}", pBudgetMode);
}

void evtc_rpc_client::SetDisableEncryption(bool pDisableEncryption)
{
	mDisableEncryption = pDisableEncryption;
	LogI("Changed disable encryption mode to {}", pDisableEncryption);
}

uintptr_t evtc_rpc_client::ProcessLocalEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* /*pSkillname*/, uint64_t pId, uint64_t /*pRevision*/)
{
	if (pEvent == nullptr)
	{
		if (pSourceAgent->elite != 0)
		{
			// Not agent adding event
			return 0;
		}

		if (pSourceAgent->prof != 0)
		{
			LOG("Register agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			if (pDestinationAgent->self != 0)
			{
				assert(pDestinationAgent->id <= UINT16_MAX);
				assert(pDestinationAgent->id > 0);
				assert(pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0');

				std::lock_guard lock(mSelfInfoLock);

				mAccountName = pDestinationAgent->name;
				mInstanceId = static_cast<uint16_t>(pDestinationAgent->id);
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);
			// pDestinationAgent is invalid in deregister events (keep trying to log it though - might have some useful information for debugging :))
			pDestinationAgent = nullptr;
		}

		return 0;
	}

	bool sendEvent = true;
	if (mBudgetMode.load(std::memory_order_relaxed) == true)
	{
		sendEvent = false;

		EventType eventType = GetEventType(pEvent, true);
		if (eventType == EventType::Healing || pEvent->is_statechange == CBTS_ENTERCOMBAT || pEvent->is_statechange == CBTS_EXITCOMBAT)
		{
			sendEvent = true;
		}
	}

	if (sendEvent == true)
	{
		CombatEventCallData* newEvent = new CombatEventCallData(*pEvent);
		if (QueueEvent(newEvent, false) == false)
		{
			DEBUGLOG("Dropping CombatEvent event %llu since queue is full", pId);
			delete newEvent;
		}
	}

	return 0;
}

uintptr_t evtc_rpc_client::ProcessAreaEvent(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* /*pSkillname*/, uint64_t /*pId*/, uint64_t /*pRevision*/)
{
	if (pEvent == nullptr)
	{
		if (pSourceAgent->elite != 0)
		{
			// Not agent adding event
			return 0;
		}

		if (pSourceAgent->prof != 0)
		{
			LOG("Register agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);

			// Don't send peer if it's ourselves (that makes no sense) and don't send peer if it has no account name (not a player?)
			if (pDestinationAgent->self == 0 &&
				(pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0'))
			{
				assert(pDestinationAgent->id <= UINT16_MAX);
				assert(pDestinationAgent->name != nullptr && pDestinationAgent->name[0] != '\0');

				std::lock_guard lock(mPeerInfoLock);
				auto [iter, inserted] = mPeers.try_emplace(pSourceAgent->id, PeerInfo{static_cast<uint16_t>(pDestinationAgent->id), pDestinationAgent->name});
				if (inserted == true)
				{
					LogD("Added peer {} {} {}", iter->first, iter->second.InstanceId, iter->second.AccountName);
				}
				else
				{
					LogE("Dropping peer {} {} {} since they're already registered as {} {}",
						pSourceAgent->id, static_cast<uint16_t>(pDestinationAgent->id), pDestinationAgent->name, iter->second.InstanceId, iter->second.AccountName);
				}
			}
		}
		else
		{
			LOG("Deregister agent %s %llu %x %x %u %u ; %s %llu %x %x %u %u",
				pSourceAgent->name, pSourceAgent->id, pSourceAgent->prof, pSourceAgent->elite, pSourceAgent->team, pSourceAgent->self,
				pDestinationAgent->name, pDestinationAgent->id, pDestinationAgent->prof, pDestinationAgent->elite, pDestinationAgent->team, pDestinationAgent->self);
			// pDestinationAgent is invalid in deregister events (keep trying to log it though - might have some useful information for debugging :))
			pDestinationAgent = nullptr;

			{
				std::lock_guard lock(mPeerInfoLock);
				size_t removedCount = mPeers.erase(pSourceAgent->id);
				if (removedCount == 1)
				{
					LogD("Removed peer {}", pSourceAgent->id);
				}
				else
				{
					LogT("Removing peer {} failed - removedCount={} (probably not a player)",
						pSourceAgent->id, removedCount);
				}
			}
		}

		return 0;
	}

	return 0;
}

void evtc_rpc_client::ThreadStartServe(void* pThis)
{
	reinterpret_cast<evtc_rpc_client*>(pThis)->Serve();
}

void evtc_rpc_client::Serve()
{
	LogD(">>");

	while (true)
	{
		//LOG("LOOP - mShouldShutdown=%s mWritePending=%s", BOOL_STR(mShouldShutdown), mConnectionContext == nullptr ? "null" : BOOL_STR(mConnectionContext->WritePending));

		void* tag = nullptr;
		bool ok = false;

		gpr_timespec timeout;
		timeout.tv_sec = 0;
		timeout.tv_nsec = 1000 * 1000; // 1 ms
		timeout.clock_type = GPR_TIMESPAN;
		grpc::CompletionQueue::NextStatus status = mCompletionQueue.AsyncNext(&tag, &ok, timeout);
		if (status == grpc::CompletionQueue::NextStatus::SHUTDOWN)
		{
			LOG("Got shutdown status");
			return;
		}
		else if (status == grpc::CompletionQueue::NextStatus::GOT_EVENT)
		{
			LOG("Notified on tag %p", tag);

			assert(tag != nullptr);
			CallDataBase* base = static_cast<CallDataBase*>(tag);

			if (ok == true)
			{
				switch (base->Type)
				{
					case CallDataType::ReadMessage:
					{
						ReadMessageCallData* message = static_cast<ReadMessageCallData*>(base);
						HandleReadMessage(message);
						break;
					}
					case CallDataType::Finish:
					{
						FinishCallData* message = static_cast<FinishCallData*>(base);
						LOG("(tag %p) got result %i '%s' '%s' %zu", tag, message->ReturnedStatus.error_code(), message->ReturnedStatus.error_message().c_str(), message->ReturnedStatus.error_details().c_str(), message->ReturnedStatus.error_details().size());
						break;
					}
					case CallDataType::Connect:
					{
						//ConnectCallData* message = static_cast<ConnectCallData*>(base);
						
						if (base->Context == mConnectionContext)
						{
							std::lock_guard lock{mStatusLock};
							mStatus.Connected = true;
							mStatus.ConnectTime = std::chrono::steady_clock::now();

							LOG("(tag %p) Successfully connected to %s", tag, mStatus.Endpoint.c_str());
						}
						break;
					}

					default:
						LOG("(tag %p) No action required for CallDataType %i", tag, base->Type);
						break;
				}
			}
			else
			{
				LOG("(tag %p) Got not-ok", tag);

				if (mShutdown == false)
				{
					switch (base->Type)
					{
					case CallDataType::Connect:
					{
						FinishCallData* queuedData = new FinishCallData(std::shared_ptr{base->Context});
						base->Context->Stream->Finish(&queuedData->ReturnedStatus, queuedData);

						LOG("(tag %p) Connection to remote failed. Queued %p to get error code", tag, queuedData);

						if (base->Context == mConnectionContext)
						{
							mConnectionContext = nullptr;
							
							{
								std::lock_guard status_lock{mStatusLock};
								mStatus.Connected = false;
							}
						}
						break;
					}
					case CallDataType::ReadMessage:
					{
						LOG("(tag %p) Remote broke connection", tag);
						if (base->Context == mConnectionContext)
						{
							mConnectionContext = nullptr;

							{
								std::lock_guard status_lock{mStatusLock};
								mStatus.Connected = false;
							}
						}
						break;
					}
					default:
						break;
					}
				}
			}

			if (base->IsWrite() == true)
			{
				LOG("(tag %p) Was write, setting mWritePending to false", tag);

				assert(base->Context->WritePending == true);
				base->Context->WritePending = false;
			}

			base->Destruct();
		}

		if (mShutdown == true)
		{
			// Nothing
		}
		else if (mShouldShutdown == true)
		{
			mShouldShutdown = false;
			mShutdown = true;

			if (mConnectionContext != nullptr)
			{
				mConnectionContext->ClientContext.TryCancel();
				LOG("ShouldShutdown==true and state==%i - shutting down completion queue", mConnectionContext->Channel->GetState(false));
			}
			else
			{
				LOG("ShouldShutdown==true and there is no active connection - shutting down completion queue");
			}

			mCompletionQueue.Shutdown();
		}
		else if (mDisabled == true)
		{
			if (mConnectionContext != nullptr)
			{
				// grpc doesn't give much of an interface to stop the calls, this is the best we can do. Queueing a Finish call
				// will not do anything, it leaves us at the mercy of the server to actually complete the underlying call.
				mConnectionContext->ClientContext.TryCancel();

				LogD("Client was disabled, cancelling calls");
			}
		}
		else if (mConnectionContext == nullptr)
		{
			if ((std::chrono::steady_clock::now() - mLastConnectionAttempt) > std::chrono::seconds{5})
			{
				mConnectionContext = std::make_shared<ConnectionContext>();

				std::string endpoint = mEndpointCallback();
				bool missingPort = false;
				bool disableEncryption = mDisableEncryption;
				if (endpoint.find(':') == endpoint.npos)
				{
					missingPort = true;
				}

				grpc::SslCredentialsOptions options;
				options.pem_root_certs = mRootCertificatesCallback();

				std::shared_ptr<grpc::ChannelCredentials> channel_creds;
				if (disableEncryption == true)
				{
					if (missingPort == true)
					{
						endpoint += ":80";
					}

					channel_creds = grpc::InsecureChannelCredentials();
				}
				else
				{
					if (missingPort == true)
					{
						endpoint += ":443";
					}

					channel_creds = grpc::SslCredentials(options);
				}

				mConnectionContext->Channel = grpc::CreateChannel(endpoint, channel_creds);
				mConnectionContext->Stub = evtc_rpc::evtc_rpc::NewStub(std::shared_ptr(mConnectionContext->Channel));

				ConnectCallData* queuedData1 = new ConnectCallData{std::shared_ptr(mConnectionContext)};
				mConnectionContext->Stream = mConnectionContext->Stub->AsyncConnect(&mConnectionContext->ClientContext, &mCompletionQueue, queuedData1);
				mConnectionContext->WritePending = true;

				ReadMessageCallData* queuedData2 = new ReadMessageCallData{std::shared_ptr(mConnectionContext)};
				queuedData2->Context->Stream->Read(&queuedData2->Message, queuedData2);

				mLastConnectionAttempt = std::chrono::steady_clock::now();
				LOG("Opening new connection to %s connect_tag=%p, read_tag=%p, root_certs_size=%zu", endpoint.c_str(), queuedData1, queuedData2, options.pem_root_certs.size());

				{
					std::lock_guard status_lock{mStatusLock};
					mStatus.Endpoint = endpoint;
					mStatus.Encrypted = !disableEncryption;
				}
			}
		}
		else if (mConnectionContext->WritePending == false)
		{
			CallDataBase* queuedData = nullptr;
			{
				std::lock_guard lock(mSelfInfoLock);

				// Either RegisteredInstanceId is 0 (meaning we aren't registered yet), or mInstanceId has changed because we changed instances
				if (mConnectionContext->RegisteredInstanceId != mInstanceId)
				{
					assert(mInstanceId != 0); // mInstanceId should never go back to zero after being set
					assert(mAccountName.size() > 0);
					queuedData = new RegisterSelfCallData{std::shared_ptr(mConnectionContext), mInstanceId, std::string(mAccountName)};

					LOG("(tag %p) Registering self %hu %s", queuedData, mInstanceId, mAccountName.c_str());
				}
			}

			if (queuedData == nullptr && mConnectionContext->RegisteredInstanceId != 0)
			{
				queuedData = TryGetPeerEvent();
			}

			if (queuedData == nullptr && mConnectionContext->RegisteredInstanceId != 0)
			{
				std::lock_guard lock(mQueuedEventsLock);

				if (mQueuedEvents.size() > 0)
				{
					queuedData = mQueuedEvents.front();
					mQueuedEvents.pop();
					queuedData->Context = std::shared_ptr(mConnectionContext);
				}
			}

			if (queuedData != nullptr)
			{
				SendEvent(queuedData);

				assert(queuedData->IsWrite() == true);
				mConnectionContext->WritePending = true;
			}
		}
	}
}

void evtc_rpc_client::Shutdown()
{
	mShouldShutdown = true;
}

void evtc_rpc_client::FlushEvents(size_t pAcceptableQueueSize)
{
	while (true)
	{
		{
			std::lock_guard lock(mQueuedEventsLock);
			if (mQueuedEvents.size() <= pAcceptableQueueSize)
			{
				return;
			}
		}

#ifdef _WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}
}

bool evtc_rpc_client::QueueEvent(CallDataBase* pCallData, bool pIsImportant)
{
	std::lock_guard lock(mQueuedEventsLock);

	if ((pIsImportant == false && mQueuedEvents.size() > 5000) || mQueuedEvents.size() > 10000)
	{
		return false;
	}

	mQueuedEvents.push(pCallData);
	return true;
}

evtc_rpc_client::CallDataBase* evtc_rpc_client::TryGetPeerEvent()
{
	std::lock_guard lock(mPeerInfoLock);

	// First deregister any no longer registered accounts
	for (auto registered = mConnectionContext->RegisteredPeers.begin();
		registered != mConnectionContext->RegisteredPeers.end();
		registered++)
	{
		auto peersIter = mPeers.find(registered->first);
		if (peersIter == mPeers.end())
		{
			LogT("Deregistering no longer registered peer {} {} {}",
				registered->first, registered->second.InstanceId, registered->second.AccountName);
		}
		else if (registered->second.InstanceId != peersIter->second.InstanceId || registered->second.AccountName != peersIter->second.AccountName)
		{
			LogT("Deregistering peer {} that changed {} {} -> {} {}",
				registered->first, registered->second.InstanceId, registered->second.AccountName, peersIter->second.InstanceId, peersIter->second.AccountName);
		}
		else
		{
			continue;
		}

		uint16_t id = registered->second.InstanceId;
		mConnectionContext->RegisteredPeers.erase(registered);
		return new RemovePeerCallData(std::shared_ptr(mConnectionContext), id);
	}

	// Then register any new accounts
	for (auto peers = mPeers.begin();
		peers != mPeers.end();
		peers++)
	{
		auto registeredIter = mConnectionContext->RegisteredPeers.find(peers->first);
		if (registeredIter != mConnectionContext->RegisteredPeers.end())
		{
			// Already registered (and we know they match - we checked in the loop above)
			assert(registeredIter->second.InstanceId == peers->second.InstanceId);
			assert(registeredIter->second.AccountName == peers->second.AccountName);
			continue;
		}

		LogT("Registering new peer {} {} {}",
			peers->first, peers->second.InstanceId, peers->second.AccountName);

		auto [iter, inserted] = mConnectionContext->RegisteredPeers.try_emplace(peers->first, peers->second);
		assert(inserted == true);
		return new AddPeerCallData(std::shared_ptr(mConnectionContext), peers->second.InstanceId, std::string{peers->second.AccountName});
	}

	return nullptr;
}

void evtc_rpc_client::ForceDisconnect(const std::shared_ptr<ConnectionContext>& pContext, const char* /*pErrorMessage*/)
{
	if (pContext->ForceDisconnected == true)
	{
		LOG("Ignoring ForceDisconnect since client is already disconnected");
		return;
	}

	if (mConnectionContext == pContext)
	{
		// Queue finish...

		mConnectionContext = nullptr;
		LOG("Cleared existing connection");
	}
	else
	{
		LOG("Existing connection is not same as the one being disconnected");
	}
}

void evtc_rpc_client::HandleReadMessage(ReadMessageCallData* pCallData)
{
	using namespace evtc_rpc::messages;

	// Add a new ReadMessageCallData so we can read the next message
	if (mShutdown == false)
	{
		ReadMessageCallData* queuedData = new ReadMessageCallData{std::shared_ptr(pCallData->Context)};
		pCallData->Context->Stream->Read(&queuedData->Message, queuedData);
	}

	const std::string& blob = pCallData->Message.blob();
	const char* data = blob.data();
	size_t dataSize = blob.size();

	if (dataSize < sizeof(Header))
	{
		LOG("(tag %p) data too short for header (%zu vs %zu)",
			pCallData, dataSize, sizeof(Header));
		ForceDisconnect(pCallData->Context, "short message header");
		return;
	}

	Header header;
	memcpy(&header, data, sizeof(Header));
	data += sizeof(Header);
	dataSize -= sizeof(Header);

	if (header.MessageVersion != 1)
	{
		LOG("(tag %p) incorrect version %u", pCallData, header.MessageVersion);
		ForceDisconnect(pCallData->Context, "incorrect version");
		return;
	}

	switch (header.MessageType)
	{
	case Type::CombatEvent:
		if (dataSize != sizeof(CombatEvent))
		{
			LOG("(tag %p) incorrect length for CombatEvent message (%zu vs %zu)",
				pCallData, dataSize, sizeof(CombatEvent));
			ForceDisconnect(pCallData->Context, "short SetSelfAccountName content");
			return;
		}

		CombatEvent message;
		memcpy(&message, data, sizeof(CombatEvent));
		data += sizeof(CombatEvent);
		dataSize -= sizeof(message);

		mCombatEventCallback(&message.Event, message.SenderInstanceId);
		LOG("Received CombatEvent source %hu target %hu skill %u value %i",
			message.Event.src_instid, message.Event.dst_instid, message.Event.skillid, message.Event.value);
		break;

	default:
		LOG("(tag %p) incorrect type %u", pCallData, header.MessageType);
		return;
	}

	LOG("(tag %p) <<", pCallData);
}

void evtc_rpc_client::SendEvent(CallDataBase* pCallData)
{
	using namespace evtc_rpc::messages;

	char buffer[1024];
	char* bufferpos = buffer;

	Header header;
	header.MessageVersion = 1;
	bufferpos += sizeof(header); // Reserve space for the header in the buffer

	switch (pCallData->Type)
	{
		case CallDataType::RegisterSelf:
		{
			RegisterSelfCallData* calldata = static_cast<RegisterSelfCallData*>(pCallData);
			assert(calldata->SelfAccountName.size() < UINT8_MAX);

			if (pCallData->Context->RegisteredInstanceId == 0)
			{
				header.MessageType = Type::RegisterSelf;

				RegisterSelf message;
				message.SelfId = calldata->SelfInstanceId;
				message.SelfAccountNameLength = static_cast<uint8_t>(calldata->SelfAccountName.size());

				memcpy(bufferpos, &message, sizeof(message));
				bufferpos += sizeof(message);

				memcpy(bufferpos, calldata->SelfAccountName.data(), calldata->SelfAccountName.size());
				bufferpos += calldata->SelfAccountName.size();

				pCallData->Context->RegisteredInstanceId = message.SelfId;

				LOG("(tag %p) Sending RegisterSelf %hu %s", pCallData, message.SelfId, calldata->SelfAccountName.c_str());
			}
			else
			{
				header.MessageType = Type::SetSelfId;

				SetSelfId message;
				message.SelfId = calldata->SelfInstanceId;

				memcpy(bufferpos, &message, sizeof(message));
				bufferpos += sizeof(message);

				pCallData->Context->RegisteredInstanceId = message.SelfId;

				LOG("(tag %p) Sending SetSelfId %hu", pCallData, message.SelfId);
			}
			break;
		}
		case CallDataType::AddPeer:
		{
			AddPeerCallData* calldata = static_cast<AddPeerCallData*>(pCallData);
			assert(calldata->PeerAccountName.size() < UINT8_MAX);

			header.MessageType = Type::AddPeer;

			AddPeer message;
			message.PeerId = calldata->PeerInstanceId;
			message.PeerAccountNameLength = static_cast<uint8_t>(calldata->PeerAccountName.size());

			memcpy(bufferpos, &message, sizeof(message));
			bufferpos += sizeof(message);

			memcpy(bufferpos, calldata->PeerAccountName.data(), calldata->PeerAccountName.size());
			bufferpos += calldata->PeerAccountName.size();

			LOG("(tag %p) Sending AddPeer %hu %s", pCallData, message.PeerId, calldata->PeerAccountName.c_str());
			break;
		}
		case CallDataType::RemovePeer:
		{
			RemovePeerCallData* calldata = static_cast<RemovePeerCallData*>(pCallData);

			header.MessageType = Type::RemovePeer;

			RemovePeer message;
			message.PeerId = calldata->PeerInstanceId;

			memcpy(bufferpos, &message, sizeof(message));
			bufferpos += sizeof(message);

			LOG("(tag %p) Sending RemovePeer %hu", pCallData, message.PeerId);
			break;
		}
		case CallDataType::CombatEvent:
		{
			CombatEventCallData* calldata = static_cast<CombatEventCallData*>(pCallData);

			header.MessageType = Type::CombatEvent;

			CombatEvent message;
			message.Event = calldata->Event;
			message.SenderInstanceId = 0;

			memcpy(bufferpos, &message, sizeof(message));
			bufferpos += sizeof(message);

			LOG("(tag %p) Sending CombatEvent source %hu target %hu skill %u value %i", pCallData, message.Event.src_instid, message.Event.dst_instid, message.Event.skillid, message.Event.value);
			break;
		}

		default:
			LOG("Invalid CallDataType %i", pCallData->Type);
			assert(false);
			return;
	}

	// Copy the header into the buffer now that it's populated
	memcpy(buffer, &header, sizeof(header));

	evtc_rpc::Message rpc_message;
	rpc_message.set_blob(buffer, bufferpos - buffer);
	pCallData->Context->Stream->Write(rpc_message, pCallData);
}
