/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSNovadeskTray.h"
#include "JSUtils.h"
#include "../Novadesk.h"
#include "../Logging.h"
#include "../Utils.h"

namespace JSApi {

    void OnTrayCommand(int id) {
        duk_context* ctx = s_JsContext;
        if (!ctx) return;

        duk_get_global_string(ctx, "novadesk");
        if (duk_get_prop_string(ctx, -1, "__trayCallbacks")) {
            if (duk_get_prop_index(ctx, -1, (duk_uarridx_t)id)) {
                if (duk_is_function(ctx, -1)) {
                    if (duk_pcall(ctx, 0) != 0) {
                        Logging::Log(LogLevel::Error, L"Tray callback failed: %S", duk_safe_to_string(ctx, -1));
                    }
                }
                duk_pop(ctx); // pop function or undefined
            }
            duk_pop(ctx); // pop __trayCallbacks
        }
        duk_pop(ctx); // pop novadesk
    }

    // Helper for recursion
    static void ParseMenu(duk_context* ctx, int arrIdx, std::vector<MenuItem>& outItems) {
        duk_size_t count = duk_get_length(ctx, arrIdx);
        for (duk_size_t i = 0; i < count; i++) {
            if (duk_get_prop_index(ctx, arrIdx, (duk_uarridx_t)i)) {
                if (duk_is_object(ctx, -1)) {
                    MenuItem item;
                    item.id = 0;
                    item.isSeparator = false;
                    item.checked = false;

                    // Check type: "separator"
                    if (duk_get_prop_string(ctx, -1, "type")) {
                        std::string type = duk_safe_to_string(ctx, -1);
                        if (type == "separator") item.isSeparator = true;
                    }
                    duk_pop(ctx);

                    if (!item.isSeparator) {
                        // text
                        if (duk_get_prop_string(ctx, -1, "text")) {
                            item.text = Utils::ToWString(duk_safe_to_string(ctx, -1));
                        }
                        duk_pop(ctx);

                        // checked
                        if (duk_get_prop_string(ctx, -1, "checked")) {
                            item.checked = duk_get_boolean(ctx, -1);
                        }
                        duk_pop(ctx);

                        // action / onClick
                        if (duk_get_prop_string(ctx, -1, "action")) {
                            if (duk_is_function(ctx, -1)) {
                                item.id = 2000 + s_NextTempId++; // Start IDs from 2000 for safety
                                
                                // Store callback in novadesk.__trayCallbacks
                                duk_get_global_string(ctx, "novadesk");
                                if (!duk_get_prop_string(ctx, -1, "__trayCallbacks")) {
                                    duk_pop(ctx);
                                    duk_push_object(ctx);
                                    duk_put_prop_string(ctx, -2, "__trayCallbacks");
                                    duk_get_prop_string(ctx, -1, "__trayCallbacks");
                                }
                                
                                duk_dup(ctx, -3); // duplicate function
                                duk_put_prop_index(ctx, -2, (duk_uarridx_t)item.id);
                                duk_pop_2(ctx); // pop __trayCallbacks and novadesk
                            }
                        }
                        duk_pop(ctx);

                        // items (subprocess for submenus)
                        if (duk_get_prop_string(ctx, -1, "items")) {
                            if (duk_is_array(ctx, -1)) {
                                ParseMenu(ctx, duk_get_top_index(ctx), item.children);
                            }
                        }
                        duk_pop(ctx);
                    }
                    
                    outItems.push_back(item);
                }
            }
            duk_pop(ctx); // pop item object
        }
    }

    duk_ret_t js_novadesk_set_tray_menu(duk_context* ctx) {
        if (!duk_is_array(ctx, 0)) return DUK_RET_TYPE_ERROR;

        // Clear old callbacks
        duk_get_global_string(ctx, "novadesk");
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "__trayCallbacks");
        duk_pop(ctx);

        std::vector<MenuItem> menu;
        ParseMenu(ctx, 0, menu);
        
        SetTrayMenu(menu);
        return 0;
    }

    duk_ret_t js_novadesk_clear_tray_menu(duk_context* ctx) {
        ClearTrayMenu();
        
        // Clear callbacks
        duk_get_global_string(ctx, "novadesk");
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "__trayCallbacks");
        duk_pop(ctx);

        return 0;
    }

    duk_ret_t js_novadesk_show_default_tray_items(duk_context* ctx) {
        bool show = true;
        if (duk_get_top(ctx) > 0 && duk_is_boolean(ctx, 0)) {
            show = duk_get_boolean(ctx, 0);
        }
        SetShowDefaultTrayItems(show);
        return 0;
    }

    duk_ret_t js_novadesk_hide_tray_icon(duk_context* ctx) {
        bool hide = true;
        if (duk_get_top(ctx) > 0 && duk_is_boolean(ctx, 0)) {
            hide = duk_get_boolean(ctx, 0);
        }
        if (hide) HideTrayIconDynamic();
        else ShowTrayIconDynamic();
        return 0;
    }

    void BindNovadeskTrayMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_novadesk_set_tray_menu, 1);
        duk_put_prop_string(ctx, -2, "setTrayMenu");
        duk_push_c_function(ctx, js_novadesk_clear_tray_menu, 0);
        duk_put_prop_string(ctx, -2, "clearTrayMenu");
        duk_push_c_function(ctx, js_novadesk_show_default_tray_items, 1);
        duk_put_prop_string(ctx, -2, "showDefaultTrayItems");
        duk_push_c_function(ctx, js_novadesk_hide_tray_icon, 1);
        duk_put_prop_string(ctx, -2, "hideTrayIcon");
    }
}
