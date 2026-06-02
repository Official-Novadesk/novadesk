/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>

#include <d2d1.h>
#include <windows.h>

#include "quickjs.h"

#include "../../../render/Element.h"
#include "../../../render/TextElement.h"
#include "PropertyParserTypes.h"

namespace PropertyParser
{
    namespace Js
    {
        std::vector<std::wstring> SplitByComma(const std::wstring &s);
        JSValue GetGlobalProperty(JSContext *ctx, const char *key);

        std::wstring GetStringProp(JSContext *ctx, JSValueConst obj, const char *key);
        bool GetIntProp(JSContext *ctx, JSValueConst obj, const char *key, int &out);
        bool GetFloatProp(JSContext *ctx, JSValueConst obj, const char *key, float &out);
        bool GetBoolProp(JSContext *ctx, JSValueConst obj, const char *key, bool &out);
        bool GetFloatArrayProp(JSContext *ctx, JSValueConst obj, const char *key, std::vector<float> &out, int minSize);
        bool GetEventCallbackProp(JSContext *ctx, JSValueConst obj, const char *key, int &outId);

        void ParseGradientOrColor(const std::wstring &v, COLORREF &color, BYTE &alpha, GradientInfo &gradient, bool &hasColor);
        D2D1_COMBINE_MODE ParseCombineMode(const std::wstring &s);
        bool GetFloatArrayPropAllowEmpty(JSContext *ctx, JSValueConst obj, const char *key, std::vector<float> &out);

        void ParsePrefixedGeneralImageOptions(
            JSContext *ctx,
            JSValueConst obj,
            const std::string &prefix,
            GeneralImageOptions &out);
        void ParseTextShadows(JSContext *ctx, JSValueConst obj, std::vector<TextShadow> &shadows);
    } // namespace Js
} // namespace PropertyParser
