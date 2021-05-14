#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

int main(int pArgumentCount, char** pArgumentVector)
{
	::testing::InitGoogleTest(&pArgumentCount, pArgumentVector); 
	return RUN_ALL_TESTS();
}
