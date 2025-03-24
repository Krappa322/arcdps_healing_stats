#include "linux_versions_auto.h"

#include "../networking/Server.h"
#include "../src/Log.h"

#ifdef LINUX
#include <signal.h>
#include <jemalloc/jemalloc.h>
#endif

#include <memory>
#include <thread>

constexpr static auto MALLOC_STATS_INTERVAL = std::chrono::seconds(600);

static std::unique_ptr<evtc_rpc_server> SERVER;
static std::thread SERVER_THREAD;
static std::thread MONITOR_THREAD;

static std::atomic_bool MONITOR_THREAD_SHUTDOWN = false;

static void signal_handler_shutdown(int pSignal)
{
	LogI("Signal {}", pSignal);
	SERVER->Shutdown();
}

static void install_signal_handler()
{
#ifdef LINUX
	struct sigaction sa = {};
	sa.sa_handler = &signal_handler_shutdown;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
#endif
}

static void uninstall_signal_handler()
{
#ifdef LINUX
	struct sigaction sa = {};
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
#endif
}

static void MallocStats(void*, const char* pData)
{
	LogI("{}", pData);
}

static void monitor_thread_entry()
{
#ifdef LINUX
	pthread_setname_np(pthread_self(), "evtc-rpc-mon");
#elif defined(_WIN32)
	SetThreadDescription(GetCurrentThread(), L"evtc-rpc-mon");
#endif
	
	std::chrono::steady_clock::time_point last_report; // epoch
	while (MONITOR_THREAD_SHUTDOWN.load(std::memory_order_relaxed) == false)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (last_report + MALLOC_STATS_INTERVAL <= now)
		{
#ifdef LINUX
			malloc_stats_print(&MallocStats, NULL, NULL);
#endif
			last_report = now;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::InitMultiSink(false, "logs/evtc_rpc_server_debug.txt", "logs/evtc_rpc_server_info.txt");
	Log_::SetLevel(spdlog::level::debug);
	LogI("Start. Dependency versions:\n{}", DEPENDENCY_VERSIONS);

	if (pArgumentCount != 3)
	{
		fprintf(stderr, "Invalid argument count\nusage: %s <listening endpoint> <prometheus endpoint>\n", pArgumentVector[0]);
		return 1;
	}

	SERVER = std::make_unique<evtc_rpc_server>(pArgumentVector[1], pArgumentVector[2], nullptr);
	SERVER_THREAD = std::thread(evtc_rpc_server::ThreadStartServe, SERVER.get());
	MONITOR_THREAD = std::thread(monitor_thread_entry);

// Set thread name after this thread is done cloning itself for other threads
#ifdef LINUX
	pthread_setname_np(pthread_self(), "evtc-rpc-main");
#elif defined(_WIN32)
	SetThreadDescription(GetCurrentThread(), L"evtc-rpc-main");
#endif

	install_signal_handler();

	SERVER_THREAD.join();
	MONITOR_THREAD_SHUTDOWN.store(true, std::memory_order_relaxed);
	MONITOR_THREAD.join();

	uninstall_signal_handler();
	SERVER = nullptr;

	LogI("Exited normally");
	Log_::Shutdown();

	return 0;
}