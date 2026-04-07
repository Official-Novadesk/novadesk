/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>
#include <vector>

namespace PathUtils {
    struct PathParts
    {
        std::wstring root;
        std::wstring dir;
        std::wstring base;
        std::wstring ext;
        std::wstring name;
    };

    std::wstring GetExePath();
    std::wstring GetExeDir();
    std::wstring GetWidgetsDir();
    std::wstring GetAddonsDir();
    bool IsPortableEnvironment();
    std::wstring GetAppDataPath();
    std::wstring GetProductName();
    std::wstring GetParentDir(const std::wstring& path);

    bool IsPathRelative(const std::wstring& path);
    std::wstring Join(const std::vector<std::wstring>& parts);
    std::wstring Basename(const std::wstring& path, const std::wstring& ext = L"");
    std::wstring Dirname(const std::wstring& path);
    std::wstring Extname(const std::wstring& path);
    bool IsAbsolute(const std::wstring& path);
    std::wstring Relative(const std::wstring& from, const std::wstring& to);
    PathParts Parse(const std::wstring& path);
    std::wstring Format(const PathParts& parts);

    std::wstring NormalizePath(const std::wstring& path);
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir = L"");
}
