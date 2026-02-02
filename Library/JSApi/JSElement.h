/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    duk_ret_t js_widget_add_image(duk_context* ctx);
    duk_ret_t js_widget_add_text(duk_context* ctx);
    duk_ret_t js_widget_add_bar(duk_context* ctx);
    duk_ret_t js_widget_add_round_line(duk_context* ctx);
    duk_ret_t js_widget_set_element_properties(duk_context* ctx);
    duk_ret_t js_widget_remove_elements(duk_context* ctx);
    duk_ret_t js_widget_get_element_property(duk_context* ctx);
    duk_ret_t js_widget_begin_update(duk_context* ctx);
    duk_ret_t js_widget_end_update(duk_context* ctx);

    void BindWidgetUIMethods(duk_context* ctx);
}
