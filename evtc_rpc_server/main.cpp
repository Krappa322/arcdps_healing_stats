#include "../networking/Server.h"
#include "../src/Log.h"

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::InitMultiSink(false, "logs/evtc_rpc_server_debug.txt", "logs/evtc_rpc_server_info.txt");
	Log_::SetLevel(spdlog::level::debug);

	if (pArgumentCount != 2)
	{
		fprintf(stderr, "Invalid argument count\nusage: %s <listening endpoint>\n", pArgumentVector[0]);
		return 1;
	}

	evtc_rpc_server Server{pArgumentVector[1], nullptr};
	Server.Serve();
	return 0;
}