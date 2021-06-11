#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "Exports.h"
#include "Log.h"

TEST(stress, DISABLED_server_stress)
{
	const size_t client_count = 10;
	const size_t event_count = 100'000;

	extern const char* LoadRootCertificatesFromResource();
	const char* res = LoadRootCertificatesFromResource();
	ASSERT_EQ(res, nullptr);

	std::atomic_bool failed = false;
	std::unique_ptr<evtc_rpc_client> clients[client_count];
	std::unique_ptr<std::thread> client_threads[client_count];
	uint32_t next_event[client_count] = {};

	auto getEndpoint = []() -> std::string
	{
		return std::string{"dev-evtc-rpc.kappa322.com:443"};
	};
	auto getCertificates = []() -> std::string
	{
		return std::string{GlobalObjects::ROOT_CERTIFICATES};
	}; 

	for (uint32_t i = 0; i < client_count; i++)
	{
		auto eventHandler = [&next_event, &failed, i](cbtevent* pEvent, uint16_t /*pInstanceId*/)
		{
			uint32_t event_num;
			memcpy(&event_num, pEvent, sizeof(event_num));
			if (event_num != next_event[i])
			{
				LogW("Client {} got event {} when it was expecting {}", i, event_num, next_event[i]);
				failed = true;
			}

			next_event[i] += 1;
		};

		clients[i] = std::make_unique<evtc_rpc_client>(std::function{getEndpoint}, std::function{getCertificates}, std::move(eventHandler));
		client_threads[i] = std::make_unique<std::thread>(evtc_rpc_client::ThreadStartServe, clients[i].get());
		for (uint32_t j = 0; j < client_count; j++)
		{
			char nameBuffer[128];
			snprintf(nameBuffer, sizeof(nameBuffer), "testagent%u.1234", j);

			ag ag1{};
			ag ag2{};
			ag1.elite = 0;
			ag1.prof = static_cast<Prof>(1);
			ag2.id = 100ULL + j;
			ag2.name = nameBuffer;

			if (i == j)
			{
				ag2.self = 1;
				clients[i]->ProcessLocalEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);
			}
			else
			{
				ag2.self = 0;
				clients[i]->ProcessAreaEvent(nullptr, &ag1, &ag2, nullptr, 0, 0);
			}
		}
	}

	cbtevent ev;
	memset(&ev, 0x00, sizeof(ev));
	for (uint32_t i = 0; i < event_count; i++)
	{
		memcpy(&ev, &i, sizeof(i));

		clients[0]->FlushEvents(4999);
		clients[0]->ProcessLocalEvent(&ev, nullptr, nullptr, nullptr, 0, 0);

		if (i % 10000 == 0)
		{
			printf("loop %u\n", i);
		}
	}

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	bool completed = false;
	while (failed.load() == false && (std::chrono::system_clock::now() - start) < std::chrono::milliseconds(30000))
	{
		uint64_t count = 0;
		for (uint32_t i = 1; i < client_count; i++)
		{
			count += next_event[i]; // not atomic, but whatever :)
		}
		if (count == event_count * (client_count - 1))
		{
			completed = true;
			break;
		}
		Sleep(1);
	}
	LogI("Awaiting client finish took {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count());
	EXPECT_TRUE(completed);

	for (uint32_t i = 0; i < client_count; i++)
	{
		clients[i]->Shutdown();
	}
	for (uint32_t i = 0; i < client_count; i++)
	{
		client_threads[i]->join();
	}
	EXPECT_EQ(next_event[0], 0);
	for (uint32_t i = 1; i < client_count; i++)
	{
		if (next_event[i] != event_count)
		{
			LogW("Client {} only received {} events when it was expecting {}", i, next_event[i], event_count);
			EXPECT_EQ(next_event[i], event_count); // Just log with values
		}
	}
	EXPECT_FALSE(failed.load());
}