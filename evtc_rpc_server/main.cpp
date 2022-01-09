#include "../networking/Server.h"
#include "../src/Log.h"

#ifdef LINUX
#include <signal.h>
#endif

#include <memory>
#include <thread>

static std::unique_ptr<evtc_rpc_server> SERVER;
static std::thread SERVER_THREAD;

void signal_handler_shutdown(int pSignal)
{
	LogI("Signal {}", pSignal);
	SERVER->Shutdown();
}

void install_signal_handler()
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

void uninstall_signal_handler()
{
#ifdef LINUX
	struct sigaction sa = {};
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
#endif
}

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::InitMultiSink(false, "logs/evtc_rpc_server_debug.txt", "logs/evtc_rpc_server_info.txt");
	Log_::SetLevel(spdlog::level::debug);

	if (pArgumentCount != 2)
	{
		fprintf(stderr, "Invalid argument count\nusage: %s <listening endpoint>\n", pArgumentVector[0]);
		return 1;
	}

	SERVER = std::make_unique<evtc_rpc_server>(pArgumentVector[1], nullptr);
	SERVER_THREAD = std::thread(evtc_rpc_server::ThreadStartServe, SERVER.get());
	install_signal_handler();
	
	SERVER_THREAD.join();
	uninstall_signal_handler();
	SERVER = nullptr;

	LogI("Exited normally");
	Log_::Shutdown();

	return 0;
}