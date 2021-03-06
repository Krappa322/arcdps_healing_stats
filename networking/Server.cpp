#include "Server.h"

#include "../src/Log.h"

evtc_rpc_server::evtc_rpc_server(const char* pListeningEndpoint, const grpc::SslServerCredentialsOptions* pCredentialsOptions)
{
	grpc::ServerBuilder builder;
	builder.AddChannelArgument("GRPC_ARG_KEEPALIVE_TIME_MS", 60000);
	builder.AddChannelArgument("GRPC_ARG_KEEPALIVE_TIMEOUT_MS", 10000);
	builder.AddChannelArgument("GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS", 0); // We really don't care about cancelling connections that are not doing anything
	builder.AddChannelArgument("GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA", 0); // Keep sending keepalive pings forever
	builder.AddChannelArgument("GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS", 300000); // Does this need configuring? Client is not supposed to be sending keepalives. Keeping it at default
	builder.AddChannelArgument("GRPC_ARG_HTTP2_MAX_PING_STRIKES", 2); // Default

	if (pCredentialsOptions != nullptr)
	{
		auto channel_creds = grpc::SslServerCredentials(*pCredentialsOptions);
		builder.AddListeningPort(pListeningEndpoint, channel_creds);
	}
	else
	{
		builder.AddListeningPort(pListeningEndpoint, grpc::InsecureServerCredentials());
	}

	builder.RegisterService(&mService);

	mCompletionQueue = builder.AddCompletionQueue();
	mServer = builder.BuildAndStart();
}

evtc_rpc_server::~evtc_rpc_server()
{
	assert(mRegisteredAgents.size() == 0);
}

void evtc_rpc_server::ThreadStartServe(void* pThis)
{
	reinterpret_cast<evtc_rpc_server*>(pThis)->Serve();
}

void evtc_rpc_server::Serve()
{
	ConnectCallData* queuedData = new ConnectCallData{std::make_shared<ConnectionContext>()};
	{
		std::lock_guard lock(mRegisteredAgentsLock);
		queuedData->Context->Iterator = mRegisteredAgents.end();
	}
	mService.RequestConnect(&queuedData->Context->ServerContext, &queuedData->Context->Stream, mCompletionQueue.get(), mCompletionQueue.get(), queuedData);

	LOG("(tag %p) Queued Connect", queuedData);

	while (true)
	{
		LOG("Server loop");
		void* tag;
		bool ok;
		if (mCompletionQueue->Next(&tag, &ok) == false)
		{
			LOG("mCompletionQueue->Next returned false, returning");
			return;
		}

		std::shared_lock lock{mShutdownLock};

		if (ok == false || mIsShutdown == true)
		{
			LOG("(tag %p) Got not-ok or shutdown(%s) (type %u)", tag, BOOL_STR(mIsShutdown), static_cast<CallDataBase*>(tag)->Type);

			switch (static_cast<CallDataBase*>(tag)->Type)
			{
			case CallDataType::Connect:
			{
				ConnectCallData* message = static_cast<ConnectCallData*>(tag);
				delete message;
				break;
			}
			case CallDataType::ReadMessage:
			{
				ReadMessageCallData* message = static_cast<ReadMessageCallData*>(tag);
				LOG("(client %p tag %p) ReadMessage got not-ok, closing connection", message->Context.get(), tag);

				ForceDisconnect("shutdown by client", message->Context);

				delete message;
				break;
			}
			case CallDataType::WriteEvent:
			{
				WriteEventCallData* message = static_cast<WriteEventCallData*>(tag);
				delete message;
				break;
			}
			case CallDataType::Disconnect:
			{
				DisconnectCallData* message = static_cast<DisconnectCallData*>(tag);
				delete message;
				break;
			}
			default:
				LOG("Invalid CallDataType %i", static_cast<CallDataBase*>(tag)->Type);
				assert(false);
			}

			continue;
		}

		switch (static_cast<CallDataBase*>(tag)->Type)
		{
		case CallDataType::Connect:
		{
			ConnectCallData* message = static_cast<ConnectCallData*>(tag);
			HandleConnect(message);

			queuedData->Context = std::make_shared<ConnectionContext>();
			{
				std::lock_guard agents_lock{mRegisteredAgentsLock};
				queuedData->Context->Iterator = mRegisteredAgents.end();
			}
			mService.RequestConnect(&queuedData->Context->ServerContext, &queuedData->Context->Stream, mCompletionQueue.get(), mCompletionQueue.get(), queuedData);
			break;
		}
		case CallDataType::ReadMessage:
		{
			ReadMessageCallData* message = static_cast<ReadMessageCallData*>(tag);
			HandleReadMessage(message);

			// Requeue the same tag for a new read. This has to be done after the the handler is done to ensure there isn't a race between two ReadMessages
			message->Context->Stream.Read(&message->Message, message);
			break;
		}
		case CallDataType::WriteEvent:
		{
			WriteEventCallData* message = static_cast<WriteEventCallData*>(tag);
			HandleWriteEvent(message);
			break;
		}
		case CallDataType::Disconnect:
		{
			DisconnectCallData* message = static_cast<DisconnectCallData*>(tag);
			delete message;
			break;
		}
		default:
			LOG("Invalid CallDataType %i", static_cast<CallDataBase*>(tag)->Type);
			assert(false);
		}
	}
}

