#pragma once
#define SPDLOG_COMPILED_LIB
#define SPDLOG_FMT_EXTERNAL

#pragma warning(push, 0)
#pragma warning(disable : 4189)
#pragma warning(disable : 6285)
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#pragma warning(pop)

#include <stdio.h>

struct SimpleFormatter
{
	constexpr auto parse(fmt::format_parse_context& pContext) -> decltype(pContext.begin())
	{
		// [ctx.begin(), ctx.end()) is a character range that contains a part of
		// the format string starting from the format specifications to be parsed,
		// e.g. in
		//
		//   fmt::format("{:f} - point of interest", point{1, 2});
		//
		// the range will contain "f} - point of interest". The formatter should
		// parse specifiers until '}' or the end of the range. In this example
		// the formatter should parse the 'f' specifier and return an iterator
		// pointing to '}'.

		// Parse the presentation format and store it in the formatter:
		auto it = pContext.begin(), end = pContext.end();

		// Check if reached the end of the range:
		if (it != end && *it != '}')
		{
			throw fmt::format_error("invalid format");
		}

		// Return an iterator past the end of the parsed range:
		return it;
	}
};

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

	void FlushLogFile();
	void Init(bool pRotateOnOpen, const char* pLogPath);
	void InitMultiSink(bool pRotateOnOpen, const char* pLogPathTrace, const char* pLogPathInfo);
	void Shutdown();
	void SetLevel(spdlog::level::level_enum pLevel);
	void LockLogger();

	inline std::shared_ptr<spdlog::logger> LOGGER;
}

#define LogT(pFormatString, ...) Log_::LOGGER->trace(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogD(pFormatString, ...) Log_::LOGGER->debug(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogI(pFormatString, ...) Log_::LOGGER->info(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogW(pFormatString, ...) Log_::LOGGER->warn(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogE(pFormatString, ...) Log_::LOGGER->error(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)
#define LogC(pFormatString, ...) Log_::LOGGER->critical(FMT_STRING("{}|{}|" pFormatString), Log_::GetFileName(__FILE__).name, __func__, ##__VA_ARGS__)

#define LOG(pFormatString, ...) Log_::LogImplementation_(Log_::GetFileName(__FILE__), __func__, pFormatString, ##__VA_ARGS__); if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define DEBUGLOG(pFormatString, ...) if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define BOOL_STR(pBool) pBool == true ? "true" : "false"
