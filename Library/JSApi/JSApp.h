/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    // App API
    duk_ret_t js_app_get_path(duk_context* ctx);
    duk_ret_t js_app_get_version(duk_context* ctx);
    duk_ret_t js_app_get_name(duk_context* ctx);
    duk_ret_t js_process_cwd(duk_context* ctx);

    void BindAppMethods(duk_context* ctx);
    void BindProcessMethods(duk_context* ctx);
}
