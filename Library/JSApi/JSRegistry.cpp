/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSRegistry.h"
#include "../Utils.h"
#include "../Logging.h"
#include <windows.h>
#include <string>

namespace JSApi {

    static HKEY GetRootKey(const std::wstring& root) {
        if (root == L"HKCU" || root == L"HKEY_CURRENT_USER") return HKEY_CURRENT_USER;
        if (root == L"HKLM" || root == L"HKEY_LOCAL_MACHINE") return HKEY_LOCAL_MACHINE;
        if (root == L"HKCR" || root == L"HKEY_CLASSES_ROOT") return HKEY_CLASSES_ROOT;
        if (root == L"HKU" || root == L"HKEY_USERS") return HKEY_USERS;
        return NULL;
    }

    duk_ret_t js_system_readRegistry(duk_context* ctx) {
        if (duk_get_top(ctx) < 2) return DUK_RET_TYPE_ERROR;

        std::wstring fullPath = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring valueName = Utils::ToWString(duk_get_string(ctx, 1));

        size_t firstBackslash = fullPath.find(L"\\");
        if (firstBackslash == std::wstring::npos) return 0;

        std::wstring rootPart = fullPath.substr(0, firstBackslash);
        std::wstring subKeyPart = fullPath.substr(firstBackslash + 1);

        HKEY hRoot = GetRootKey(rootPart);
        if (!hRoot) return 0;

        HKEY hKey;
        if (RegOpenKeyExW(hRoot, subKeyPart.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return 0;
        }

        DWORD dwType;
        DWORD dwSize = 0;
        if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return 0;
        }

        if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
            std::vector<wchar_t> buffer(dwSize / sizeof(wchar_t) + 1);
            if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &dwType, (LPBYTE)buffer.data(), &dwSize) == ERROR_SUCCESS) {
                duk_push_string(ctx, Utils::ToString(buffer.data()).c_str());
            } else {
                duk_push_null(ctx);
            }
        } else if (dwType == REG_DWORD) {
            DWORD dwValue;
            if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
                duk_push_number(ctx, (double)dwValue);
            } else {
                duk_push_null(ctx);
            }
        } else {
            duk_push_null(ctx);
        }

        RegCloseKey(hKey);
        return 1;
    }

    duk_ret_t js_system_writeRegistry(duk_context* ctx) {
        if (duk_get_top(ctx) < 3) return DUK_RET_TYPE_ERROR;

        std::wstring fullPath = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring valueName = Utils::ToWString(duk_get_string(ctx, 1));

        size_t firstBackslash = fullPath.find(L"\\");
        if (firstBackslash == std::wstring::npos) return 0;

        std::wstring rootPart = fullPath.substr(0, firstBackslash);
        std::wstring subKeyPart = fullPath.substr(firstBackslash + 1);

        HKEY hRoot = GetRootKey(rootPart);
        if (!hRoot) return 0;

        HKEY hKey;
        if (RegCreateKeyExW(hRoot, subKeyPart.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
            return 0;
        }

        bool success = false;
        if (duk_is_string(ctx, 2)) {
            std::wstring data = Utils::ToWString(duk_get_string(ctx, 2));
            if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (const BYTE*)data.c_str(), (DWORD)((data.length() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS) {
                success = true;
            }
        } else if (duk_is_number(ctx, 2)) {
            DWORD data = (DWORD)duk_get_number(ctx, 2);
            if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&data, sizeof(DWORD)) == ERROR_SUCCESS) {
                success = true;
            }
        }

        RegCloseKey(hKey);
        duk_push_boolean(ctx, success);
        return 1;
    }

    void BindRegistryMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_system_readRegistry, 2);
        duk_put_prop_string(ctx, -2, "readRegistry");

        duk_push_c_function(ctx, js_system_writeRegistry, 3);
        duk_put_prop_string(ctx, -2, "writeRegistry");
    }
}
