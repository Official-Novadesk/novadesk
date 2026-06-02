/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include <string>
#include "quickjs.h"

namespace novadesk::scripting::quickjs
{
    void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId);
    void AttachWidgetWindowEventMethods(JSContext *ctx, JSValue proto);
    void InvokeWidgetContextMenuCallback(const std::wstring &widgetId, int commandId);
}
