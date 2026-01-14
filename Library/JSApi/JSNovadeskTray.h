/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    // Tray Menu API
    duk_ret_t js_novadesk_set_tray_menu(duk_context* ctx);
    duk_ret_t js_novadesk_clear_tray_menu(duk_context* ctx);
    duk_ret_t js_novadesk_show_default_tray_items(duk_context* ctx);
    duk_ret_t js_novadesk_hide_tray_icon(duk_context* ctx);
    
    void BindNovadeskTrayMethods(duk_context* ctx);
    void OnTrayCommand(int id);
}