void evtc_rpc_server::Shutdown()
{
	std::unique_lock lock{mShutdownLock};

	mServer->Shutdown();
	mCompletionQueue->Shutdown();
	mIsShutdown = true;
}

void evtc_rpc_server::HandleConnect(ConnectCallData* pCallData)
{
	// Add a ReadMessageCallData so we can start reading messages on this new connection
	{
		ReadMessageCallData* queuedData = new ReadMessageCallData{std::shared_ptr{pCallData->Context}};
		queuedData->Context->Stream.Read(&queuedData->Message, queuedData);
	}

	LOG("(client %p tag %p) new connection from %s",
		pCallData->Context.get(), pCallData, pCallData->Context->ServerContext.peer().c_str());
}

void evtc_rpc_server::HandleReadMessage(ReadMessageCallData* pCallData)
{
	using namespace evtc_rpc::messages;

	const std::string& blob = pCallData->Message.blob();
	const char* data = blob.data();
	size_t dataSize = blob.size();

	if (dataSize < sizeof(Header))
	{
		LOG("(client %p tag %p) data too short for header (%zu vs %zu)",
			pCallData->Context.get(), pCallData, dataSize, sizeof(Header));
		ForceDisconnect("short message header", pCallData->Context);
		return;
	}

	Header header;
	memcpy(&header, data, sizeof(Header));
	data += sizeof(Header);
	dataSize -= sizeof(Header);

	if (header.MessageVersion != 1)
	{
		LOG("(client %p tag %p) incorrect version %u", pCallData->Context.get(), pCallData, header.MessageVersion);
		ForceDisconnect("incorrect version", pCallData->Context);
		return;
	}

	switch (header.MessageType)
	{
	case Type::RegisterSelf:
	{
		if (dataSize < sizeof(RegisterSelf))
		{
			LOG("(client %p tag %p) data too short for RegisterSelf message (%zu vs %zu)",
				pCallData->Context.get(), pCallData, dataSize, sizeof(RegisterSelf));
			ForceDisconnect("short RegisterSelf content", pCallData->Context);
			return;
		}

		RegisterSelf message;
		memcpy(&message, data, sizeof(RegisterSelf));
		data += sizeof(RegisterSelf);
		dataSize -= sizeof(RegisterSelf);

		if (dataSize != message.SelfAccountNameLength)
		{
			LOG("(client %p tag %p) incorrect RegisterSelf name length (%zu vs %hhu)",
				pCallData->Context.get(), pCallData, dataSize, message.SelfAccountNameLength);
			ForceDisconnect("mismatched RegisterSelf length", pCallData->Context);
			return;
		}

		const char* error = HandleRegisterSelf(message.SelfId, std::string_view{data, message.SelfAccountNameLength}, pCallData->Context);
		if (error != nullptr)
		{
			LOG("(client %p tag %p) HandleRegisterSelf failed - %s", pCallData->Context.get(), pCallData, error);
			ForceDisconnect(error, pCallData->Context);
			return;
		}
		break;
	}
	case Type::SetSelfId:
	{
		if (dataSize != sizeof(SetSelfId))
		{
			LOG("(client %p tag %p) data length mismatch for SetSelfId message (%zu vs %zu)",
				pCallData->Context.get(), pCallData, dataSize, sizeof(SetSelfId));
			ForceDisconnect("short SetSelfId content", pCallData->Context);
			return;
		}

		SetSelfId message;
		memcpy(&message, data, sizeof(SetSelfId));
		data += sizeof(SetSelfId);
		dataSize -= sizeof(SetSelfId);

		const char* error = HandleSetSelfId(message.SelfId, pCallData->Context);
		if (error != nullptr)
		{
			LOG("(client %p tag %p) HandleSetSelfId failed - %s", pCallData->Context.get(), pCallData, error);
			ForceDisconnect(error, pCallData->Context);
			return;
		}
		break;
	}
	case Type::AddPeer:
	{
		if (dataSize < sizeof(AddPeer))
		{
			LOG("(client %p tag %p) data too short for AddPeer message (%zu vs %zu)",
				pCallData->Context.get(), pCallData, dataSize, sizeof(AddPeer));
			ForceDisconnect("short AddPeer content", pCallData->Context);
			return;
		}

		AddPeer message;
		memcpy(&message, data, sizeof(AddPeer));
		data += sizeof(AddPeer);
		dataSize -= sizeof(AddPeer);

		if (dataSize != message.PeerAccountNameLength)
		{
			LOG("(client %p tag %p) incorrect AddPeer name length (%zu vs %hhu)",
				pCallData->Context.get(), pCallData, dataSize, message.PeerAccountNameLength);
			ForceDisconnect("mismatched AddPeer length", pCallData->Context);
			return;
		}

		const char* error = HandleAddPeer(message.PeerId, std::string_view{data, message.PeerAccountNameLength}, pCallData->Context);
		if (error != nullptr)
		{
			LOG("(client %p tag %p) HandleAddPeer failed - %s", pCallData->Context.get(), pCallData, error);
			ForceDisconnect(error, pCallData->Context);
			return;
		}
		break;
	}
	case Type::RemovePeer:
	{
		if (dataSize != sizeof(RemovePeer))
		{
			LOG("(client %p tag %p) data length mismatch for RemovePeer message (%zu vs %zu)",
				pCallData->Context.get(), pCallData, dataSize, sizeof(RemovePeer));
			ForceDisconnect("RemovePeer size mismatch", pCallData->Context);
			return;
		}

		RemovePeer message;
		memcpy(&message, data, sizeof(RemovePeer));
		data += sizeof(RemovePeer);
		dataSize -= sizeof(RemovePeer);

		const char* error = HandleRemovePeer(message.PeerId, pCallData->Context);
		if (error != nullptr)
		{
			LOG("(client %p tag %p) HandleRemovePeer failed - %s", pCallData->Context.get(), pCallData, error);
			ForceDisconnect(error, pCallData->Context);
			return;
		}
		break;
	}
	case Type::CombatEvent:
	{
		if (dataSize != sizeof(CombatEvent))
		{
			LOG("(client %p tag %p) data length mismatch for CombatEvent message (%zu vs %zu)",
				pCallData->Context.get(), pCallData, dataSize, sizeof(CombatEvent));
			ForceDisconnect("CombatEvent size mismatch", pCallData->Context);
			return;
		}

		CombatEvent message;
		memcpy(&message, data, sizeof(CombatEvent));
		data += sizeof(CombatEvent);
		dataSize -= sizeof(CombatEvent);

		const char* error = HandleCombatEvent(message.Event, pCallData->Context);
		if (error != nullptr)
		{
			LOG("(client %p tag %p) HandleCombatEvent failed - %s", pCallData->Context.get(), pCallData, error);
			ForceDisconnect(error, pCallData->Context);
			return;
		}
		break;
	}

	default:
		LOG("(client %p tag %p) incorrect type %u", pCallData->Context.get(), pCallData, header.MessageType);
		return;
	}

	LOG("(client %p tag %p) <<", pCallData->Context.get(), pCallData);
}

