#include <gtest/gtest.h>

int main(int pArgumentCount, char** pArgumentVector)
{
	::testing::InitGoogleTest(&pArgumentCount, pArgumentVector); 
	return RUN_ALL_TESTS();
}
