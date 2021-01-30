#pragma once

size_t constexpr constexpr_strlen(const char* pString)
{
	size_t result = 0;
	for (const char* pChar = pString; *pChar != '\0'; pChar++)
	{
		result++;
	}

	return result;
}

// Overly complicated solution so the compiler errors aren't too readable
template <const char* const* Array, size_t i>
struct LongestStringInArray
{
	constexpr const static uint64_t value = std::max(constexpr_strlen(Array[i]), LongestStringInArray<Array, i - 1>::value);
};

template<const char* const* Array>
struct LongestStringInArray<Array, 0>
{
	constexpr const static uint64_t value = constexpr_strlen(Array[0]);
};