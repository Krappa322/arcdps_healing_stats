#pragma once

#ifdef DEBUG
// Add optimized out call to printf to get a compiler warning for format string errors
#define LOG(pFormatString, ...) LogImplementation_(__func__, pFormatString, __VA_ARGS__); if (false) { printf(pFormatString, __VA_ARGS__); }
#else
#define LOG(pFormatString, ...)
#endif

void LogImplementation_(const char* pFunctionName, const char* pFormatString, ...);

#define BOOL_STR(pBool) pBool == true ? "true" : "false"
