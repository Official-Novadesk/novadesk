#ifndef __NOVADESK_LOGGING_H__
#define __NOVADESK_LOGGING_H__

#include <windows.h>
#include <string>

enum class LogLevel
{
    Info,
    Error,
    Debug
};

class Logging
{
public:
    static void Log(LogLevel level, const wchar_t* format, ...);
};

#endif
