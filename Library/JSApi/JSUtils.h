/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    // Logging API
    duk_ret_t js_log(duk_context* ctx);
    duk_ret_t js_error(duk_context* ctx);
    duk_ret_t js_debug(duk_context* ctx);

    // Timer API
    duk_ret_t js_set_timer(duk_context* ctx);
    duk_ret_t js_clear_timer(duk_context* ctx);
    duk_ret_t js_set_immediate(duk_context* ctx);

    // Script API
    duk_ret_t js_include(duk_context* ctx);
    duk_ret_t js_novadesk_saveLogToFile(duk_context* ctx);
    duk_ret_t js_novadesk_enableDebugging(duk_context* ctx);
    duk_ret_t js_novadesk_disableLogging(duk_context* ctx);
    duk_ret_t js_novadesk_hideTrayIcon(duk_context* ctx);
    duk_ret_t js_novadesk_refresh(duk_context* ctx);

    // Binding functions
    void BindConsoleMethods(duk_context* ctx);
    void BindNovadeskAppMethods(duk_context* ctx);
}
