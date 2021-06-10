#include "../networking/Server.h"
#include "../src/Log.h"

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::Init(false, "logs/evtc_rpc_server.txt");
	Log_::SetLevel(spdlog::level::info);

	if (pArgumentCount != 2)
	{
		fprintf(stderr, "Invalid argument count\nusage: %s <listening endpoint>\n", pArgumentVector[0]);
		return 1;
	}

	evtc_rpc_server Server{pArgumentVector[1], nullptr};
	Server.Serve();
	return 0;
}