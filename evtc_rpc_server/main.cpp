#include "../networking/Server.h"

int main()
{
	evtc_rpc_server Server{"localhost:50052"};
	Server.Serve();
	return 0;
}