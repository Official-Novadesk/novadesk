/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
 #include "JSIPC.h"
 #include "JSWidget.h"
 #include "JSApi.h"
 #include "../Logging.h"
 #include "../Utils.h"
 #include "../TimerManager.h"
 #include <cstring>
 #include <vector>
 
 extern std::vector<Widget*> widgets;
 
 namespace JSApi {
     static const UINT WM_DISPATCH_IPC = WM_USER + 102;
 
     static std::string GetCurrentContextListenersKey(duk_context* ctx) {
         duk_push_global_stash(ctx);
         if (duk_has_prop_string(ctx, -1, "original_win")) {
             duk_pop(ctx);
             duk_get_global_string(ctx, "win");
             if (duk_is_object(ctx, -1) && duk_get_prop_string(ctx, -1, "\xFF" "id")) {
                 if (duk_is_string(ctx, -1)) {
                     std::string widgetId = duk_get_string(ctx, -1);
                     duk_pop_2(ctx); 
                     return widgetId;
                 }
                 duk_pop(ctx); 
             }
             duk_pop(ctx); 
         } else {
             duk_pop(ctx);
         }
 
         duk_get_global_string(ctx, "win");
         if (duk_is_object(ctx, -1) && duk_has_prop_string(ctx, -1, "\xFF" "widgetPtr")) {
             if (duk_get_prop_string(ctx, -1, "\xFF" "id")) {
                 if (duk_is_string(ctx, -1)) {
                     std::string widgetId = duk_get_string(ctx, -1);
                     duk_pop_2(ctx); 
                     return widgetId;
                 }
                 duk_pop(ctx); 
             }
         }
         duk_pop(ctx); 
 
         return "main";
     }
 
     static std::string GetTargetContextListenersKey(duk_context* ctx) {
         std::string current = GetCurrentContextListenersKey(ctx);
         return (current == "main") ? "ui" : "main";
     }
 
     duk_ret_t js_ipc_on(duk_context* ctx) {
         if (!duk_is_string(ctx, 0) || !duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;
         const char* channel = duk_get_string(ctx, 0);
         std::string contextKey = GetCurrentContextListenersKey(ctx);
 
         duk_push_global_stash(ctx);
         if (contextKey != "main") {
             if (!duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) {
                 duk_pop(ctx);
                 duk_push_object(ctx);
                 duk_dup(ctx, -1);
                 duk_put_prop_string(ctx, -3, "ui_ipc_listeners");
             }
             if (!duk_get_prop_string(ctx, -1, contextKey.c_str())) {
                 duk_pop(ctx);
                 duk_push_object(ctx);
                 duk_dup(ctx, -1);
                 duk_put_prop_string(ctx, -3, contextKey.c_str());
             }
         } else {
             if (!duk_get_prop_string(ctx, -1, "main_ipc_listeners")) {
                 duk_pop(ctx);
                 duk_push_object(ctx);
                 duk_dup(ctx, -1);
                 duk_put_prop_string(ctx, -3, "main_ipc_listeners");
             }
         }
 
         if (!duk_get_prop_string(ctx, -1, channel)) {
             duk_pop(ctx);
             duk_push_array(ctx);
             duk_dup(ctx, -1);
             duk_put_prop_string(ctx, -3, channel);
         }
         
         duk_dup(ctx, 1);
         duk_put_prop_index(ctx, -2, (duk_uarridx_t)duk_get_length(ctx, -2));
         
         duk_pop_3(ctx); 
         return 0;
     }
 
     duk_ret_t js_ipc_send(duk_context* ctx) {
         duk_idx_t nargs = duk_get_top(ctx);
         if (nargs < 1 || !duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
 
         const char* channel = duk_get_string(ctx, 0);
         std::string targetKey = GetTargetContextListenersKey(ctx);
 
         duk_push_global_stash(ctx);
         if (!duk_get_prop_string(ctx, -1, "pending_ipc")) {
             duk_pop(ctx);
             duk_push_array(ctx);
             duk_dup(ctx, -1);
             duk_put_prop_string(ctx, -3, "pending_ipc");
         }
 
         duk_push_object(ctx);
         duk_push_string(ctx, channel);
         duk_put_prop_string(ctx, -2, "channel");
         duk_push_string(ctx, targetKey.c_str());
         duk_put_prop_string(ctx, -2, "target");
         
         if (nargs > 1) {
             duk_dup(ctx, 1); 
             duk_put_prop_string(ctx, -2, "data");
         }
 
         duk_put_prop_index(ctx, -2, (duk_uarridx_t)duk_get_length(ctx, -2));
         duk_pop_2(ctx); 
 
         PostMessage(TimerManager::GetWindow(), WM_DISPATCH_IPC, 0, 0);
         return 0;
     }
 
     void DispatchPendingIPC(duk_context* ctx) {
         if (!ctx) return;
         
         duk_idx_t top_before = duk_get_top(ctx);
         
         duk_push_global_stash(ctx); // [top+0] stash
         if (!duk_get_prop_string(ctx, -1, "pending_ipc")) { // [top+1] original_array
             duk_pop_2(ctx);
             return;
         }
 
         duk_size_t pendingCount = duk_get_length(ctx, -1);
         if (pendingCount == 0) {
             duk_pop_2(ctx);
             return;
         }
 
         // Create a local copy to avoid mutation issues
         duk_push_array(ctx); // [top+2] local_copy
         for (duk_size_t i = 0; i < pendingCount; i++) {
             duk_get_prop_index(ctx, -2, (duk_uarridx_t)i); 
             duk_put_prop_index(ctx, -2, (duk_uarridx_t)i); 
         }
         
         // Clear stash.pending_ipc immediately
         duk_push_array(ctx); // [top+3] new_empty
         duk_put_prop_string(ctx, -4, "pending_ipc"); // stash.pending_ipc = empty. empty is popped.
 
         // stack: [top+0] stash, [top+1] original_array, [top+2] local_copy
 
         for (duk_size_t i = 0; i < pendingCount; i++) {
             duk_get_prop_index(ctx, -1, (duk_uarridx_t)i); // [top+3] item object
             
             duk_get_prop_string(ctx, -1, "channel"); // [top+4]
             const char* channel = duk_get_string(ctx, -1);
             duk_get_prop_string(ctx, -2, "target"); // [top+5]
             const char* targetKey = duk_get_string(ctx, -1);
             duk_get_prop_string(ctx, -3, "data"); // [top+6] data payload
             
             duk_push_global_stash(ctx); // [top+7] stash
             if (strcmp(targetKey, "ui") == 0) {
                 if (duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) { // [top+8] ui_ipc_listeners
                     if (duk_is_object(ctx, -1)) {
                         duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY); // [top+9] enum
                         while (duk_next(ctx, -1, 1)) { // [top+10] key, [top+11] widget_listeners
                             if (duk_is_object(ctx, -1) && duk_get_prop_string(ctx, -1, channel)) { // [top+12] chan_arr
                                 if (duk_is_array(ctx, -1)) {
                                     duk_size_t len = duk_get_length(ctx, -1);
                                     for (duk_size_t j = 0; j < len; j++) {
                                         duk_get_prop_index(ctx, -1, (duk_uarridx_t)j); // [top+13] callback
                                         if (duk_is_function(ctx, -1)) {
                                             duk_dup(ctx, -8); // Points to data at [top+6]
                                             if (duk_pcall(ctx, 1) != 0) {
                                                 Logging::Log(LogLevel::Error, L"IPC Error (UI:%S): %S", channel, duk_safe_to_string(ctx, -1));
                                             }
                                             duk_pop(ctx); 
                                         } else {
                                             duk_pop(ctx); 
                                         }
                                     }
                                 }
                                 duk_pop(ctx); // chan_arr
                             } else {
                                 duk_pop(ctx); // undefined/null from get_prop
                             }
                             duk_pop_2(ctx); // key, widget_listeners
                         }
                         duk_pop(ctx); // enum
                     }
                 }
                 duk_pop_2(ctx); // ui_ipc_listeners, stash
             } else {
                 if (duk_get_prop_string(ctx, -1, "main_ipc_listeners")) { // [top+8] main_ipc_listeners
                     if (duk_is_object(ctx, -1) && duk_get_prop_string(ctx, -1, channel)) { // [top+9] chan_arr
                         if (duk_is_array(ctx, -1)) {
                             duk_size_t len = duk_get_length(ctx, -1);
                             for (duk_size_t j = 0; j < len; j++) {
                                 duk_get_prop_index(ctx, -1, (duk_uarridx_t)j); // [top+10] callback
                                 if (duk_is_function(ctx, -1)) {
                                     duk_dup(ctx, -5); // Points to data at [top+6]
                                     if (duk_pcall(ctx, 1) != 0) {
                                         Logging::Log(LogLevel::Error, L"IPC Error (Main:%S): %S", channel, duk_safe_to_string(ctx, -1));
                                     }
                                     duk_pop(ctx); 
                                 } else {
                                     duk_pop(ctx);
                                 }
                             }
                         }
                         duk_pop(ctx); // chan_arr
                     } else {
                         duk_pop(ctx); // undefined
                     }
                 }
                 duk_pop_2(ctx); // main_ipc_listeners, stash
             }
 
             duk_pop_n(ctx, 4); // data, target, channel, item object
         }
 
         duk_pop_3(ctx); // local_copy, original_array, stash
         
         if (duk_get_top(ctx) != top_before) {
             Logging::Log(LogLevel::Error, L"IPC Stack Unbalanced: %d -> %d", (int)top_before, (int)duk_get_top(ctx));
             duk_set_top(ctx, top_before);
         }
     }
 
     void ClearIPCListeners(duk_context* ctx, const std::string& widgetId) {
         duk_push_global_stash(ctx);
         if (duk_get_prop_string(ctx, -1, "ui_ipc_listeners")) {
             if (duk_has_prop_string(ctx, -1, widgetId.c_str())) {
                 duk_del_prop_string(ctx, -1, widgetId.c_str());
             }
         }
         duk_pop_2(ctx);
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
