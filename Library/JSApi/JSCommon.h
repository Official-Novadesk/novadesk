/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "duktape/duktape.h"

class Widget;
class Element;

namespace JSApi {
    struct MouseEventData;

    // Internal state accessible by modules
    extern duk_context* s_JsContext;
    extern std::wstring s_CurrentScriptPath;
    extern int s_NextTempId;
    extern int s_NextTimerId;
    extern int s_NextImmId;

    // Helper functions
    void ExecuteScript(const std::wstring& script);
    void ExecuteWidgetScript(Widget* widget);
    void TriggerWidgetEvent(Widget* widget, const std::string& eventName, const MouseEventData* mouseEvent = nullptr);
    void CallStoredCallback(int id);
    void CallHotkeyCallback(int callbackIdx);
    void BindWidgetControlMethods(duk_context* ctx);
    void BindWidgetUIMethods(duk_context* ctx);
    void ReloadScripts(duk_context* ctx);

    Widget* GetWidgetFromThis(duk_context* ctx);
    bool TryGetWidgetIdFromObject(duk_context* ctx, duk_idx_t idx, std::string& outId);
    bool WrapCallbackWithDirContext(duk_context* ctx, duk_idx_t fnIndex, const char* overrideDir = nullptr);

    inline void BindFunction(duk_context* ctx, const char* name, duk_c_function fn, duk_idx_t nargs) {
        duk_push_c_function(ctx, fn, nargs);
        duk_put_prop_string(ctx, -2, name);
    }

    struct JsBinding {
        const char* name;
        duk_c_function fn;
        duk_idx_t nargs;
    };

    void BindMethods(duk_context* ctx, const JsBinding* bindings, size_t count);
}
