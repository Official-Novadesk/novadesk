/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSCommon.h"
#include "../Widget.h"

namespace JSApi {
    duk_context* s_JsContext = nullptr;
    std::wstring s_CurrentScriptPath = L"";
    int s_NextTempId = 1;
    int s_NextTimerId = 1;
    int s_NextImmId = 1;

    Widget* GetWidgetFromThis(duk_context* ctx) {
        if (!ctx) return nullptr;
        duk_push_this(ctx);
        if (duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr")) {
            Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
            duk_pop_2(ctx);
            if (Widget::IsValid(widget)) return widget;
            return nullptr;
        }
        duk_pop(ctx);
        return nullptr;
    }

    bool TryGetWidgetIdFromObject(duk_context* ctx, duk_idx_t idx, std::string& outId) {
        if (!ctx) return false;
        duk_idx_t objIdx = duk_normalize_index(ctx, idx);
        if (!duk_is_object(ctx, objIdx)) return false;

        bool found = false;
        if (duk_get_prop_string(ctx, objIdx, "\xFF" "id")) {
            if (duk_is_string(ctx, -1)) {
                outId = duk_get_string(ctx, -1);
                found = true;
            }
        }
        duk_pop(ctx);
        return found;
    }

    bool WrapCallbackWithDirContext(duk_context* ctx, duk_idx_t fnIndex, const char* overrideDir) {
        if (!ctx) return false;
        fnIndex = duk_normalize_index(ctx, fnIndex);
        if (!duk_is_function(ctx, fnIndex)) return false;

        std::string dirname;
        if (overrideDir && *overrideDir) {
            dirname = overrideDir;
        } else {
            duk_get_global_string(ctx, "__dirname");
            if (!duk_is_string(ctx, -1)) {
                duk_pop(ctx);
                return false;
            }
            dirname = duk_get_string(ctx, -1);
            duk_pop(ctx);
            if (dirname.empty()) return false;
        }

        if (duk_peval_string(ctx,
            "(function(fn, dirname) {"
            "  return function() {"
            "    var oldDir = __dirname;"
            "    __dirname = dirname;"
            "    try { return fn.apply(this, arguments); }"
            "    finally { __dirname = oldDir; }"
            "  };"
            "})") != 0) {
            duk_pop(ctx);
            return false;
        }

        duk_dup(ctx, fnIndex);
        duk_push_string(ctx, dirname.c_str());
        if (duk_pcall(ctx, 2) != 0) {
            duk_pop(ctx);
            return false;
        }
        return true;
    }

    void BindMethods(duk_context* ctx, const JsBinding* bindings, size_t count) {
        if (!ctx || !bindings) return;
        for (size_t i = 0; i < count; ++i) {
            BindFunction(ctx, bindings[i].name, bindings[i].fn, bindings[i].nargs);
        }
    }
}
