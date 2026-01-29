/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Utils.h"
#include <Windows.h>
#include <vector>

#pragma comment(lib, "version.lib")

namespace Utils {

    /*
    ** Convert a UTF-8 encoded std::string to a wide character std::wstring.
    ** Returns an empty wstring if the input string is empty.
    ** Uses Windows MultiByteToWideChar for conversion.
    */

    std::wstring ToWString(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    /*
    ** Convert a wide character std::wstring to a UTF-8 encoded std::string.
    ** Returns an empty string if the input wstring is empty.
    */

    std::string ToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    /*
    ** Retrieve the ProductName version string from the current executable.
    ** Falls back to IDS_APP_TITLE resource if version info is unavailable.
    */
    std::wstring GetAppTitle() {
        static std::wstring cachedTitle;
        if (!cachedTitle.empty()) return cachedTitle;

        WCHAR szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, MAX_PATH);

        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeW(szPath, &handle);
        if (size > 0) {
            std::vector<BYTE> buffer(size);
            if (GetFileVersionInfoW(szPath, handle, size, buffer.data())) {
                struct LANGANDCODEPAGE {
                    WORD wLanguage;
                    WORD wCodePage;
                } *lpTranslate;
                UINT cbTranslate;

                // Read the list of languages and code pages.
                if (VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
                    for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
                        WCHAR subBlock[256];
                        swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\ProductName",
                            lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

                        LPWSTR lpBuffer = NULL;
                        UINT dwBytes = 0;
                        if (VerQueryValueW(buffer.data(), subBlock, (LPVOID*)&lpBuffer, &dwBytes) && dwBytes > 0) {
                            cachedTitle = lpBuffer;
                            return cachedTitle;
                        }
                    }
                }
            }
        }

        // Fallback to resource string if version info retrieval fails
        WCHAR szTitleRes[100];
        if (LoadStringW(GetModuleHandle(NULL), 103, szTitleRes, 100)) { // 103 is IDS_APP_TITLE commonly
            cachedTitle = szTitleRes;
        } else {
            cachedTitle = L"Novadesk";
        }

        return cachedTitle;
    }
}
