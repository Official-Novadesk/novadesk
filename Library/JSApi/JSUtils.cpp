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
        
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, tempId);
        duk_dup(ctx, 0);
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx);

        int id = TimerManager::Register(ms, tempId, repeating);
        duk_push_int(ctx, id);
        return 1;
    }

    duk_ret_t js_clear_timer(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return 0;
        int id = duk_get_int(ctx, 0);
        
        TimerManager::Clear(id);

        duk_get_global_string(ctx, "novadesk");
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

        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, tempId);
        duk_dup(ctx, 0);
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx);

        int id = TimerManager::PushImmediate(tempId);
        duk_push_int(ctx, id);
        return 1;
    }

    duk_ret_t js_include(duk_context* ctx) {
        std::wstring filename = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring fullPath = PathUtils::ResolvePath(filename, PathUtils::GetWidgetsDir());

        std::string content = FileUtils::ReadFileContent(fullPath);
        if (content.empty()) {
            return duk_error(ctx, DUK_ERR_ERROR, "Could not read file: %s", Utils::ToString(filename).c_str());
        }

        if (duk_peval_string(ctx, content.c_str()) != 0) {
            return duk_throw(ctx);
        }

        duk_pop(ctx);
        return 0;
    }

    duk_ret_t js_novadesk_saveLogToFile(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool enable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("saveLogToFile", enable);
        if (enable) {
            std::wstring logPath = PathUtils::GetExeDir() + L"novadesk.log";
            Logging::SetFileLogging(logPath, true);
        } else {
            Logging::SetFileLogging(L"");
        }
        return 0;
    }

    duk_ret_t js_novadesk_enableDebugging(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool enable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("enableDebugging", enable);
        Logging::SetLogLevel(enable ? LogLevel::Debug : LogLevel::Info);
        return 0;
    }

    duk_ret_t js_novadesk_disableLogging(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool disable = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("disableLogging", disable);
        Logging::SetConsoleLogging(!disable);
        if (disable) {
             Logging::SetFileLogging(L"");
        } else if (Settings::GetGlobalBool("saveLogToFile", false)) {
             std::wstring logPath = PathUtils::GetExeDir() + L"novadesk.log";
             Logging::SetFileLogging(logPath, true);
        }
        return 0;
    }

    duk_ret_t js_novadesk_hideTrayIcon(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return DUK_RET_TYPE_ERROR;
        bool hide = duk_get_boolean(ctx, 0);
        Settings::SetGlobalBool("hideTrayIcon", hide);
        if (hide) ::HideTrayIconDynamic();
        else ::ShowTrayIconDynamic();
        return 0;
    }

    duk_ret_t js_novadesk_refresh(duk_context* ctx) {
        ReloadScripts(ctx);
        return 0;
    }

    duk_ret_t js_novadesk_exit(duk_context* ctx) {
        PostQuitMessage(0);
        return 0;
    }

    void BindNovadeskBaseMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_log, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "log");
        duk_push_c_function(ctx, js_error, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "error");
        duk_push_c_function(ctx, js_debug, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "debug");
        duk_push_c_function(ctx, js_include, 1);
        duk_put_prop_string(ctx, -2, "include");
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
        duk_push_c_function(ctx, js_novadesk_refresh, 0);
        duk_put_prop_string(ctx, -2, "refresh");
        duk_push_c_function(ctx, js_novadesk_exit, 0);
        duk_put_prop_string(ctx, -2, "exit");
    }
}