void evtc_rpc_server::HandleWriteEvent(WriteEventCallData* pCallData)
{
	std::lock_guard lock(pCallData->Context->WriteLock);

	assert(pCallData->Context->WritePending == true);
	pCallData->Context->WritePending = false;
	
	if (pCallData->Context->QueuedEvents.size() > 0)
	{
		SendEvent(pCallData->Context->QueuedEvents.front(), pCallData, pCallData->Context);
		pCallData->Context->QueuedEvents.pop_front();
	}
	else
	{
		LOG("(client %p tag %p) No more events queued", pCallData->Context.get(), pCallData);
		delete pCallData;
	}
}

const char* evtc_rpc_server::HandleRegisterSelf(uint16_t pInstanceId, std::string_view pAccountName, std::shared_ptr<ConnectionContext>& pClient)
{
	std::lock_guard lock(mRegisteredAgentsLock);

	// Check this under lock, mRegisteredAgentsLock also guards the ReceivedAccountName flag
	if (pClient->Iterator != mRegisteredAgents.end())
	{
		LOG("(client %p) this connection already has a registered account name", pClient.get());
		return "already registered account name on this connection";
	}

	std::string accountName{pAccountName};
	auto [newEntry, inserted] = mRegisteredAgents.try_emplace(std::move(accountName), std::shared_ptr{pClient});
	if (inserted == false)
	{
		LOG("(client %p) account name %s is already registered from another connection", pClient.get(), accountName.c_str());
		return "account name collision";
	}

	pClient->Iterator = newEntry;
	pClient->InstanceId = pInstanceId;

	LOG("(client %p) registered account %s %hu", pClient.get(), newEntry->first.c_str(), newEntry->second->InstanceId);
	return nullptr;
}

