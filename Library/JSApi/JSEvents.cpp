/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSEvents.h"
#include "../Widget.h"
#include "../Logging.h"
#include "../FileUtils.h"
#include "../PropertyParser.h"
#include "../Utils.h"
#include "JSWidget.h"
#include "JSElement.h"
#include "JSSystem.h"
#include "JSUtils.h"
#include "JSIPC.h"

namespace JSApi {
    void ExecuteScript(const std::wstring& script) {
        if (!s_JsContext) {
            Logging::Log(LogLevel::Error, L"ExecuteScript: Context not set");
            return;
        }

        std::string scriptStr = Utils::ToString(script);
        if (duk_peval_string(s_JsContext, scriptStr.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"JS Error: %S", duk_safe_to_string(s_JsContext, -1));
        }
        duk_pop(s_JsContext);
    }

    duk_ret_t js_on_ready(duk_context* ctx) {
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
        duk_push_global_stash(ctx);
        duk_dup(ctx, 0);
        duk_put_prop_string(ctx, -2, "readyCallback");
        duk_pop(ctx);
        return 0;
    }

    void ExecuteWidgetScript(Widget* widget) {
        if (!s_JsContext || !widget) return;
        const std::wstring& scriptPath = widget->GetOptions().scriptPath;
        if (scriptPath.empty()) return;

        std::string content = FileUtils::ReadFileContent(scriptPath);
        if (content.empty()) {
            Logging::Log(LogLevel::Error, L"ExecuteWidgetScript: Failed to read %s", scriptPath.c_str());
            return;
        }

        duk_push_global_stash(s_JsContext);
        duk_get_global_string(s_JsContext, "win");
        duk_put_prop_string(s_JsContext, -2, "original_win");
        duk_get_global_string(s_JsContext, "system");
        duk_put_prop_string(s_JsContext, -2, "original_system");
        duk_get_global_string(s_JsContext, "novadesk");
        duk_put_prop_string(s_JsContext, -2, "original_novadesk");

        // Save and remove timers and constructors
        const char* forbidden[] = { 
            "setInterval", "setTimeout", "clearInterval", "clearTimeout", "setImmediate",
            "widgetWindow"
        };
        for (const char* f : forbidden) {
            duk_get_global_string(s_JsContext, f);
            duk_put_prop_string(s_JsContext, -2, f); // Save to stash
            duk_push_global_object(s_JsContext);
            duk_del_prop_string(s_JsContext, -1, f);
            duk_pop(s_JsContext);
        }

        duk_pop(s_JsContext); // Pop stash

        PropertyParser::PushWidgetProperties(s_JsContext, widget);
        std::string idStr = Utils::ToString(widget->GetOptions().id);
        duk_push_string(s_JsContext, idStr.c_str());
        duk_put_prop_string(s_JsContext, -2, "\xFF" "id");

        BindWidgetUIMethods(s_JsContext);
        duk_put_global_string(s_JsContext, "win");

        duk_push_object(s_JsContext);
        BindSystemBaseMethods(s_JsContext);
        duk_put_global_string(s_JsContext, "system");

        duk_push_object(s_JsContext);
        BindNovadeskBaseMethods(s_JsContext);
        duk_put_global_string(s_JsContext, "novadesk");

        BindIPCMethods(s_JsContext);

        if (duk_peval_string(s_JsContext, content.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Widget Script Error (%s): %S", widget->GetOptions().id.c_str(), duk_safe_to_string(s_JsContext, -1));
        }
        duk_pop(s_JsContext);

        duk_push_global_stash(s_JsContext);
        duk_get_prop_string(s_JsContext, -1, "original_win");
        duk_put_global_string(s_JsContext, "win");
        duk_get_prop_string(s_JsContext, -1, "original_system");
        duk_put_global_string(s_JsContext, "system");
        duk_get_prop_string(s_JsContext, -1, "original_novadesk");
        duk_put_global_string(s_JsContext, "novadesk");

        // Restore timers and constructors
        for (const char* f : forbidden) {
            duk_get_prop_string(s_JsContext, -1, f);
            duk_put_global_string(s_JsContext, f);
            duk_del_prop_string(s_JsContext, -1, f); // Clean up stash
        }

        duk_pop(s_JsContext);
    }

    void CallStoredCallback(int id) {
        if (!s_JsContext) return;
        duk_get_global_string(s_JsContext, "novadesk");
        duk_get_prop_string(s_JsContext, -1, "__timers");
        duk_push_int(s_JsContext, id);
        if (duk_get_prop(s_JsContext, -2)) {
            if (duk_is_function(s_JsContext, -1)) {
                if (duk_pcall(s_JsContext, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Timer callback error: %S", duk_safe_to_string(s_JsContext, -1));
                }
            }
            duk_pop(s_JsContext);
        } else {
            duk_pop(s_JsContext);
        }
        duk_pop_2(s_JsContext);
    }

    void CallHotkeyCallback(int callbackIdx) {
        if (!s_JsContext || callbackIdx < 0) return;
        duk_get_global_string(s_JsContext, "novadesk");
        if (duk_get_prop_string(s_JsContext, -1, "__hotkeys")) {
            duk_push_int(s_JsContext, callbackIdx);
            if (duk_get_prop(s_JsContext, -2)) {
                if (duk_is_function(s_JsContext, -1)) {
                    if (duk_pcall(s_JsContext, 0) != 0) {
                        Logging::Log(LogLevel::Error, L"Hotkey callback error: %S", duk_safe_to_string(s_JsContext, -1));
                    }
                }
                duk_pop(s_JsContext);
            } else {
                duk_pop(s_JsContext);
            }
        }
        duk_pop_2(s_JsContext);
    }
}
