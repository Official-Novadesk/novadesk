/* Copyright (C) 2026 OfficialNovadesk 
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
#include "../PathUtils.h"
#include "JSWidget.h"
#include "JSElement.h"
#include "JSSystem.h"
#include "JSUtils.h"
#include "JSIPC.h"
#include "JSJson.h"
#include "JSPath.h"


namespace JSApi {
    static int s_NextEventCallbackId = 1;

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


    void CleanupWidget(const std::wstring& id) {
        if (!s_JsContext) return;
        std::string idStr = Utils::ToString(id);
        ClearIPCListeners(s_JsContext, idStr);
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
        duk_get_global_string(s_JsContext, "path");
        duk_put_prop_string(s_JsContext, -2, "original_path");

        duk_get_global_string(s_JsContext, "__dirname");
        duk_put_prop_string(s_JsContext, -2, "original_dirname");
        duk_get_global_string(s_JsContext, "__filename");
        duk_put_prop_string(s_JsContext, -2, "original_filename");

        // Save and remove timers and constructors
        const char* forbidden[] = { 
            "setInterval", "setTimeout", "clearInterval", "clearTimeout", "setImmediate",
            "widgetWindow", "system"
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
        
        // Add internal pointers for C++ identification
        duk_push_pointer(s_JsContext, widget);
        duk_put_prop_string(s_JsContext, -2, "\xFF" "widgetPtr");
        
        std::string idStr = Utils::ToString(widget->GetOptions().id);
        duk_push_string(s_JsContext, idStr.c_str());
        duk_put_prop_string(s_JsContext, -2, "\xFF" "id");

        BindWidgetUIMethods(s_JsContext);
        duk_put_global_string(s_JsContext, "win");

        BindPathMethods(s_JsContext);
        
        // Clear old IPC listeners for this specific widget before binding new ones
        // This prevents old handlers from persisting after widget refresh
        std::string widgetId = Utils::ToString(widget->GetOptions().id);
        ClearIPCListeners(s_JsContext, widgetId);
        
        BindIPCMethods(s_JsContext);

        // Provide __dirname and __filename
        std::string dirname = Utils::ToString(PathUtils::GetParentDir(scriptPath));
        if (dirname.length() > 3 && dirname.back() == '\\') dirname.pop_back();
        std::string filename = Utils::ToString(scriptPath);
        duk_push_string(s_JsContext, dirname.c_str());
        duk_put_global_string(s_JsContext, "__dirname");
        duk_push_string(s_JsContext, filename.c_str());
        duk_put_global_string(s_JsContext, "__filename");

        // Wrap the script in an IIFE (Immediately Invoked Function Expression) to provide 
        // variable isolation. This prevents top-level 'var' and 'function' declarations 
        // from colliding between different widgets that share the same Duktape heap.
        // The parameters (win, novadesk, etc.) ensure these widget-specific objects 
        // are trapped in the closure. By adding restricted globals like 'system' 
        // to the parameters but NOT the arguments, we shadow them with 'undefined'.
        std::string wrappedContent = "(function(win, ipc, path, __dirname, __filename, system, app, setInterval, setTimeout, clearInterval, clearTimeout, setImmediate, widgetWindow) {\n";
        wrappedContent += content;
        wrappedContent += "\n})(win, ipc, path, __dirname, __filename);";

        widget->BeginUpdate();
        if (duk_peval_string(s_JsContext, wrappedContent.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Widget Script Error (%s): %S", widget->GetOptions().id.c_str(), duk_safe_to_string(s_JsContext, -1));
        }
        widget->EndUpdate();
        
        duk_pop(s_JsContext);

        duk_push_global_stash(s_JsContext);
        duk_get_prop_string(s_JsContext, -1, "original_win");
        duk_put_global_string(s_JsContext, "win");
        duk_get_prop_string(s_JsContext, -1, "original_path");
        duk_put_global_string(s_JsContext, "path");

        duk_get_prop_string(s_JsContext, -1, "original_dirname");
        duk_put_global_string(s_JsContext, "__dirname");
        duk_get_prop_string(s_JsContext, -1, "original_filename");
        duk_put_global_string(s_JsContext, "__filename");

        // Restore timers and constructors
        for (const char* f : forbidden) {
            duk_get_prop_string(s_JsContext, -1, f);
            duk_put_global_string(s_JsContext, f);
            duk_del_prop_string(s_JsContext, -1, f); // Clean up stash
        }

        // Clean up context tracking properties
        const char* context_props[] = { 
            "original_win", 
            "original_path", "original_dirname", "original_filename" 
        };
        for (const char* p : context_props) {
            duk_del_prop_string(s_JsContext, -1, p);
        }

        duk_pop(s_JsContext);
    }

    void TriggerWidgetEvent(Widget* widget, const std::string& eventName) {
        if (!s_JsContext || !widget) return;
        
        duk_push_global_stash(s_JsContext);
        if (!duk_get_prop_string(s_JsContext, -1, "widget_objects")) {
            duk_pop_2(s_JsContext); // undefined, stash
            return;
        }
        
        std::string id = Utils::ToString(widget->GetOptions().id);
        if (!duk_get_prop_string(s_JsContext, -1, id.c_str())) {
            duk_pop_3(s_JsContext); // undefined, widget_objects, stash
            return;
        }
        
        // Check if the widget object is valid (not undefined/null)
        if (!duk_is_object(s_JsContext, -1)) {
            duk_pop_3(s_JsContext); // widget object, widget_objects, stash
            return;
        }
        
        // Save original globals to restore later
        duk_get_global_string(s_JsContext, "win");
        duk_get_global_string(s_JsContext, "system");
        duk_get_global_string(s_JsContext, "widgetWindow");
        duk_get_global_string(s_JsContext, "app");

        // Set widget-specific context for the duration of the event
        duk_dup(s_JsContext, -5); // The widget object
        duk_put_global_string(s_JsContext, "win");
        
        // Mask restricted globals
        duk_push_undefined(s_JsContext);
        duk_put_global_string(s_JsContext, "system");
        duk_push_undefined(s_JsContext);
        duk_put_global_string(s_JsContext, "widgetWindow");
        duk_push_undefined(s_JsContext);
        duk_put_global_string(s_JsContext, "app");

        // Try to get the events object - use protected call to avoid crashes
        if (duk_get_prop_string(s_JsContext, -5, "\xFF" "events")) {
            if (duk_get_prop_string(s_JsContext, -1, eventName.c_str())) {
                if (duk_is_array(s_JsContext, -1)) {
                    duk_size_t len = duk_get_length(s_JsContext, -1);
                    for (duk_size_t i = 0; i < len; i++) {
                        duk_get_prop_index(s_JsContext, -1, (duk_uarridx_t)i);
                        if (duk_is_function(s_JsContext, -1)) {
                            if (duk_pcall(s_JsContext, 0) != 0) {
                                Logging::Log(LogLevel::Error, L"Widget Event Error (%s: %S): %S", 
                                    widget->GetOptions().id.c_str(), eventName.c_str(), duk_safe_to_string(s_JsContext, -1));
                            }
                        }
                        duk_pop(s_JsContext);
                    }
                }
            }
            duk_pop(s_JsContext); // event name prop
        }
        duk_pop(s_JsContext); // events object or undefined

        // Restore original globals
        duk_put_global_string(s_JsContext, "app");
        duk_put_global_string(s_JsContext, "widgetWindow");
        duk_put_global_string(s_JsContext, "system");
        duk_put_global_string(s_JsContext, "win");
        
        duk_pop(s_JsContext); // widget object
        duk_pop_2(s_JsContext); // widget_objects, stash
    }

    void CallStoredCallback(int id) {
        if (!s_JsContext) return;
        duk_push_global_stash(s_JsContext);
        if (duk_get_prop_string(s_JsContext, -1, "__timers")) {
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
        }
        duk_pop_2(s_JsContext);
    }

    void CallHotkeyCallback(int callbackIdx) {
        if (!s_JsContext || callbackIdx < 0) return;
        duk_push_global_stash(s_JsContext);
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

    int RegisterEventCallback(duk_context* ctx, duk_idx_t idx) {
        idx = duk_normalize_index(ctx, idx); // Normalize relative index before stack changes
        if (!duk_is_function(ctx, idx)) return -1;
        
        int id = s_NextEventCallbackId++;

        // Store in global stash hidden object
        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__events")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__events");
            duk_get_prop_string(ctx, -1, "__events");
        }
        
        duk_push_int(ctx, id);
        duk_dup(ctx, idx);
        duk_put_prop(ctx, -3);
        
        duk_pop_2(ctx); // pop __events, stash

        // Logging::Log(LogLevel::Debug, L"Event callback registered: %d", id);
        return id;
    }

    void CallEventCallback(int id, Widget* contextWidget) {
        if (!s_JsContext || id < 0) return;
        
        // Logging::Log(LogLevel::Debug, L"Calling event callback: %d", id);

        // If we have a context widget, set up the environment
        if (contextWidget) {
            duk_push_global_stash(s_JsContext);
            if (duk_get_prop_string(s_JsContext, -1, "widget_objects")) {
                std::string idStr = Utils::ToString(contextWidget->GetOptions().id);
                if (duk_get_prop_string(s_JsContext, -1, idStr.c_str())) {
                    if (duk_is_object(s_JsContext, -1)) {
                        duk_get_global_string(s_JsContext, "win"); // Save original win
                        duk_dup(s_JsContext, -2); // Duplicate widget object
                        duk_put_global_string(s_JsContext, "win"); // Set new win
                        
                        // Stack: [stash, widget_objects, widget_obj, original_win]
                    } else {
                        duk_pop(s_JsContext); // Pop undefined/null
                    }
                } else {
                    duk_pop(s_JsContext); // Pop undefined
                }
            }
            duk_pop_2(s_JsContext); // Pop widget_objects, stash (Wait, we kept original_win on stack if success)
        }

        // Wait, stack management above is tricky. Let's simplify.
        bool contextSwitched = false;
        if (contextWidget) {
             duk_push_global_stash(s_JsContext);
             if (duk_get_prop_string(s_JsContext, -1, "widget_objects")) {
                 std::string idStr = Utils::ToString(contextWidget->GetOptions().id);
                 if (duk_get_prop_string(s_JsContext, -1, idStr.c_str())) {
                     // Stack: [stash, widget_objects, widget_obj]
                     duk_get_global_string(s_JsContext, "win"); // [..., original_win]
                     duk_dup(s_JsContext, -2); // [..., original_win, widget_obj]
                     duk_put_global_string(s_JsContext, "win");
                     contextSwitched = true;
                 } else {
                     duk_pop(s_JsContext);
                 }
             }
             if (!contextSwitched) {
                 duk_pop_2(s_JsContext); // pop widget_objects, stash if failed
             } else {
                 // Leave items on stack to restore later
                 // Stack: [stash, widget_objects, widget_obj, original_win]
             }
        }

        duk_push_global_stash(s_JsContext);
        if (duk_get_prop_string(s_JsContext, -1, "__events")) {
            duk_push_int(s_JsContext, id);
            if (duk_get_prop(s_JsContext, -2)) {
                if (duk_is_function(s_JsContext, -1)) {
                    if (duk_pcall(s_JsContext, 0) != 0) {
                        Logging::Log(LogLevel::Error, L"Event callback error: %S", duk_safe_to_string(s_JsContext, -1));
                    }
                } else {
                    Logging::Log(LogLevel::Error, L"Event callback %d is not a function", id);
                }
                duk_pop(s_JsContext);
            } else {
                Logging::Log(LogLevel::Error, L"Event callback %d not found", id);
                duk_pop(s_JsContext);
            }
        }
        duk_pop_2(s_JsContext);

        if (contextSwitched) {
            // Restore win
            duk_put_global_string(s_JsContext, "win"); // Pops original_win
            duk_pop_3(s_JsContext); // widget_obj, widget_objects, stash
        }
    }
}