const char* evtc_rpc_server::HandleSetSelfId(uint16_t pInstanceId, std::shared_ptr<ConnectionContext>& pClient)
{
	std::lock_guard lock(mRegisteredAgentsLock);

	// Check this under lock, mRegisteredAgentsLock also guards the ReceivedAccountName flag
	if (pClient->Iterator == mRegisteredAgents.end())
	{
		LOG("(client %p) this connection is not registered yet", pClient.get());
		return "not registered yet";
	}

	pClient->InstanceId = pInstanceId;

	LOG("(client %p) set self id to %hu", pClient.get(), pInstanceId);
	return nullptr;
}

const char* evtc_rpc_server::HandleAddPeer(uint16_t pInstanceId, std::string_view pAccountName, std::shared_ptr<ConnectionContext>& pClient)
{
	std::lock_guard lock(mRegisteredAgentsLock);

	// Check this under lock, mRegisteredAgentsLock also guards the ReceivedAccountName flag
	if (pClient->Iterator == mRegisteredAgents.end())
	{
		LOG("(client %p) this connection is not registered yet", pClient.get());
		return "not registered yet";
	}

	std::string accountName{pAccountName};
	auto [newEntry, inserted] = pClient->Peers.try_emplace(std::move(accountName), pInstanceId);
	if (inserted == false)
	{
		LOG("(client %p) peer %s is already registered (instance id %hu, new instance id is %hu)", pClient.get(), accountName.c_str(), newEntry->second, pInstanceId);
		return "peer already registered";
	}

	LOG("(client %p) added peer %s %hu", pClient.get(), newEntry->first.c_str(), newEntry->second);
	return nullptr;
}

const char* evtc_rpc_server::HandleRemovePeer(uint16_t pInstanceId, std::shared_ptr<ConnectionContext>& pClient)
{
	std::lock_guard lock(mRegisteredAgentsLock);

	// Check this under lock, mRegisteredAgentsLock also guards the ReceivedAccountName flag
	if (pClient->Iterator == mRegisteredAgents.end())
	{
		LOG("(client %p) this connection is not registered yet", pClient.get());
		return "not registered yet";
	}

	std::string removed_name = "";
	for (auto iter = pClient->Peers.begin(); iter != pClient->Peers.end(); iter++)
	{
		if (iter->second == pInstanceId)
		{
			removed_name = iter->first;
			pClient->Peers.erase(iter);
			break;
		}
	}
	
	if (removed_name == "")
	{
		LOG("(client %p) can't find peer with instance id %hu", pClient.get(), pInstanceId);
		return nullptr;
	}

	LOG("(client %p) removed peer %s %hu", pClient.get(), removed_name.c_str(), pInstanceId);
	return nullptr;
}

