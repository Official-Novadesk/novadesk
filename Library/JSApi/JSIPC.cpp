/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSIPC.h"
#include "../Logging.h"
#include "../Utils.h"
#include <cstring>

namespace JSApi {

    // Helper to get the widget ID or context identifier for IPC listener storage.
    // Returns the widget ID for widget contexts, or "main" for main context.
    static std::string GetCurrentContextListenersKey(duk_context* ctx) {
        // If we are currently executing a widget script, 'original_win' will be in the stash.
        duk_push_global_stash(ctx);
        if (duk_has_prop_string(ctx, -1, "original_win")) {
            duk_pop(ctx);
            
            // Get the widget ID from the global 'win' object
            duk_get_global_string(ctx, "win");
            if (duk_is_object(ctx, -1) && duk_get_prop_string(ctx, -1, "\xFF" "id")) {
                if (duk_is_string(ctx, -1)) {
                    std::string widgetId = duk_get_string(ctx, -1);
                    duk_pop_2(ctx); // Pop id and win
                    return widgetId;
                }
                duk_pop(ctx); // Pop id
            }
            duk_pop(ctx); // Pop win
        } else {
            duk_pop(ctx);
        }

        // Also check if the global 'win' object is currently a widget.
        // This helps identify asynchronous callbacks if the engine leaves 'win' breadcrumbs.
        duk_get_global_string(ctx, "win");
        if (duk_is_object(ctx, -1) && duk_has_prop_string(ctx, -1, "\xFF" "widgetPtr")) {
            if (duk_get_prop_string(ctx, -1, "\xFF" "id")) {
                if (duk_is_string(ctx, -1)) {
                    std::string widgetId = duk_get_string(ctx, -1);
                    duk_pop_2(ctx); // Pop id and win
                    return widgetId;
                }
                duk_pop(ctx); // Pop id
            }
        }
        duk_pop(ctx); // Pop win

        return "main";
    }

    static std::string GetTargetContextListenersKey(duk_context* ctx) {
        std::string current = GetCurrentContextListenersKey(ctx);
        // If current is a widget ID, target is main; if main, target is all widgets
        return (current == "main") ? "ui" : "main";
    }

    duk_ret_t js_ipc_on(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        std::string contextKey = GetCurrentContextListenersKey(ctx);

        duk_push_global_stash(ctx);
        
        // For widget contexts, store under ui_ipc_listeners[widgetId][channel]
        // For main context, store under main_ipc_listeners[channel]
        if (contextKey != "main") {
            // Widget context - use ui_ipc_listeners
            if (!duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) {
                duk_pop(ctx);
                duk_push_object(ctx);
                duk_dup(ctx, -1);
                duk_put_prop_string(ctx, -3, "ui_ipc_listeners");
            }
            
            // Get or create object for this widget ID
            if (!duk_get_prop_string(ctx, -1, contextKey.c_str())) {
                duk_pop(ctx);
                duk_push_object(ctx);
                duk_dup(ctx, -1);
                duk_put_prop_string(ctx, -3, contextKey.c_str());
            }
        } else {
            // Main context - use main_ipc_listeners
            if (!duk_get_prop_string(ctx, -1, "main_ipc_listeners")) {
                duk_pop(ctx);
                duk_push_object(ctx);
                duk_dup(ctx, -1);
                duk_put_prop_string(ctx, -3, "main_ipc_listeners");
            }
        }

        // Get or create the array for this specific channel
        if (!duk_get_prop_string(ctx, -1, channel)) {
            duk_pop(ctx);
            duk_push_array(ctx);
            duk_dup(ctx, -1);
            duk_put_prop_string(ctx, -3, channel);
        }
        
        // Add the callback to the array
        duk_dup(ctx, 1);
        duk_put_prop_index(ctx, -2, (duk_uarridx_t)duk_get_length(ctx, -2));
        
        duk_pop_3(ctx); // array, listeners object, stash
        return 0;
    }

