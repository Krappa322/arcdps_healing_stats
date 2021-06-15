#include "Log.h"

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

class TestLogFlusher : public testing::EmptyTestEventListener
{
	void OnTestStart(const ::testing::TestInfo& pTestInfo) override
	{
		LogI("Starting test {}.{}.{}", pTestInfo.test_suite_name(), pTestInfo.test_case_name(), pTestInfo.name());
	}

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

	testing::UnitTest::GetInstance()->listeners().Append(new TestLogFlusher);

	return RUN_ALL_TESTS();
}
