#include "Utilities.h"

#pragma warning(push, 0)
#pragma warning(disable : 4005)
#pragma warning(disable : 4389)
#pragma warning(disable : 26439)
#pragma warning(disable : 26495)
#include <gtest/gtest.h>
#pragma warning(pop)

TEST(UtilitiesTest, utf8_substr_ascii)
{
	const char* sourceDataConst = "Hello World!!!";
	char sourceDataBuf[14];	// Ensure there's no null termination
	memcpy(sourceDataBuf, sourceDataConst, 14);
	std::string_view sourceData = std::string_view{sourceDataBuf, 14};
	ASSERT_EQ(utf8_substr(sourceData, 0), "");
	ASSERT_EQ(utf8_substr(sourceData, 1), "H");
	ASSERT_EQ(utf8_substr(sourceData, 4), "Hell");
	ASSERT_EQ(utf8_substr(sourceData, 13), "Hello World!!");
	ASSERT_EQ(utf8_substr(sourceData, 14), "Hello World!!!");
	ASSERT_EQ(utf8_substr(sourceData, 15), "Hello World!!!");
	ASSERT_EQ(utf8_substr(sourceData, 100000000), "Hello World!!!");
}

TEST(UtilitiesTest, utf8_substr_unicode)
{
	const char* sourceDataConst = "ホヽのö Wó卂丂ᗪ";
	char sourceDataBuf[24];	// Ensure there's no null termination
	memcpy(sourceDataBuf, sourceDataConst, 24);
	std::string_view sourceData = std::string_view{sourceDataBuf, 24};
	ASSERT_EQ(utf8_substr(sourceData, 0), "");
	ASSERT_EQ(utf8_substr(sourceData, 1), "ホ");
	ASSERT_EQ(utf8_substr(sourceData, 4), "ホヽのö");
	ASSERT_EQ(utf8_substr(sourceData, 8), "ホヽのö Wó卂");
	ASSERT_EQ(utf8_substr(sourceData, 9), "ホヽのö Wó卂丂");
	ASSERT_EQ(utf8_substr(sourceData, 10), "ホヽのö Wó卂丂ᗪ");
	ASSERT_EQ(utf8_substr(sourceData, 11), "ホヽのö Wó卂丂ᗪ");
	ASSERT_EQ(utf8_substr(sourceData, 100000000), "ホヽのö Wó卂丂ᗪ");
}
