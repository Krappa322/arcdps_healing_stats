#pragma once

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
}

#ifdef DEBUG
// Add optimized out call to printf to get a compiler warning for format string errors
#define LOG(pFormatString, ...) Log_::LogImplementation_(Log_::GetFileName(__FILE__), __func__, pFormatString, ##__VA_ARGS__); if (false) { printf(pFormatString, ##__VA_ARGS__); }
#else
#define LOG(pFormatString, ...)
#endif
#define DEBUGLOG(...)

#define LOG_ARC(pFormatString, ...) Log_::LogImplementationArc_(Log_::GetFileName(__FILE__), __func__, pFormatString, ##__VA_ARGS__); if (false) { printf(pFormatString, ##__VA_ARGS__); }

#define BOOL_STR(pBool) pBool == true ? "true" : "false"
