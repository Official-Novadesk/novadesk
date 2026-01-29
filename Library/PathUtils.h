/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

namespace PathUtils {

    std::wstring GetExePath();
    std::wstring GetExeDir();
    std::wstring GetWidgetsDir();
    std::wstring GetParentDir(const std::wstring& path);

    bool IsPathRelative(const std::wstring& path);

    std::wstring NormalizePath(const std::wstring& path);
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir = L"");
}
