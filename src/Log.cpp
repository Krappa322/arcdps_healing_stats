#include "Log.h"
#include "Log.h"

#ifdef _WIN32
#include "arcdps_structs.h"
#include "Exports.h"
#endif

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <chrono>
#include <ctime>

void Log_::LogImplementation_(const char* pComponentName, const char* pFunctionName, const char* pFormatString, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, pFormatString);
	vsnprintf(buffer, sizeof(buffer), pFormatString, args);
	va_end(args);

	Log_::LOGGER->debug("{}|{}|{}", pComponentName, pFunctionName, buffer);
}

#ifdef _WIN32
void Log_::LogImplementationArc_(const char* pComponentName, const char* pFunctionName, const char* pFormatString, ...)
{
	if (GlobalObjects::ARC_E3 == nullptr)
	{
		return;
	}

	char timeBuffer[128];
	int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	int64_t seconds = milliseconds / 1000;
	milliseconds = milliseconds % 1000;
	int64_t writtenChars = std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", std::localtime(&seconds));
	assert(writtenChars >= 0);

	char buffer[4096];
	writtenChars = snprintf(buffer, sizeof(buffer) - 1, "%s.%lli|%s|%s|", timeBuffer, milliseconds, pComponentName, pFunctionName);
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

	GlobalObjects::ARC_E3(buffer);
}
#endif

void Log_::FlushLogFile()
{
	Log_::LOGGER->flush();
}

void Log_::Init(bool pRotateOnOpen, const char* pLogPath)
{
	if (Log_::LOGGER != nullptr)
	{
		LogD("Skipping logger initialization since logger is not nullptr");
		return;
	}

	Log_::LOGGER = spdlog::rotating_logger_mt<spdlog::async_factory>("arcdps_healing_stats", pLogPath, 128*1024*1024, 8, pRotateOnOpen);
	Log_::LOGGER->set_pattern("%b %d %H:%M:%S.%f %t %L %v");
	spdlog::flush_every(std::chrono::seconds(5));
}

static std::atomic_bool LoggerLocked = false;
void Log_::SetLevel(spdlog::level::level_enum pLevel)
{
	if (pLevel < 0 || pLevel >= spdlog::level::n_levels)
	{
		LogW("Not setting level to {} since level is out of range", pLevel);
		return;
	}
	if (LoggerLocked == true)
	{
		LogW("Not setting level to {} since logger is locked", pLevel);
		return;
	}

	Log_::LOGGER->set_level(pLevel);
	LogI("Changed level to {}", pLevel);
}

void Log_::LockLogger()
{
	LoggerLocked = true;
	LogI("Locked logger");
}
