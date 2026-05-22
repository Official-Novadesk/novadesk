/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include <string>
#include "quickjs.h"

class Widget;

namespace novadesk::scripting::quickjs
{
    void SetWidgetUiDebug(bool debug);
    JSClassID EnsureWidgetWindowClass(JSContext *ctx);
    JSValue JsWidgetWindowCtor(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv);
    bool ExecuteWidgetUiScript(JSContext *ctx, Widget *widget, const std::wstring &scriptPath);
}
