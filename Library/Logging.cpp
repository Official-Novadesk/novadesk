/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Logging.h"
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <ctime>
#include <chrono>
#include <vector>
#include "PathUtils.h"

// Static member initialization
bool Logging::s_ConsoleEnabled = true;
bool Logging::s_FileEnabled = false;
std::wstring Logging::s_LogFilePath = L"";
LogLevel Logging::s_MinLevel = LogLevel::Info;

/*
** Log a message to the debug output and/or file.
** Supports formatted output similar to printf.
** Messages are prefixed with [INFO], [ERROR], or [DEBUG] based on level.
*/

void Logging::Log(LogLevel level, const wchar_t* format, ...)
{
    // Check if this log level should be displayed
    if (level < s_MinLevel) return;
    if (!s_ConsoleEnabled && !s_FileEnabled) return;

    va_list args;
    va_start(args, format);
    
    // Determine required size
    int len = _vscwprintf(format, args);
    if (len < 0) {
        va_end(args);
        return;
    }

    std::vector<wchar_t> buffer(len + 1);
    vswprintf_s(buffer.data(), buffer.size(), format, args);
    va_end(args);

    const wchar_t* levelStr = L"";
    switch (level)
    {
    case LogLevel::Info:  levelStr = L"[LOG]"; break;
    case LogLevel::Warn:  levelStr = L"[WARN]"; break;
    case LogLevel::Error: levelStr = L"[ERROR]"; break;
    case LogLevel::Debug: levelStr = L"[DEBUG]"; break;
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    tm timeInfo;
    localtime_s(&timeInfo, &now_c);
    
    wchar_t timestamp[64];
    swprintf_s(timestamp, L"[%04d-%02d-%02d %02d:%02d:%02d.%03lld]",
        timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
        timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, ms.count());

    // Prepare final output string dynamically
    // format: timestamp + " [" + productName + "] " + levelStr + " " + buffer + "\n"
    std::wstring productName = PathUtils::GetProductName();
    int outputLen = _scwprintf(L"%s [%s] %s %s\n", timestamp, productName.c_str(), levelStr, buffer.data());
    
    if (outputLen < 0) return;

    std::vector<wchar_t> output(outputLen + 1);
    swprintf_s(output.data(), output.size(), L"%s [%s] %s %s\n", timestamp, productName.c_str(), levelStr, buffer.data());

    // Output to console (debug output)
    if (s_ConsoleEnabled)
    {
        OutputDebugStringW(output.data());
        wprintf(L"%s", output.data());
        fflush(stdout);
    }

    // Output to file
    if (s_FileEnabled && !s_LogFilePath.empty())
    {
        std::wofstream logFile(s_LogFilePath, std::ios::app);
        if (logFile.is_open())
        {
            logFile << output.data();
            logFile.close();
        }
    }
}

/*
** Enable or disable console (debug output) logging.
*/

void Logging::SetConsoleLogging(bool enable)
{
    s_ConsoleEnabled = enable;
}

/*
** Enable or disable file logging with an option to clear the file.
** If filePath is empty, file logging is disabled.
*/
void Logging::SetFileLogging(const std::wstring& filePath, bool clearFile)
{
    s_LogFilePath = filePath;
    s_FileEnabled = !filePath.empty();

    if (s_FileEnabled && clearFile)
    {
        // Truncate the file
        std::wofstream logFile(s_LogFilePath, std::ios::trunc);
        if (logFile.is_open())
        {
            logFile.close();
        }
    }
}

/*
** Set the minimum log level for output.
** Messages below this level will be ignored.
*/
void Logging::SetLogLevel(LogLevel minLevel)
{
    s_MinLevel = minLevel;
}
