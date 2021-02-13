#pragma once

#include <stdio.h>

#ifdef DEBUG
// Add optimized out call to printf to get a compiler warning for format string errors
#define LOG(pFormatString, ...) LogImplementation_(__func__, pFormatString, __VA_ARGS__); if (false) { printf(pFormatString, __VA_ARGS__); }
#else
#define LOG(pFormatString, ...)
#endif

#define LOG_ARC(pFormatString, ...) LogImplementationArc_(__func__, pFormatString, __VA_ARGS__); if (false) { printf(pFormatString, __VA_ARGS__); }


void LogImplementation_(const char* pFunctionName, const char* pFormatString, ...);
void LogImplementationArc_(const char* pFunctionName, const char* pFormatString, ...);

#define BOOL_STR(pBool) pBool == true ? "true" : "false"
