/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSCommon.h"

namespace JSApi {
    duk_context* s_JsContext = nullptr;
    std::wstring s_CurrentScriptPath = L"";
    int s_NextTempId = 1;
    int s_NextTimerId = 1;
    int s_NextImmId = 1;
}
