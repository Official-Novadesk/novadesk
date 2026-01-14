/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    duk_ret_t js_widget_set_context_menu(duk_context* ctx);
    duk_ret_t js_widget_clear_context_menu(duk_context* ctx);
    duk_ret_t js_widget_disable_context_menu(duk_context* ctx);
    duk_ret_t js_widget_show_default_context_menu_items(duk_context* ctx);

    void BindWidgetContextMenuMethods(duk_context* ctx);
    void OnWidgetContextCommand(const std::wstring& widgetId, int commandId);
}
