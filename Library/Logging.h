/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LOGGING_H__
#define __NOVADESK_LOGGING_H__

#include <windows.h>
#include <string>

enum class LogLevel
{
    Debug = 0,
    Info = 1,
    Error = 2
};

class Logging
{
public:
    /*
    ** Log a message to the debug output and/or file.
    ** Supports formatted output similar to printf.
    ** Messages are prefixed with [INFO], [ERROR], or [DEBUG] based on level.
    */
    static void Log(LogLevel level, const wchar_t* format, ...);

    /*
    ** Enable or disable console (debug output) logging.
    */
    static void SetConsoleLogging(bool enable);

    /*
    ** Enable file logging. Logs will be written to the specified file.
    ** Pass empty string to disable file logging.
    */
    static void SetFileLogging(const std::wstring& filePath);

    /*
    ** Set the minimum log level. Messages below this level are ignored.
    */
    static void SetLogLevel(LogLevel minLevel);

private:
    static bool s_ConsoleEnabled;
    static bool s_FileEnabled;
    static std::wstring s_LogFilePath;
    static LogLevel s_MinLevel;
};

#endif
