/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    duk_ret_t js_ipc_on(duk_context* ctx);
    duk_ret_t js_ipc_send(duk_context* ctx);

    void ClearIPCListeners(duk_context* ctx, const std::string& widgetId);
    void DispatchPendingIPC(duk_context* ctx);
    void BindIPCMethods(duk_context* ctx);
}
