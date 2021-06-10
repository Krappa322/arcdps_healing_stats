#include "Log.h"

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::Init(true, "logs/unit_tests.txt");
	Log_::SetLevel(spdlog::level::debug);
	::testing::InitGoogleTest(&pArgumentCount, pArgumentVector); 
	return RUN_ALL_TESTS();
}
