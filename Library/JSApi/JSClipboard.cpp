/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSClipboard.h"
#include "../Utils.h"
#include <windows.h>

namespace JSApi {

    duk_ret_t js_system_getClipboardText(duk_context* ctx) {
        if (!OpenClipboard(NULL)) return 0;

        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pszText = (wchar_t*)GlobalLock(hData);
            if (pszText) {
                duk_push_string(ctx, Utils::ToString(pszText).c_str());
                GlobalUnlock(hData);
                CloseClipboard();
                return 1;
            }
        }

        CloseClipboard();
        return 0;
    }

    duk_ret_t js_system_setClipboardText(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        std::wstring text = Utils::ToWString(duk_get_string(ctx, 0));

        if (!OpenClipboard(NULL)) {
            duk_push_boolean(ctx, false);
            return 1;
        }

        EmptyClipboard();

        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            void* pData = GlobalLock(hGlobal);
            if (pData) {
                memcpy(pData, text.c_str(), size);
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
                duk_push_boolean(ctx, true);
            } else {
                GlobalFree(hGlobal);
                duk_push_boolean(ctx, false);
            }
        } else {
            duk_push_boolean(ctx, false);
        }

        CloseClipboard();
        return 1;
    }

    void BindClipboardMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_system_getClipboardText, 0);
        duk_put_prop_string(ctx, -2, "getClipboardText");

        duk_push_c_function(ctx, js_system_setClipboardText, 1);
        duk_put_prop_string(ctx, -2, "setClipboardText");
    }
}