    duk_ret_t js_ipc_send(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        std::string targetKey = GetTargetContextListenersKey(ctx);

        duk_push_global_stash(ctx);
        
        if (targetKey == "ui") {
            // Sending from main to all widgets - iterate through all widget IDs
            if (duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) {
                if (duk_is_object(ctx, -1)) {
                    // Enumerate all widget IDs
                    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
                    while (duk_next(ctx, -1, 1)) {
                        // Stack: stash, ui_ipc_listeners, enum, key, value(widget's listeners)
                        if (duk_is_object(ctx, -1) && duk_get_prop_string(ctx, -1, channel)) {
                            if (duk_is_array(ctx, -1)) {
                                duk_size_t len = duk_get_length(ctx, -1);
                                for (duk_size_t i = 0; i < len; i++) {
                                    duk_get_prop_index(ctx, -1, (duk_uarridx_t)i);
                                    if (duk_is_function(ctx, -1)) {
                                        duk_dup(ctx, 1); // Pass data arg
                                        if (duk_pcall(ctx, 1) != 0) {
                                            Logging::Log(LogLevel::Error, L"IPC Broadcast Error (%S) index %d: %S", channel, (int)i, duk_safe_to_string(ctx, -1));
                                        }
                                    }
                                    duk_pop(ctx); // result
                                }
                            }
                            duk_pop(ctx); // channel prop
                        } else {
                            duk_pop(ctx); // channel prop (undefined)
                        }
                        duk_pop_2(ctx); // key, value
                    }
                    duk_pop(ctx); // enum
                }
            }
            duk_pop(ctx); // ui_ipc_listeners
        } else {
            // Sending from widget to main - use main_ipc_listeners
            if (duk_get_prop_string(ctx, -1, "main_ipc_listeners")) {
                if (duk_get_prop_string(ctx, -1, channel)) {
                    if (duk_is_array(ctx, -1)) {
                        duk_size_t len = duk_get_length(ctx, -1);
                        for (duk_size_t i = 0; i < len; i++) {
                            duk_get_prop_index(ctx, -1, (duk_uarridx_t)i);
                            if (duk_is_function(ctx, -1)) {
                                duk_dup(ctx, 1); // Pass data arg
                                if (duk_pcall(ctx, 1) != 0) {
                                    Logging::Log(LogLevel::Error, L"IPC Broadcast Error (%S) index %d: %S", channel, (int)i, duk_safe_to_string(ctx, -1));
                                }
                            }
                            duk_pop(ctx); // result
                        }
                    } else if (duk_is_function(ctx, -1)) {
                        // Fallback for single functions (migration safety)
                        duk_dup(ctx, 1);
                        if (duk_pcall(ctx, 1) != 0) {
                            Logging::Log(LogLevel::Error, L"IPC Single Error (%S): %S", channel, duk_safe_to_string(ctx, -1));
                        }
                    }
                }
                duk_pop(ctx); // channel prop
            }
            duk_pop(ctx); // main_ipc_listeners
        }
        
        duk_pop(ctx); // stash
        return 0;
    }

    void ClearIPCListeners(duk_context* ctx, const std::string& widgetId) {
        duk_push_global_stash(ctx);
        
        // Get ui_ipc_listeners object
        if (duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) {
            // Delete the listeners for this specific widget ID
            if (duk_has_prop_string(ctx, -1, widgetId.c_str())) {
                duk_del_prop_string(ctx, -1, widgetId.c_str());
                Logging::Log(LogLevel::Debug, L"Cleared IPC listeners for widget: %S", widgetId.c_str());
            }
        }
        
        duk_pop_2(ctx); // Pop ui_ipc_listeners and stash
    }

    void BindIPCMethods(duk_context* ctx) {
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_ipc_on, 2);
        duk_put_prop_string(ctx, -2, "on");
        duk_push_c_function(ctx, js_ipc_send, 2);
        duk_put_prop_string(ctx, -2, "send");
        duk_put_global_string(ctx, "ipc");
    }
}
