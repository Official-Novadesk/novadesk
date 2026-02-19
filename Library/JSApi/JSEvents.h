/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

class Widget;

namespace JSApi {
    struct MouseEventData {
        int offsetX = 0;
        int offsetY = 0;
        int offsetXPercent = 0;
        int offsetYPercent = 0;
        int clientX = 0;
        int clientY = 0;
        int screenX = 0;
        int screenY = 0;
    };

    void ExecuteScript(const std::wstring& script);
    
    // Function callback support
    int RegisterEventCallback(duk_context* ctx, duk_idx_t idx);
    void CallEventCallback(int id, Widget* contextWidget = nullptr, const MouseEventData* mouseEvent = nullptr);
}
