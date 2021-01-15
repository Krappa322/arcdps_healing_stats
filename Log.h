#pragma once

// Add optimized out call to printf to get a compiler warning for format string errors
#define LOG(pFormatString, ...) LogImplementation_(__func__, pFormatString, __VA_ARGS__); if (false) { printf(pFormatString, __VA_ARGS__); }

void LogImplementation_(const char* pFunctionName, const char* pFormatString, ...);