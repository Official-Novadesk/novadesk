/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    // Path API
    duk_ret_t js_path_join(duk_context* ctx);
    duk_ret_t js_path_resolve(duk_context* ctx);
    duk_ret_t js_path_dirname(duk_context* ctx);
    duk_ret_t js_path_basename(duk_context* ctx);
    duk_ret_t js_path_extname(duk_context* ctx);
    duk_ret_t js_path_parse(duk_context* ctx);
    duk_ret_t js_path_format(duk_context* ctx);
    duk_ret_t js_path_is_absolute(duk_context* ctx);
    duk_ret_t js_path_normalize(duk_context* ctx);
    duk_ret_t js_path_relative(duk_context* ctx);

    void BindPathMethods(duk_context* ctx);
}
