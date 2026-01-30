/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PathUtils.h"
#include <Windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "Version.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

#include <vector>

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
    ** Check if a path is relative.
    */
    bool IsPathRelative(const std::wstring& path) {
        return PathIsRelativeW(path.c_str()) != FALSE;
    }

    /*
    ** Normalize a path by converting slashes and resolving . and .. segments.
    */
    std::wstring NormalizePath(const std::wstring& path) {
        if (path.empty()) return L"";
        
        std::wstring res = path;
        for (auto& c : res) if (c == L'/') c = L'\\';

        // Check if absolute and what type of root it has
        bool isAbsolute = !IsPathRelative(res);
        std::wstring root;
        if (isAbsolute) {
            if (res.length() >= 2 && res[1] == L':') {
                root = res.substr(0, 3);
                if (root.back() != L'\\') root += L'\\';
            } else if (res.front() == L'\\') {
                root = L"\\";
            }
        }

        // Split by backslash
        std::vector<std::wstring> segments;
        size_t start = root.length(), end;
        while ((end = res.find(L'\\', start)) != std::wstring::npos) {
            segments.push_back(res.substr(start, end - start));
            start = end + 1;
        }
        segments.push_back(res.substr(start));

        std::vector<std::wstring> resolved;
        for (const auto& s : segments) {
            if (s.empty() || s == L".") continue;
            if (s == L"..") {
                if (!resolved.empty() && resolved.back() != L"..") {
                    resolved.pop_back();
                } else if (!isAbsolute) {
                    resolved.push_back(L"..");
                }
            } else {
                resolved.push_back(s);
            }
        }

        std::wstring result = root;
        for (size_t i = 0; i < resolved.size(); ++i) {
            if (!result.empty() && result.back() != L'\\') result += L'\\';
            result += resolved[i];
        }
        
        if (result.empty()) return L".";
        return result;
    }

    /*
    ** Get the directory containing the widgets.
    */
    std::wstring GetWidgetsDir() {
        return GetExeDir() + L"Widgets\\";
    }

    /*
    ** Get the AppData path for the product.
    */
    std::wstring GetAppDataPath() {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            std::wstring appData = path;
            appData += L"\\" + GetProductName() + L"\\";
            
            // Ensure directory exists
            CreateDirectoryW(appData.c_str(), NULL);
            
            return appData;
        }
        return GetExeDir(); // Fallback
    }

    /*
    ** Get the product name from the executable's version resources.
    */
    std::wstring GetProductName() {
        static std::wstring cachedProductName;
        if (!cachedProductName.empty()) return cachedProductName;

        std::wstring exePath = GetExePath();
        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeW(exePath.c_str(), &handle);
        if (size > 0) {
            std::vector<BYTE> buffer(size);
            if (GetFileVersionInfoW(exePath.c_str(), handle, size, buffer.data())) {
                struct LANGANDCODEPAGE {
                    WORD wLanguage;
                    WORD wCodePage;
                } *lpTranslate;
                UINT cbTranslate;

                // Read the list of languages and code pages.
                if (VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
                    for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
                        wchar_t subBlock[256];
                        swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\ProductName",
                                   lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

                        wchar_t* productName = nullptr;
                        UINT productNameLen = 0;
                        if (VerQueryValueW(buffer.data(), subBlock, (LPVOID*)&productName, &productNameLen) && productNameLen > 0) {
                            cachedProductName = productName;
                            return cachedProductName;
                        }
                    }
                }
            }
        }
        cachedProductName = L"Novadesk"; // Fallback
        return cachedProductName;
    }

    /*
    ** Get the parent directory of a path.
    */
    std::wstring GetParentDir(const std::wstring& path) {
        if (path.empty()) return L"";
        
        std::wstring norm = NormalizePath(path);
        
        // Remove trailing backslash if it's not the root
        while (norm.length() > 3 && norm.back() == L'\\') {
            norm.pop_back();
        }

        size_t lastBackslash = norm.find_last_of(L"\\");
        if (lastBackslash != std::wstring::npos) {
            return norm.substr(0, lastBackslash + 1);
        }
        return L"";
    }

    /*
    ** Resolve a path to an absolute path.
    */
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir) {
        if (path.empty()) return L"";
        
        std::wstring res = path;
        for (auto& c : res) if (c == L'/') c = L'\\';

        if (!IsPathRelative(res)) {
            return NormalizePath(res);
        }

        std::wstring finalBase = baseDir.empty() ? GetExeDir() : baseDir;
        if (!finalBase.empty() && finalBase.back() != L'\\') finalBase += L'\\';

        std::wstring combined = finalBase + res;
        
        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(combined.c_str(), MAX_PATH, fullPath, NULL)) {
            return std::wstring(fullPath);
        }

        return NormalizePath(combined);
    }
}
