/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSContextMenu.h"
#include "../Widget.h"
#include "../Utils.h"

namespace JSApi {

    duk_ret_t js_widget_add_context_menu_item(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || duk_get_top(ctx) < 2) return DUK_RET_ERROR;
        
        std::wstring label = Utils::ToWString(duk_safe_to_string(ctx, 0));
        std::wstring action = Utils::ToWString(duk_safe_to_string(ctx, 1));
        widget->AddContextMenuItem(label, action);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_remove_context_menu_item(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || duk_get_top(ctx) < 1) return DUK_RET_ERROR;
        
        std::wstring label = Utils::ToWString(duk_safe_to_string(ctx, 0));
        widget->RemoveContextMenuItem(label);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_clear_context_menu(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;
        widget->ClearContextMenuItems();
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
}
