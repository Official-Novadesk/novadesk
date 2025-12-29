/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PathUtils.h"
#include <Windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace PathUtils {

    /*
    ** Get the full path to the current executable.
    */

    std::wstring GetExePath() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::wstring(path);
    }

    /*
    ** Get the directory of the current executable.
    */
    std::wstring GetExeDir() {
        std::wstring fullPath = GetExePath();
        size_t lastBackslash = fullPath.find_last_of(L"\\");
        if (lastBackslash != std::wstring::npos) {
            return fullPath.substr(0, lastBackslash + 1);
        }
        return L"";
    }

    /*
    ** Get the directory containing the widgets.
    */
    std::wstring GetWidgetsDir() {
        return GetExeDir() + L"Widgets\\";
    }

    /*
    ** Check if a path is relative.
    */
    bool IsPathRelative(const std::wstring& path) {
        return PathIsRelativeW(path.c_str()) != FALSE;
    }

    /*
    ** Resolve a path to an absolute path.
    */
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir) {
        if (!IsPathRelative(path)) {
            return path;
        }

        std::wstring finalBase = baseDir.empty() ? GetExeDir() : baseDir;
        // Ensure trailing backslash
        if (!finalBase.empty() && finalBase.back() != L'\\') {
            finalBase += L'\\';
        }

        return finalBase + path;
    }
}
