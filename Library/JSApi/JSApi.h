/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once
#include <string>
#include <Windows.h>
#include "duktape/duktape.h"
#include "novadesk_addon.h"

class Widget;

namespace JSApi {

    void ExecuteScript(const std::wstring& script);
    void ExecuteWidgetScript(Widget* widget);
    
    void InitializeJavaScriptAPI(duk_context* ctx);
    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath = L"");
    void ReloadScripts(duk_context* ctx);
    void Reload();

    // Event handlers for the main message loop
    void OnTimer(UINT_PTR id);
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void SetMessageWindow(HWND hWnd);
    HWND GetMessageWindow();

    const NovadeskHostAPI* GetHostAPI();

    static const UINT WM_NOVADESK_DISPATCH = WM_USER + 101;
}
