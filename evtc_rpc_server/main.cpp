#include "../networking/Server.h"

int main(int pArgumentCount, char** pArgumentVector)
{
	if (pArgumentCount != 2)
	{
		fprintf(stderr, "Invalid argument count\nusage: %s <listening endpoint>", pArgumentVector[0]);
		return 1;
	}

	evtc_rpc_server Server{pArgumentVector[1], nullptr};
	Server.Serve();
	return 0;
}