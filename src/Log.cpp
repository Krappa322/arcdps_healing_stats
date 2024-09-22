#include "Log.h"

#include <absl/log/absl_log.h>
#include <absl/log/log_sink_registry.h>
#include <absl/log/globals.h>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef LINUX
#include <pthread.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif

#include <chrono>
#include <ctime>

class AbslLogRedirector final : public absl::LogSink
{
public:
	AbslLogRedirector() {}
	~AbslLogRedirector() {}

	void Send(const absl::LogEntry& entry) override
	{
		spdlog::level::level_enum level = spdlog::level::info;
		switch (entry.log_severity())
		{
		case absl::LogSeverity::kInfo:
			level = spdlog::level::info;
			break;
		case absl::LogSeverity::kWarning:
			level = spdlog::level::warn;
			break;
		case absl::LogSeverity::kError:
			level = spdlog::level::err;
			break;
		case absl::LogSeverity::kFatal:
			level = spdlog::level::critical;
			break;
		};

		return Log_::LOGGER->log(level, FMT_STRING("ABSL|{}:{}|{}"),
			static_cast<std::string>(entry.source_basename()), entry.source_line(), static_cast<std::string>(entry.text_message()));
	}
};

namespace
{
void SetThreadNameLogThread()
{
#ifdef LINUX
	pthread_setname_np(pthread_self(), "spdlog-worker");
#elif defined(_WIN32)
	SetThreadDescription(GetCurrentThread(), L"spdlog-worker");
#endif
}
std::shared_ptr<AbslLogRedirector> ABSL_LOGGER;
};

void Log_::LogImplementation_(const char* pComponentName, const char* pFunctionName, const char* pFormatString, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, pFormatString);
	vsnprintf(buffer, sizeof(buffer), pFormatString, args);
	va_end(args);

	Log_::LOGGER->debug("{}|{}|{}", pComponentName, pFunctionName, buffer);
}

void Log_::FlushLogFile()
{
	Log_::LOGGER->flush();
}

void Log_::Init(bool pRotateOnOpen, const char* pLogPath)
{
	if (Log_::LOGGER != nullptr)
	{
		LogW("Skipping logger initialization since logger is not nullptr");
		return;
	}

	Log_::LOGGER = spdlog::rotating_logger_mt<spdlog::async_factory>("arcdps_healing_stats", pLogPath, 128*1024*1024, 8, pRotateOnOpen);
	Log_::LOGGER->set_pattern("%b %d %H:%M:%S.%f %t %L %v");
	Log_::LOGGER->flush_on(spdlog::level::err);
	spdlog::flush_every(std::chrono::seconds(5));

	ABSL_LOGGER = std::make_shared<AbslLogRedirector>();
	absl::AddLogSink(ABSL_LOGGER.get());
}

// SetLevel can still be used after calling this, the sink levels and logger levels are different things - e.g if logger
// level is debug and sink level is trace then trace lines will not be shown
void Log_::InitMultiSink(bool pRotateOnOpen, const char* pLogPathTrace, const char* pLogPathInfo)
{
	if (Log_::LOGGER != nullptr)
	{
		LogW("Skipping logger initialization since logger is not nullptr");
		return;
	}

	spdlog::init_thread_pool(8192, 1, &SetThreadNameLogThread);

	auto debug_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(pLogPathTrace, 128*1024*1024, 8, pRotateOnOpen);
	debug_sink->set_level(spdlog::level::trace);
	auto info_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(pLogPathInfo, 128*1024*1024, 8, pRotateOnOpen);
	info_sink->set_level(spdlog::level::info);

	std::vector<spdlog::sink_ptr> sinks{debug_sink, info_sink};
	Log_::LOGGER = std::make_shared<spdlog::async_logger>("arcdps_healing_stats", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	Log_::LOGGER->set_pattern("%b %d %H:%M:%S.%f %t %L %v");
	Log_::LOGGER->flush_on(spdlog::level::err);
	spdlog::register_logger(Log_::LOGGER);

	spdlog::flush_every(std::chrono::seconds(5));
	
	ABSL_LOGGER = std::make_shared<AbslLogRedirector>();
	absl::AddLogSink(ABSL_LOGGER.get());
}

void Log_::Shutdown()
{
	Log_::LOGGER = nullptr;
	spdlog::shutdown();
}

static std::atomic_bool LoggerLocked = false;
void Log_::SetLevel(spdlog::level::level_enum pLevel)
{
	if (pLevel < 0 || pLevel >= spdlog::level::n_levels)
	{
		LogW("Not setting level to {} since level is out of range", static_cast<int>(pLevel));
		return;
	}
	if (LoggerLocked == true)
	{
		LogW("Not setting level to {} since logger is locked", static_cast<int>(pLevel));
		return;
	}

	Log_::LOGGER->set_level(pLevel);
	LogI("Changed level to {}", static_cast<int>(pLevel));
}

void Log_::LockLogger()
{
	LoggerLocked = true;
	LogI("Locked logger");
}
