/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    duk_ret_t js_create_widget_window(duk_context* ctx);
    duk_ret_t js_widget_set_properties(duk_context* ctx);
    duk_ret_t js_widget_get_properties(duk_context* ctx);
    duk_ret_t js_widget_close(duk_context* ctx);
    duk_ret_t js_widget_refresh(duk_context* ctx);

    void BindWidgetControlMethods(duk_context* ctx);
}
