#include "Log.h"

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

class TestLogFlusher : public testing::EmptyTestEventListener
{
	// Called after a failed assertion or a SUCCESS().
	void OnTestPartResult(const testing::TestPartResult& /*pTestInfo*/) override
	{
		Log_::FlushLogFile();
	}

	// Called after a test ends.
	void OnTestEnd(const testing::TestInfo& /*pTestInfo*/) override
	{
		Log_::FlushLogFile();
	}
};

int main(int pArgumentCount, char** pArgumentVector)
{
	Log_::Init(true, "logs/unit_tests.txt");
	Log_::SetLevel(spdlog::level::debug);
	Log_::LockLogger();

	::testing::InitGoogleTest(&pArgumentCount, pArgumentVector); 
	return RUN_ALL_TESTS();
}
