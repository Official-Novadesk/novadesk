/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once
#include <string>
#include "duktape/duktape.h"
#include "Hotkey.h"

namespace JSApi {

    void InitializeJavaScriptAPI(duk_context* ctx);
    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath = L"");
    void ReloadScripts(duk_context* ctx);

    duk_ret_t js_log(duk_context* ctx);
    duk_ret_t js_error(duk_context* ctx);
    duk_ret_t js_debug(duk_context* ctx);

    duk_ret_t js_create_widget_window(duk_context* ctx);
    duk_ret_t js_widget_add_image(duk_context* ctx);
    duk_ret_t js_widget_add_text(duk_context* ctx);
    duk_ret_t js_widget_update_image(duk_context* ctx);
    duk_ret_t js_widget_update_text(duk_context* ctx);
    duk_ret_t js_widget_remove_content(duk_context* ctx);
    duk_ret_t js_widget_clear_content(duk_context* ctx);
    duk_ret_t js_widget_set_properties(duk_context* ctx);
    duk_ret_t js_widget_get_properties(duk_context* ctx);

    void ExecuteScript(const std::wstring& script);

    // Event handlers for the main message loop
    void OnTimer(UINT_PTR id);
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void SetMessageWindow(HWND hWnd);

    duk_ret_t js_register_hotkey(duk_context* ctx);
    duk_ret_t js_unregister_hotkey(duk_context* ctx);
}
