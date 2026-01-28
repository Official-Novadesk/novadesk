/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSUtils.h"
#include "JSEvents.h"
#include "../Logging.h"
#include "../Utils.h"
#include "../TimerManager.h"
#include "../PathUtils.h"
#include "../FileUtils.h"
#include "../Settings.h"
#include "../Novadesk.h"
#include "../Version.h"

#pragma comment(lib, "version.lib")

namespace JSApi {

    duk_ret_t js_log(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Info, L"%s", msg.c_str());
        return 0;
    }

    duk_ret_t js_warn(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Warn, L"%s", msg.c_str());
        return 0;
    }

    duk_ret_t js_error(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Error, L"%s", msg.c_str());
        return 0;
    }

    duk_ret_t js_debug(duk_context* ctx) {
        for (int i = 0; i < duk_get_top(ctx); i++) {
            Logging::Log(LogLevel::Debug, L"%S", duk_safe_to_string(ctx, i));
        }
        return 0;
    }

    duk_ret_t js_set_timer(duk_context* ctx) {
        bool repeating = duk_get_current_magic(ctx) != 0;
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int ms = duk_get_int_default(ctx, 1, 0);

        int tempId = s_NextTimerId++;
        
        duk_push_int(ctx, tempId);

        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_dup(ctx, 0); // duplicate function
        duk_put_prop_index(ctx, -2, (duk_uarridx_t)tempId);
        duk_pop_2(ctx); // pop __timers and global_stash

        int id = TimerManager::Register(ms, tempId, repeating);
        duk_push_int(ctx, id);
        return 1;
    }

    duk_ret_t js_clear_timer(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return 0;
        int id = duk_get_int(ctx, 0);
        
        TimerManager::Clear(id);
        
        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "__timers")) {
            duk_push_int(ctx, id);
            duk_del_prop(ctx, -2);
        }
        duk_pop_2(ctx);
        return 0;
    }

    duk_ret_t js_set_immediate(duk_context* ctx) {
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;

        int tempId = s_NextImmId++;

        duk_push_int(ctx, tempId);

        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_dup(ctx, 0); // duplicate function
        duk_put_prop_index(ctx, -2, (duk_uarridx_t)tempId);
        duk_pop_2(ctx); // pop __timers and global_stash

        int id = TimerManager::PushImmediate(tempId);
        duk_push_int(ctx, id);
        return 1;
    }

    duk_ret_t js_include(duk_context* ctx) {
        std::wstring filename = Utils::ToWString(duk_require_string(ctx, 0));
        
        std::wstring fullPath = ResolveScriptPath(ctx, filename);

        std::string content = FileUtils::ReadFileContent(fullPath);
        if (content.empty()) {
            return duk_error(ctx, DUK_ERR_ERROR, "Could not read file: %s", Utils::ToString(filename).c_str());
        }

        if (duk_peval_string(ctx, content.c_str()) != 0) {
            return duk_throw(ctx);
        }

        duk_pop(ctx);
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_saveLogToFile(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool enable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("saveLogToFile", enable);
        if (enable) {
            std::wstring logPath = PathUtils::GetExeDir() + L"logs.log";
            Logging::SetFileLogging(logPath, false);
        } else {
            Logging::SetFileLogging(L"");
        }
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_enableDebugging(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool enable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("enableDebugging", enable);
        Logging::SetLogLevel(enable ? LogLevel::Debug : LogLevel::Info);
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_disableLogging(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool disable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("disableLogging", disable);
        Logging::SetConsoleLogging(!disable);
        if (disable) {
             Logging::SetFileLogging(L"");
        } else if (Settings::GetGlobalBool("saveLogToFile", false)) {
             std::wstring logPath = PathUtils::GetExeDir() + L"logs.log";
             Logging::SetFileLogging(logPath, false);
        }
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_hideTrayIcon(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool hide = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("hideTrayIcon", hide);
        if (hide) ::HideTrayIconDynamic();
        else ::ShowTrayIconDynamic();
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_useHardwareAcceleration(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool enable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("useHardwareAcceleration", enable);
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_refresh(duk_context* ctx) {
        ReloadScripts(ctx);
        duk_push_boolean(ctx, true);
        return 1;
    }

    duk_ret_t js_novadesk_exit(duk_context* ctx) {
        PostQuitMessage(0);
        duk_push_boolean(ctx, true);
        return 1;
    }

    std::wstring GetVersionProperty(const std::wstring& propertyName) {
        wchar_t szExePath[MAX_PATH];
        GetModuleFileNameW(NULL, szExePath, MAX_PATH);

        DWORD dwHandle = 0;
        DWORD dwSize = GetFileVersionInfoSizeW(szExePath, &dwHandle);
        if (dwSize == 0) return L"Unknown";

        std::vector<BYTE> data(dwSize);
        if (!GetFileVersionInfoW(szExePath, dwHandle, dwSize, data.data())) return L"Unknown";

        struct LANGANDCODEPAGE {
            WORD wLanguage;
            WORD wCodePage;
        } *lpTranslate;
        UINT cbTranslate;

        if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
            return L"Unknown";

        wchar_t szSubBlock[100];
        swprintf_s(szSubBlock, L"\\StringFileInfo\\%04x%04x\\%ls",
                   lpTranslate[0].wLanguage, lpTranslate[0].wCodePage, propertyName.c_str());

        LPVOID lpBuffer;
        UINT dwLen;
        if (VerQueryValueW(data.data(), szSubBlock, &lpBuffer, &dwLen)) {
            return std::wstring((wchar_t*)lpBuffer);
        }

        return L"Unknown";
    }

    duk_ret_t js_novadesk_getProductVersion(duk_context* ctx) {
        std::wstring version = GetVersionProperty(L"ProductVersion");
        duk_push_string(ctx, Utils::ToString(version).c_str());
        return 1;
    }

    duk_ret_t js_novadesk_getFileVersion(duk_context* ctx) {
        std::wstring version = GetVersionProperty(L"FileVersion");
        duk_push_string(ctx, Utils::ToString(version).c_str());
        return 1;
    }

    duk_ret_t js_novadesk_getNovadeskVersion(duk_context* ctx) {
        duk_push_string(ctx, NOVADESK_VERSION);
        return 1;
    }

    void BindConsoleMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_log, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "log");
        duk_push_c_function(ctx, js_warn, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "warn");
        duk_push_c_function(ctx, js_error, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "error");
        duk_push_c_function(ctx, js_debug, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "debug");
    }

    void BindNovadeskAppMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_novadesk_saveLogToFile, 1);
        duk_put_prop_string(ctx, -2, "saveLogToFile");
        duk_push_c_function(ctx, js_novadesk_enableDebugging, 1);
        duk_put_prop_string(ctx, -2, "enableDebugging");
        duk_push_c_function(ctx, js_novadesk_disableLogging, 1);
        duk_put_prop_string(ctx, -2, "disableLogging");
        duk_push_c_function(ctx, js_novadesk_hideTrayIcon, 1);
        duk_put_prop_string(ctx, -2, "hideTrayIcon");
        duk_push_c_function(ctx, js_novadesk_useHardwareAcceleration, 1);
        duk_put_prop_string(ctx, -2, "useHardwareAcceleration");
        duk_push_c_function(ctx, js_novadesk_refresh, 0);
        duk_put_prop_string(ctx, -2, "refresh");
        duk_push_c_function(ctx, js_novadesk_exit, 0);
        duk_put_prop_string(ctx, -2, "exit");
        duk_push_c_function(ctx, js_novadesk_getProductVersion, 0);
        duk_put_prop_string(ctx, -2, "getProductVersion");
        duk_push_c_function(ctx, js_novadesk_getFileVersion, 0);
        duk_put_prop_string(ctx, -2, "getFileVersion");
        duk_push_c_function(ctx, js_novadesk_getNovadeskVersion, 0);
        duk_put_prop_string(ctx, -2, "getNovadeskVersion");
    }

    std::wstring ResolveScriptPath(duk_context* ctx, const std::wstring& path) {
        if (path.empty()) return L"";
        if (!PathUtils::IsPathRelative(path)) return PathUtils::NormalizePath(path);

        std::wstring baseDir = PathUtils::GetWidgetsDir();
        
        duk_get_global_string(ctx, "__dirname");
        if (duk_is_string(ctx, -1)) {
            std::wstring dirname = Utils::ToWString(duk_get_string(ctx, -1));
            if (!dirname.empty()) {
                baseDir = dirname;
                if (baseDir.back() != L'\\') baseDir += L'\\';
            }
        }
        duk_pop(ctx);

        return PathUtils::ResolvePath(path, baseDir);
    }
}
