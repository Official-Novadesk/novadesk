/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSContextMenu.h"
#include "JSUtils.h"
#include "../Widget.h"
#include "../Utils.h"
#include "../Logging.h"

namespace JSApi {

    static void ParseMenu(duk_context* ctx, int arrIdx, std::vector<MenuItem>& outItems, duk_uarridx_t widgetId) {
        duk_size_t count = duk_get_length(ctx, arrIdx);
        for (duk_size_t i = 0; i < count; i++) {
            if (duk_get_prop_index(ctx, arrIdx, (duk_uarridx_t)i)) {
                if (duk_is_object(ctx, -1)) {
                    MenuItem item;
                    item.id = 0;
                    item.isSeparator = false;
                    item.checked = false;

                    if (duk_get_prop_string(ctx, -1, "type")) {
                        std::string type = duk_safe_to_string(ctx, -1);
                        if (type == "separator") item.isSeparator = true;
                    }
                    duk_pop(ctx);

                    if (!item.isSeparator) {
                        if (duk_get_prop_string(ctx, -1, "text")) {
                            item.text = Utils::ToWString(duk_safe_to_string(ctx, -1));
                        }
                        duk_pop(ctx);

                        if (duk_get_prop_string(ctx, -1, "checked")) {
                            item.checked = duk_get_boolean(ctx, -1);
                        }
                        duk_pop(ctx);

                        if (duk_get_prop_string(ctx, -1, "action")) {
                            if (duk_is_function(ctx, -1)) {
                                item.id = 2000 + s_NextTempId++;
                                
                                // Store callback in widget object
                                duk_push_this(ctx);
                                if (!duk_get_prop_string(ctx, -1, "__contextCallbacks")) {
                                    duk_pop(ctx);
                                    duk_push_object(ctx);
                                    duk_put_prop_string(ctx, -2, "__contextCallbacks");
                                    duk_get_prop_string(ctx, -1, "__contextCallbacks");
                                }
                                duk_dup(ctx, -3); // function
                                duk_put_prop_index(ctx, -2, (duk_uarridx_t)item.id);
                                duk_pop_2(ctx); // __contextCallbacks and this
                            }
                        }
                        duk_pop(ctx);

                        if (duk_get_prop_string(ctx, -1, "items")) {
                            if (duk_is_array(ctx, -1)) {
                                ParseMenu(ctx, duk_get_top_index(ctx), item.children, widgetId);
                            }
                        }
                        duk_pop(ctx);
                    }
                    outItems.push_back(item);
                }
            }
            duk_pop(ctx);
        }
    }

    duk_ret_t js_widget_set_context_menu(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_array(ctx, 0)) return DUK_RET_ERROR;

        // Clear old callbacks
        duk_push_this(ctx);
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "__contextCallbacks");
        duk_pop(ctx);

        std::vector<MenuItem> menu;
        ParseMenu(ctx, 0, menu, 0);
        widget->SetContextMenu(menu);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_clear_context_menu(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;
        widget->ClearContextMenu();

        // Clear callbacks
        duk_push_this(ctx);
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "__contextCallbacks");
        duk_pop(ctx);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_disable_context_menu(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;
        bool disable = true;
        if (duk_get_top(ctx) > 0) disable = duk_get_boolean(ctx, 0);
        widget->SetContextMenuDisabled(disable);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_show_default_context_menu_items(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;
        if (duk_get_top(ctx) > 0) {
            bool show = duk_get_boolean(ctx, 0);
            widget->SetShowDefaultContextMenuItems(show);
        }
        duk_push_this(ctx);
        return 1;
    }

    void BindWidgetContextMenuMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_widget_set_context_menu, 1);
        duk_put_prop_string(ctx, -2, "setContextMenu");
        duk_push_c_function(ctx, js_widget_clear_context_menu, 0);
        duk_put_prop_string(ctx, -2, "clearContextMenu");
        duk_push_c_function(ctx, js_widget_disable_context_menu, 1);
        duk_put_prop_string(ctx, -2, "disableContextMenu");
        duk_push_c_function(ctx, js_widget_show_default_context_menu_items, 1);
        duk_put_prop_string(ctx, -2, "showDefaultContextMenuItems");
    }

    void OnWidgetContextCommand(const std::wstring& widgetId, int commandId) {
        duk_context* ctx = s_JsContext;
        if (!ctx) return;

        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "widget_objects")) {
            if (duk_get_prop_string(ctx, -1, Utils::ToString(widgetId).c_str())) {
                if (duk_get_prop_string(ctx, -1, "__contextCallbacks")) {
                    if (duk_get_prop_index(ctx, -1, (duk_uarridx_t)commandId)) {
                        if (duk_is_function(ctx, -1)) {
                            if (duk_pcall(ctx, 0) != 0) {
                                Logging::Log(LogLevel::Error, L"Widget context callback failed: %S", duk_safe_to_string(ctx, -1));
                            }
                        }
                        duk_pop(ctx);
                    }
                    duk_pop(ctx); // __contextCallbacks
                }
                duk_pop(ctx); // widget object
            } else {
                duk_pop(ctx); // undefined
            }
        }
        duk_pop_2(ctx); // widget_objects and stash
    }
}
