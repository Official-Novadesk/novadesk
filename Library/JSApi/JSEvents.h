/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

class Widget;

namespace JSApi {
    void ExecuteScript(const std::wstring& script);
    
    // Function callback support
    int RegisterEventCallback(duk_context* ctx, duk_idx_t idx);
    void CallEventCallback(int id, Widget* contextWidget = nullptr);
}
