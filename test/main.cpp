#include "Exports.h"
#include "Log.h"

#pragma warning(push, 0)
#include <gtest/gtest.h>
#pragma warning(pop)

extern "C" __declspec(dllexport) void e3(const char* pString);
extern "C" __declspec(dllexport) uint64_t e6();
extern "C" __declspec(dllexport) uint64_t e7();
extern "C" __declspec(dllexport) void e9(cbtevent* pEvent, uint32_t pSignature);

#pragma pack(push, 1)
namespace
{
struct ArcModifiers
{
	uint16_t _1 = VK_SHIFT;
	uint16_t _2 = VK_MENU;
	uint16_t Multi = 0;
	uint16_t Fill = 0;
};
} // anonymous namespace
#pragma pack(pop)

void e3(const char* /*pString*/)
{
	return; // Logging, ignored
}

uint64_t e6()
{
	return 0; // everything set to false
}

uint64_t e7()
{
	ArcModifiers mods;
	return *reinterpret_cast<uint64_t*>(&mods);
}

void e9(cbtevent*, uint32_t)
{
	return; // Ignore, can be overridden by specific test if need be
}

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
	GlobalObjects::IS_UNIT_TEST = true;

	Log_::Init(true, "logs/unit_tests.txt");
	Log_::SetLevel(spdlog::level::debug);
	Log_::LockLogger();

	::testing::InitGoogleTest(&pArgumentCount, pArgumentVector); 

	testing::UnitTest::GetInstance()->listeners().Append(new TestLogFlusher);

	int result = RUN_ALL_TESTS();

	Log_::LOGGER = nullptr;
	spdlog::shutdown();

	return result;
}
