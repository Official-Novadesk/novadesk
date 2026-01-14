/* Copyright (C) 2026 Novadesk Project 
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
    // Internal state accessible by modules
    extern duk_context* s_JsContext;
    extern std::wstring s_CurrentScriptPath;
    extern int s_NextTempId;
    extern int s_NextTimerId;
    extern int s_NextImmId;

    // Helper functions
    void ExecuteScript(const std::wstring& script);
    void ExecuteWidgetScript(Widget* widget);
    void TriggerWidgetEvent(Widget* widget, const std::string& eventName);
    void CallStoredCallback(int id);
    void CallHotkeyCallback(int callbackIdx);
    void BindWidgetControlMethods(duk_context* ctx);
    void BindWidgetUIMethods(duk_context* ctx);
    void ReloadScripts(duk_context* ctx);
}
