#pragma once
#include "Log.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <Windows.h>

#include <algorithm>
#include <array>
#include <array>
#include <string>
#include <optional>
#include <variant>

template<typename EnumType, size_t Size = static_cast<size_t>(EnumType::Max)>
class EnumStringArray : public std::array<const char*, Size>
{
public:
	template <typename... Args,
		typename = std::enable_if_t<(std::is_same_v<Args, const char*> && ...)>>
	constexpr EnumStringArray(Args... pArgs)
		: std::array<const char*, Size>{pArgs...}
	{
		static_assert(sizeof...(Args) == Size, "Incorrect array size");
	}

	using std::array<const char*, Size>::operator[];

	constexpr const char* operator[](EnumType pIndex) const
	{
		return std::array<const char*, Size>::operator[](static_cast<size_t>(pIndex));
	}
};

uint64_t constexpr divide_rounded_safe(uint64_t pDividend, uint64_t pDivisor)
{
	if (pDivisor == 0)
	{
		return 0;
	}

	return (pDividend + (pDivisor / 2)) / pDivisor;
}

template <typename T, typename U>
double constexpr divide_safe(T pDividend, U pDivisor)
{
	if (pDivisor == 0)
	{
		return 0;
	}

	return static_cast<double>(pDividend) / static_cast<double>(pDivisor);
}

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
	return static_cast<size_t>(charCount) - 1;
}

static inline std::string VirtualKeyToString(int pVirtualKey)
{
	char buffer[1024];

	uint32_t scanCode = MapVirtualKeyA(pVirtualKey, MAPVK_VK_TO_VSC);

	switch (pVirtualKey)
	{
	case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
	case VK_RCONTROL: case VK_RMENU:
	case VK_LWIN: case VK_RWIN: case VK_APPS:
	case VK_PRIOR: case VK_NEXT:
	case VK_END: case VK_HOME:
	case VK_INSERT: case VK_DELETE:
	case VK_DIVIDE:
	case VK_NUMLOCK:
		scanCode |= KF_EXTENDED;
		[[fallthrough]];
	default:
		int result = GetKeyNameTextA(scanCode << 16, buffer, sizeof(buffer));
		if (result == 0)
		{
			//LOG("Translating key %i failed - error %u", pVirtualKey, GetLastError());
			return std::string{};
		}
		break;
	}

	return std::string{buffer};
}

// Prints pNumber to pResultBuffer with magnitude suffix if necessary
// Returns output with the same rules as snprintf
template<typename NumberType>
static inline int snprint_magnitude(char* pResultBuffer, size_t pResultBufferLength, NumberType pNumber)
{
	if (pNumber < 10000)
	{
		if constexpr (std::is_same<NumberType, uint64_t>::value == true)
		{
			return snprintf(pResultBuffer, pResultBufferLength, "%llu", pNumber);
		}
		else if constexpr (std::is_same<NumberType, double>::value == true)
		{
			return snprintf(pResultBuffer, pResultBufferLength, "%.1f", pNumber);
		}
		else
		{
			static_assert(false, "Bad type");
		}
	}

	static constexpr char bases[] = {'k', 'M', 'G', 'T', 'P', 'E'};

	int magnitude = static_cast<int>(log(pNumber) / log(1000));
	return snprintf(pResultBuffer, pResultBufferLength, "%.1f%c", pNumber / pow(1000, magnitude), bases[magnitude - 1]);
}

// Replaces "{1}", "{2}", etc. with pArgs[0], pArgs[1], etc. If that argument is nullopt, the entry is not replaced.
// Returns the amount of bytes written
template<int ArgCount>
static inline size_t ReplaceFormatted(char* pResultBuffer, size_t pResultBufferLength, const char* pFormatString, std::array<std::optional<std::variant<uint64_t, double>>, ArgCount> pArgs)
{
	char* startBuffer = pResultBuffer;
	assert(pFormatString != nullptr);

	for (const char* curChar = pFormatString; *curChar != '\0'; curChar++)
	{
		if (*curChar == '{')
		{
			const char* key = curChar + 1;
			const char* closeBrace = curChar + 2;
			if (*key >= '1' && *key <= '9' && *closeBrace == '}')
			{
				uint32_t num = *key - '1';

				if (pArgs.size() > num && pArgs[num].has_value() == true)
				{
					int count;
					if (std::holds_alternative<uint64_t>(*pArgs[num]) == true)
					{
						count = snprint_magnitude(pResultBuffer, pResultBufferLength, std::get<uint64_t>(*pArgs[num]));
					}
					else
					{
						assert(std::holds_alternative<double>(*pArgs[num]) == true);
						count = snprint_magnitude(pResultBuffer, pResultBufferLength, std::get<double>(*pArgs[num]));
					}

					if (count >= pResultBufferLength)
					{
						// Value was truncated. Break the loop, which will insert a null character at the start of the
						// printed value (thus not showing the truncated value)
						LOG("Truncated value for format string %s at pos %llu - would require %i bytes but only %zu are available", pFormatString, curChar - pFormatString, count, pResultBufferLength);
						break;
					}

					curChar += 2; // We've processed the braces as well
					pResultBuffer += count;
					pResultBufferLength -= count;
					continue;
				}
			}
		}

		if (pResultBufferLength == 1)
		{
			// Not enough space in buffer
			break;
		}

		*pResultBuffer = *curChar;
		pResultBufferLength--;
		pResultBuffer++;
	}

	assert(pResultBufferLength > 0);
	*pResultBuffer = '\0';

	return pResultBuffer - startBuffer;
}