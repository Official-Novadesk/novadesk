#include "Logging.h"
#include <cstdio>
#include <cstdarg>

void Logging::Log(LogLevel level, const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wchar_t buffer[1024];
    vswprintf_s(buffer, format, args);
    va_end(args);

    const wchar_t* levelStr = L"";
    switch (level)
    {
    case LogLevel::Info:  levelStr = L"[LOG]"; break;
    case LogLevel::Error: levelStr = L"[ERROR]"; break;
    case LogLevel::Debug: levelStr = L"[DEBUG]"; break;
    }

    wchar_t output[1200];
    swprintf_s(output, L"[Novadesk] %s %s\n", levelStr, buffer);
    OutputDebugStringW(output);
}
