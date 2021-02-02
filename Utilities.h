#pragma once

#include <assert.h>
#include <Windows.h>

#include <algorithm>

size_t constexpr constexpr_strlen(const char* pString)
{
	size_t result = 0;
	for (const char* curChar = pString; *curChar != '\0'; curChar++)
	{
		result++;
	}

	return result;
}

// Overly complicated solution so the compiler errors aren't too readable
template <const char* const* Array, size_t i>
struct LongestStringInArray
{
	constexpr const static uint64_t value = (std::max)(constexpr_strlen(Array[i]), LongestStringInArray<Array, i - 1>::value);
};

template<const char* const* Array>
struct LongestStringInArray<Array, 0>
{
	constexpr const static uint64_t value = constexpr_strlen(Array[0]);
};

// Returns number of characters (as opposed to number of bytes) in a utf8 string
static inline size_t utf8_strlen(const char* pString)
{
	// _mbstrlen is broken for utf8 so we use MultiByteToWideChar to get the character count (passing no destination
	// buffer makes it return the amount of space needed - our character count).
	int charCount = MultiByteToWideChar(CP_UTF8, 0, pString, -1, nullptr, 0);
	assert(charCount > 0);

	// charCount includes null character so remove 1
	return charCount - 1;
}