const char* evtc_rpc_server::HandleCombatEvent(const cbtevent& pEvent, std::shared_ptr<ConnectionContext>& pClient)
{
	uint16_t instanceId = 0;
	std::vector<std::shared_ptr<ConnectionContext>> peers; 
	{
		std::lock_guard lock(mRegisteredAgentsLock);
		// Check this under lock, mRegisteredAgentsLock also guards the ReceivedAccountName flag
		if (pClient->Iterator == mRegisteredAgents.end())
		{
			LOG("(client %p) this connection is not registered yet", pClient.get());
			return "not registered yet";
		}

		instanceId = pClient->InstanceId;

		for (const auto& [peerName, peerId] : pClient->Peers)
		{
			auto iter = mRegisteredAgents.find(peerName);
			if (iter != mRegisteredAgents.end())
			{
				if (iter->second->InstanceId != peerId)
				{
					LOG("(client %p) peer %p has incorrect instance id (expected %hu, found %hu)", pClient.get(), iter->second.get(), iter->second->InstanceId, peerId);
					continue;
				}

				peers.emplace_back(std::shared_ptr<ConnectionContext>(iter->second));
			}
		}
	}

	// We know that all peers are registered at this point since we got them from the registered agents map
	for (const auto& peer : peers)
	{
		std::lock_guard lock(peer->WriteLock);

		evtc_rpc::messages::CombatEvent message;
		message.Event = pEvent;
		message.SenderInstanceId = instanceId;

		if (peer->WritePending == false)
		{
			SendEvent(message, new WriteEventCallData(std::shared_ptr<ConnectionContext>(peer)), peer);
		}
		else
		{
			peer->QueuedEvents.emplace_back(std::move(message));
			LOG("(client %p) Queued CombatEvent from %p", peer.get(), pClient.get());
		}
	}

	LOG("(client %p) Queued CombatEvents to %zu peers", pClient.get(), peers.size());

	return nullptr;
}

void evtc_rpc_server::SendEvent(const evtc_rpc::messages::CombatEvent& pEvent, WriteEventCallData* pCallData, const std::shared_ptr<ConnectionContext>& pClient)
{
	assert(pClient->WritePending == false);

	evtc_rpc::messages::Header header;
	header.MessageVersion = 1;
	header.MessageType = evtc_rpc::messages::Type::CombatEvent;

	char buffer[sizeof(header) + sizeof(pEvent)];
	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + sizeof(header), &pEvent, sizeof(pEvent));

	evtc_rpc::Message rpc_message;
	rpc_message.set_blob(buffer, sizeof(header) + sizeof(pEvent));
	pClient->Stream.Write(rpc_message, pCallData);

	pClient->WritePending = true;

	LOG("(client %p tag %p) Sending CombatEvent from %hu source %hu target %hu skill %u value %i", pClient.get(), pCallData, pEvent.SenderInstanceId, pEvent.Event.src_instid, pEvent.Event.dst_instid, pEvent.Event.skillid, pEvent.Event.value);
}

void evtc_rpc_server::ForceDisconnect(const char* pErrorMessage, const std::shared_ptr<ConnectionContext>& pClient)
{
	if (pClient->ForceDisconnected == true)
	{
		LOG("Ignoring ForceDisconnect since client is already disconnected");
		return;
	}

	bool removedFromTable = false;
	{
		std::lock_guard lock(mRegisteredAgentsLock);

		if (pClient->Iterator != mRegisteredAgents.end())
		{
			mRegisteredAgents.erase(pClient->Iterator);
			pClient->Iterator = mRegisteredAgents.end();
			removedFromTable = true;
		}
	}
	
	pClient->ForceDisconnected = true;

	if (mIsShutdown == true)
	{
		LOG("(client %p) force disconnected (removedFromTable=%s) - '%s'. Server is shutting down so not queueing a Finish",
			pClient.get(), BOOL_STR(removedFromTable), pErrorMessage);
		return;
	}

	DisconnectCallData* queuedData = new DisconnectCallData{std::shared_ptr(pClient)};
	pClient->Stream.Finish(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, pErrorMessage}, queuedData);
	
	LOG("(client %p tag %p) force disconnected (removedFromTable=%s) - '%s'", pClient.get(), queuedData, BOOL_STR(removedFromTable), pErrorMessage);
}