/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSIPC.h"
#include "../Logging.h"
#include "../Utils.h"

namespace JSApi {

    // Helper to get the correct listeners object based on context
    // We'll use "main_ipc_listeners" and "ui_ipc_listeners" in the global stash
    static const char* GetListenersKey(duk_context* ctx) {
        duk_push_global_stash(ctx);
        bool isUI = duk_has_prop_string(ctx, -1, "original_win");
        duk_pop(ctx);
        return isUI ? "ui_ipc_listeners" : "main_ipc_listeners";
    }

    static const char* GetTargetKey(duk_context* ctx) {
        duk_push_global_stash(ctx);
        bool isUI = duk_has_prop_string(ctx, -1, "original_win");
        duk_pop(ctx);
        return isUI ? "main_ipc_listeners" : "ui_ipc_listeners";
    }

    duk_ret_t js_ipc_on(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        const char* key = GetListenersKey(ctx);

        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, key)) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_dup(ctx, -1);
            duk_put_prop_string(ctx, -3, key);
        }

        // listeners[channel] = callback
        duk_dup(ctx, 1);
        duk_put_prop_string(ctx, -2, channel);
        
        duk_pop_2(ctx); // listeners, stash
        return 0;
    }

    duk_ret_t js_ipc_send(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        const char* targetKey = GetTargetKey(ctx);

        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, targetKey)) {
            if (duk_get_prop_string(ctx, -1, channel)) {
                if (duk_is_function(ctx, -1)) {
                    // Call the listener with the data (arg 1)
                    duk_dup(ctx, 1); 
                    if (duk_pcall(ctx, 1) != 0) {
                        Logging::Log(LogLevel::Error, L"IPC Callback Error (%S): %S", channel, duk_safe_to_string(ctx, -1));
                    }
                }
                duk_pop(ctx); // listener or error
            } else {
                duk_pop(ctx); // undefined prop
            }
        }
        duk_pop_2(ctx); // target object, stash
        return 0;
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
