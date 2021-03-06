#pragma once
#define SPDLOG_COMPILED_LIB
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <stdio.h>

namespace Log_
{
	static constexpr const char* StripPath(const char* pPath)
	{
		const char* lastname = pPath;

		for (const char* p = pPath; *p != '\0'; p++)
		{
			if ((*p == '/' || *p == '\\') && (*(p + 1) != '\0'))
			{
				lastname = p + 1;
			}
		}

		return lastname;
	}

	static constexpr const char* FindEnd(const char* pPath)
	{
		const char* lastpoint = pPath;

		while (*lastpoint != '\0')
		{
			lastpoint++;
			if (*lastpoint == '.')
			{
				break;
			}
		}

		return lastpoint;
	}

	struct FileNameStruct
	{
		char name[1024];

		constexpr operator char* ()
		{
			return name;
		}
	};

	static constexpr FileNameStruct GetFileName(const char* pFilePath)
	{
		FileNameStruct fileName = {};
		const char* strippedPath = StripPath(pFilePath);
		const char* pointPosition = FindEnd(strippedPath);

		for (unsigned int i = 0; i < pointPosition - strippedPath; i++)
		{
			*(fileName + i) = *(strippedPath + i);
		}

		fileName[pointPosition - strippedPath] = '\0';
		return fileName;
	}

	void LogImplementation_(const char* pComponentName, const char* pFunctionName, const char* pFormatString, ...);
	void LogImplementationArc_(const char* pComponentName, const char* pFunctionName, const char* pFormatString, ...);

	void FlushLogFile();
	void Init(bool pRotateOnOpen, const char* pLogPath);
	void SetLevel(spdlog::level::level_enum pLevel);
	void LockLogger();

	inline std::shared_ptr<spdlog::logger> LOGGER;
}

#define LogD(pFormatString, ...) Log_::LOGGER->debug(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogI(pFormatString, ...) Log_::LOGGER->info(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogW(pFormatString, ...) Log_::LOGGER->warn(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)

#define LOG(pFormatString, ...) Log_::LogImplementation_(Log_::GetFileName(__FILE__), __func__, pFormatString, ##__VA_ARGS__); if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define DEBUGLOG(pFormatString, ...) if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define LOG_ARC(pFormatString, ...) Log_::LogImplementationArc_(Log_::GetFileName(__FILE__), __func__, pFormatString, ##__VA_ARGS__); if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define BOOL_STR(pBool) pBool == true ? "true" : "false"
