#include "Log.h"

#include "ArcDPS.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>

#include <chrono>
#include <ctime>

void LogImplementation_(const char* pFunctionName, const char* pFormatString, ...)
{
	char timeBuffer[128];
	int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	int64_t seconds = milliseconds / 1000;
	milliseconds = milliseconds - seconds * 1000;
	int64_t writtenChars = std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", std::localtime(&seconds));
	assert(writtenChars >= 0);

	char buffer[4096];
	writtenChars = snprintf(buffer, sizeof(buffer) - 1, "%s.%lli|%s|", timeBuffer, milliseconds, pFunctionName);
	assert(writtenChars >= 0);
	assert(writtenChars < sizeof(buffer) - 1);

	va_list args;
	va_start(args, pFormatString);

	int writtenChars2 = vsnprintf(buffer + writtenChars, sizeof(buffer) - writtenChars - 1, pFormatString, args);
	assert(writtenChars2 >= 0);
	assert(writtenChars2 < (sizeof(buffer) - writtenChars - 1));
	buffer[writtenChars + writtenChars2] = '\n';
	buffer[writtenChars + writtenChars2 + 1] = '\0';

	va_end(args);

	DWORD written;
	bool result = WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, static_cast<DWORD>(strlen(buffer)), &written, 0);
	assert(result == true);
}

void LogImplementationArc_(const char* pFunctionName, const char* pFormatString, ...)
{
	if (ARC_E3 == nullptr)
	{
		return;
	}

	char timeBuffer[128];
	int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	int64_t seconds = milliseconds / 1000;
	milliseconds = milliseconds - seconds * 1000;
	int64_t writtenChars = std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", std::localtime(&seconds));
	assert(writtenChars >= 0);

	char buffer[4096];
	writtenChars = snprintf(buffer, sizeof(buffer) - 1, "%s.%lli|%s|", timeBuffer, milliseconds, pFunctionName);
	assert(writtenChars >= 0);
	assert(writtenChars < sizeof(buffer) - 1);

	va_list args;
	va_start(args, pFormatString);

	int writtenChars2 = vsnprintf(buffer + writtenChars, sizeof(buffer) - writtenChars - 1, pFormatString, args);
	assert(writtenChars2 >= 0);
	assert(writtenChars2 < (sizeof(buffer) - writtenChars - 1));
	buffer[writtenChars + writtenChars2] = '\n';
	buffer[writtenChars + writtenChars2 + 1] = '\0';

	va_end(args);

	DWORD written;
	ARC_E3(buffer);
}
