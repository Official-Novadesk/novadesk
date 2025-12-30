/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "JSCommon.h"
#include "JSUtils.h"
#include "JSSystem.h"
#include "JSWidget.h"
#include "JSElement.h"
#include "JSContextMenu.h"
#include "JSEvents.h"
#include "JSIPC.h"

#include "../Widget.h"
#include "../Settings.h"
#include "../Logging.h"
#include "../PathUtils.h"
#include "../FileUtils.h"
#include "../TimerManager.h"
#include "../Hotkey.h"
#include "../Utils.h"

#include <vector>

extern std::vector<Widget*> widgets;

namespace JSApi {

    void InitializeJavaScriptAPI(duk_context* ctx) {
        s_JsContext = ctx;

        // Register novadesk object and methods
        duk_push_object(ctx);
        BindNovadeskBaseMethods(ctx);
        BindNovadeskAppMethods(ctx);
        duk_put_global_string(ctx, "novadesk");

        // Register Managers
        HotkeyManager::SetCallbackHandler([](int idx) {
            CallHotkeyCallback(idx);
        });
        TimerManager::SetCallbackHandler([](int idx) {
            CallStoredCallback(idx);
        });

        // Register system object
        duk_push_object(ctx);
        BindSystemBaseMethods(ctx);
        BindSystemMonitors(ctx);
        duk_put_global_string(ctx, "system");

        // Register timers
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 1);
        duk_put_global_string(ctx, "setInterval");
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 0);
        duk_put_global_string(ctx, "setTimeout");
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearInterval");
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearTimeout");
        duk_push_c_function(ctx, js_set_immediate, 1);
        duk_put_global_string(ctx, "setImmediate");

        // Register widgetWindow
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        // Register global IPC
        BindIPCMethods(ctx);

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath) {
        Settings::ApplyGlobalSettings();
        std::wstring finalScriptPath;
        if (!scriptPath.empty()) {
            finalScriptPath = scriptPath;
            if (PathUtils::IsPathRelative(finalScriptPath)) {
                finalScriptPath = PathUtils::ResolvePath(finalScriptPath);
            }
            s_CurrentScriptPath = finalScriptPath;
        } else {
            if (s_CurrentScriptPath.empty()) {
                finalScriptPath = PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
                s_CurrentScriptPath = finalScriptPath;
            } else {
                finalScriptPath = s_CurrentScriptPath;
            }
        }

        Logging::Log(LogLevel::Info, L"Loading script from: %s", finalScriptPath.c_str());
        std::string content = FileUtils::ReadFileContent(finalScriptPath);
        if (content.empty()) {
            Logging::Log(LogLevel::Error, L"Failed to open script at %s", finalScriptPath.c_str());
            return false;
        }

        if (duk_peval_string(ctx, content.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
            duk_pop(ctx);
            return false;
        }

        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "readyCallback")) {
            if (duk_is_function(ctx, -1)) {
                if (duk_pcall(ctx, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Ready callback failed: %S", duk_safe_to_string(ctx, -1));
                }
            }
        }
        duk_pop_2(ctx);
        return true;
    }

    void ReloadScripts(duk_context* ctx) {
        Logging::Log(LogLevel::Info, L"Reloading scripts...");
        HotkeyManager::UnregisterAll();
        TimerManager::ClearAll();
        for (auto w : widgets) delete w;
        widgets.clear();

        duk_get_global_string(ctx, "novadesk");
        duk_del_prop_string(ctx, -1, "__hotkeys");
        duk_del_prop_string(ctx, -1, "__timers");
        duk_pop(ctx);
 
        s_NextTempId = 1;
        s_NextTimerId = 1;
        s_NextImmId = 1;

        LoadAndExecuteScript(ctx, L"");
    }

    void Reload() {
        if (s_JsContext) ReloadScripts(s_JsContext);
    }

    void OnTimer(UINT_PTR id) {
        TimerManager::HandleTimer(id);
    }
 
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        TimerManager::HandleMessage(message, wParam, lParam);
    }
 
    void SetMessageWindow(HWND hWnd) {
        TimerManager::Initialize(hWnd);
    }
}
