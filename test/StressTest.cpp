#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

#include "Exports.h"
#include "Log.h"

#include "spdlog/stopwatch.h"

#include <numeric>

TEST(Stress, Stress)
{
	constexpr static size_t CLIENT_COUNT = 10;
	constexpr static size_t SENDER_COUNT = 10;
	constexpr static uint64_t EVENT_COUNT = 100'000;
	constexpr static uint64_t REPORT_INTERVAL = 10'000;

	extern const char* LoadRootCertificatesFromResource();
	const char* res = LoadRootCertificatesFromResource();
	ASSERT_EQ(res, nullptr);

	std::atomic_bool failed = false;

	std::array<std::unique_ptr<evtc_rpc_client>, CLIENT_COUNT> clients;
	std::array<std::unique_ptr<std::thread>, CLIENT_COUNT> client_threads;
	std::array<std::unique_ptr<std::thread>, SENDER_COUNT> sender_threads;

	std::array<std::array<size_t, SENDER_COUNT>, CLIENT_COUNT> next_event = {};

	auto getEndpoint = []() -> std::string
	{
		return std::string{"dev-evtc-rpc.kappa322.com:443"};
	};
	auto getCertificates = []() -> std::string
	{
		return std::string{GlobalObjects::ROOT_CERTIFICATES};
	}; 

	for (size_t i = 0; i < CLIENT_COUNT; i++)
	{
		auto eventHandler = [&next_event, &failed, i](cbtevent* pEvent, uint16_t /*pInstanceId*/)
		{
			size_t sender_num;
			memcpy(&sender_num, pEvent, sizeof(sender_num));
			size_t event_num;
			memcpy(&event_num, reinterpret_cast<const byte*>(pEvent) + sizeof(sender_num), sizeof(event_num));

			if (sender_num > next_event[i].size())
			{
				LogE("Client {} got event from sender {} which is invalid", i, sender_num);
				failed = true;
				return;
			}

			if (event_num != next_event[i][sender_num])
			{
				LogE("Client {} got event {} from {} when it was expecting {}", i, event_num, sender_num, next_event[i][sender_num]);
				failed = true;
			}

			next_event[i][sender_num] += 1;
		};

		clients[i] = std::make_unique<evtc_rpc_client>(std::function{getEndpoint}, std::function{getCertificates}, std::move(eventHandler));
		client_threads[i] = std::make_unique<std::thread>(evtc_rpc_client::ThreadStartServe, clients[i].get());
		for (size_t j = 0; j < CLIENT_COUNT; j++)
		{
			char nameBuffer[128];
			snprintf(nameBuffer, sizeof(nameBuffer), "testagent%zu.1234", j);

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

	for (size_t i = 0; i < CLIENT_COUNT; i++)
	{
		// Make sure server receives the client registration before sending any events
		clients[i]->FlushEvents(0);
	}

	for (size_t i = 0; i < SENDER_COUNT; i++)
	{
		sender_threads[i] = std::make_unique<std::thread>([](size_t pSenderNum, evtc_rpc_client& pClient, size_t pEventCount, std::atomic_bool& pFailedFlag)
			{
				cbtevent ev;
				memset(&ev, 0x00, sizeof(ev));
				for (uint64_t i = 0; i < pEventCount; i++)
				{
					memcpy(&ev, &pSenderNum, sizeof(pSenderNum));
					memcpy(reinterpret_cast<byte*>(&ev) + sizeof(pSenderNum), &i, sizeof(i));

					pClient.FlushEvents(4999);
					pClient.ProcessLocalEvent(&ev, nullptr, nullptr, nullptr, 0, 0);

					if (i % REPORT_INTERVAL == 0)
					{
						printf("%zu: loop %llu\n", pSenderNum, i);
						if (pFailedFlag.load() == true)
						{
							printf("%zu: Flagged as failed, stopping\n", pSenderNum);
							break;
						}
					}
				}
				pClient.FlushEvents(0);
			}, i, std::ref(*clients[i]), EVENT_COUNT, std::ref(failed));
	}
	for (size_t i = 0; i < SENDER_COUNT; i++)
	{
		sender_threads[i]->join();
	}

	spdlog::stopwatch complete_timer;

	bool completed = false;
	std::chrono::steady_clock::time_point last_received_event = std::chrono::steady_clock::now();
	std::array<uint64_t, SENDER_COUNT> lastCount = {};
	while (failed.load() == false && last_received_event + std::chrono::milliseconds(1000) > std::chrono::steady_clock::now())
	{
		for (size_t sender_index = 0; sender_index < SENDER_COUNT; sender_index++)
		{
			uint64_t count = 0;
			for (size_t client_index = 0; client_index < CLIENT_COUNT; client_index++)
			{
				count += next_event[client_index][sender_index];
			}

			EXPECT_GE(count, lastCount[sender_index]);
			if (count > lastCount[sender_index])
			{
				last_received_event = std::chrono::steady_clock::now();
				if ((count / REPORT_INTERVAL) > (lastCount[sender_index] / REPORT_INTERVAL))
				{
					printf("%llu: Received %llu events\n", sender_index, (count / REPORT_INTERVAL) * REPORT_INTERVAL);
				}
				lastCount[sender_index] = count;
			}
		}

		uint64_t totalCount = 0;
		totalCount = std::accumulate(lastCount.begin(), lastCount.end(), 0ULL);

		if (totalCount >= SENDER_COUNT * EVENT_COUNT * (CLIENT_COUNT - 1))
		{
			completed = true;
			break;
		}
		
		Sleep(1);
	}

	LogI("Awaiting client finish took {}", complete_timer);
	EXPECT_TRUE(completed);
	EXPECT_FALSE(failed.load());

	for (size_t i = 0; i < CLIENT_COUNT; i++)
	{
		clients[i]->Shutdown();
	}
	for (size_t i = 0; i < CLIENT_COUNT; i++)
	{
		client_threads[i]->join();
	}

	for (size_t sender_index = 0; sender_index < SENDER_COUNT; sender_index++)
	{
		for (size_t client_index = 0; client_index < CLIENT_COUNT; client_index++)
		{
			if (client_index == sender_index)
			{
				// Sender shouldn't send events to itself
				EXPECT_EQ(next_event[client_index][sender_index], 0);
			}
			else
			{
				if (next_event[client_index][sender_index] != EVENT_COUNT)
				{
					LogE("Client {} only received {} events from {} when it was expecting {}",
						client_index, next_event[client_index][sender_index], sender_index, EVENT_COUNT);
					EXPECT_EQ(next_event[client_index][sender_index], EVENT_COUNT); // Just log with values
				}
			}
		}
	}
}