/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/Utils.h"
#include <algorithm>
#include <string>
#include <cstring>
namespace PropertyParser
{
    using namespace Js;
    static void ParseAnimationTargetObject(
        JSContext *ctx,
        JSValueConst obj,
        bool &hasX,
        float &x,
        bool &hasY,
        float &y,
        bool &hasWidth,
        float &width,
        bool &hasHeight,
        float &height,
        bool &hasRotate,
        float &rotate,
        bool &hasFontSize,
        float &fontSize,
        bool &hasFontWeight,
        float &fontWeight,
        bool &hasLetterSpacing,
        float &letterSpacing,
        bool &hasFontColor,
        float &fontColorR,
        float &fontColorG,
        float &fontColorB,
        float &fontAlpha)
    {
        hasX = GetFloatProp(ctx, obj, "x", x);
        hasY = GetFloatProp(ctx, obj, "y", y);
        hasWidth = GetFloatProp(ctx, obj, "width", width);
        hasHeight = GetFloatProp(ctx, obj, "height", height);
        hasRotate = GetFloatProp(ctx, obj, "rotate", rotate);
        hasFontSize = GetFloatProp(ctx, obj, "fontSize", fontSize);
        hasFontWeight = GetFloatProp(ctx, obj, "fontWeight", fontWeight);
        hasLetterSpacing = GetFloatProp(ctx, obj, "letterSpacing", letterSpacing);

        std::wstring fontColor = GetStringProp(ctx, obj, "fontColor");
        if (!fontColor.empty())
        {
            COLORREF color = 0;
            BYTE alpha = 255;
            if (ColorUtil::ParseRGBA(fontColor, color, alpha))
            {
                hasFontColor = true;
                fontColorR = static_cast<float>(GetRValue(color));
                fontColorG = static_cast<float>(GetGValue(color));
                fontColorB = static_cast<float>(GetBValue(color));
                fontAlpha = static_cast<float>(alpha);
            }
        }
    }

    static bool ParseKeyframeOffsetValue(JSContext *ctx, JSValueConst val, float &outOffset)
    {
        if (JS_IsNumber(val) || JS_IsBool(val))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, val) != 0)
                return false;
            if (d < 0.0)
                d = 0.0;
            if (d > 1.0)
                d = 1.0;
            outOffset = static_cast<float>(d);
            return true;
        }

        if (!JS_IsString(val))
            return false;

        const char *utf8 = JS_ToCString(ctx, val);
        if (!utf8)
            return false;
        std::string s = utf8;
        JS_FreeCString(ctx, utf8);

        size_t end = s.find_last_not_of(" \t\r\n");
        if (end == std::string::npos)
            return false;
        s = s.substr(0, end + 1);

        bool isPercent = false;
        if (!s.empty() && s.back() == '%')
        {
            isPercent = true;
            s.pop_back();
        }

        try
        {
            size_t pos = 0;
            double d = std::stod(s, &pos);
            if (pos != s.size())
                return false;
            if (isPercent)
                d /= 100.0;
            if (d < 0.0)
                d = 0.0;
            if (d > 1.0)
                d = 1.0;
            outOffset = static_cast<float>(d);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static void ParseAnimationKeyframeValues(JSContext *ctx, JSValueConst obj, AnimationKeyframeOptions &kf)
    {
        ParseAnimationTargetObject(
            ctx,
            obj,
            kf.hasX,
            kf.x,
            kf.hasY,
            kf.y,
            kf.hasWidth,
            kf.width,
            kf.hasHeight,
            kf.height,
            kf.hasRotate,
            kf.rotate,
            kf.hasFontSize,
            kf.fontSize,
            kf.hasFontWeight,
            kf.fontWeight,
            kf.hasLetterSpacing,
            kf.letterSpacing,
            kf.hasFontColor,
            kf.fontColorR,
            kf.fontColorG,
            kf.fontColorB,
            kf.fontAlpha);

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            kf.easing = easing;
    }

    static bool ParseAnimationKeyframeEntry(JSContext *ctx, JSValueConst obj, float forcedOffset, bool hasForcedOffset, AnimationKeyframeOptions &out)
    {
        if (!JS_IsObject(obj))
            return false;

        out = AnimationKeyframeOptions{};
        if (hasForcedOffset)
        {
            out.offset = forcedOffset;
            out.hasOffset = true;
        }
        else
        {
            JSValue offsetVal = JS_GetPropertyStr(ctx, obj, "offset");
            if (JS_IsException(offsetVal) || JS_IsUndefined(offsetVal) || JS_IsNull(offsetVal))
            {
                JS_FreeValue(ctx, offsetVal);
                offsetVal = JS_GetPropertyStr(ctx, obj, "at");
            }
            if (!JS_IsException(offsetVal) && !JS_IsUndefined(offsetVal) && !JS_IsNull(offsetVal))
            {
                float parsed = 0.0f;
                if (ParseKeyframeOffsetValue(ctx, offsetVal, parsed))
                {
                    out.offset = parsed;
                    out.hasOffset = true;
                }
            }
            JS_FreeValue(ctx, offsetVal);
        }

        ParseAnimationKeyframeValues(ctx, obj, out);
        return out.hasOffset && out.HasAnyProps();
    }

    static void ParseAnimationKeyframesArray(JSContext *ctx, JSValueConst arr, AnimationOptions &options)
    {
        uint32_t len = 0;
        JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
        JS_ToUint32(ctx, &len, lenV);
        JS_FreeValue(ctx, lenV);

        for (uint32_t i = 0; i < len; ++i)
        {
            JSValue item = JS_GetPropertyUint32(ctx, arr, i);
            AnimationKeyframeOptions kf{};
            if (!ParseAnimationKeyframeEntry(ctx, item, 0.0f, false, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry needs offset/at and at least one property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
    }

    static void ParseAnimationKeyframesObject(JSContext *ctx, JSValueConst obj, AnimationOptions &options)
    {
        JSPropertyEnum *tab = nullptr;
        uint32_t tabLen = 0;
        if (JS_GetOwnPropertyNames(ctx, &tab, &tabLen, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
        {
            options.keyframesInvalid = true;
            options.keyframesError = L"invalid keyframes object";
            return;
        }

        for (uint32_t i = 0; i < tabLen; ++i)
        {
            JSValue key = JS_AtomToValue(ctx, tab[i].atom);
            const char *keyUtf8 = JS_ToCString(ctx, key);
            if (!keyUtf8)
            {
                JS_FreeValue(ctx, key);
                continue;
            }

            float offset = 0.0f;
            JSValue keyStr = JS_NewString(ctx, keyUtf8);
            const bool hasOffset = ParseKeyframeOffsetValue(ctx, keyStr, offset);
            JS_FreeValue(ctx, keyStr);
            JS_FreeCString(ctx, keyUtf8);
            JS_FreeValue(ctx, key);

            if (!hasOffset)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes object keys must be percentages like \"0%\" or \"33%\"";
                continue;
            }

            JSValue item = JS_GetProperty(ctx, obj, tab[i].atom);
            AnimationKeyframeOptions kf{};
            if (!ParseAnimationKeyframeEntry(ctx, item, offset, true, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry must include at least one property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
        js_free(ctx, tab);
    }

    void ParseAnimationOptions(JSContext *ctx, JSValueConst obj, AnimationOptions &options)
    {
        options.id = GetStringProp(ctx, obj, "id");
        GetIntProp(ctx, obj, "duration", options.duration);
        if (options.duration <= 0)
            options.duration = 250;

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            options.easing = easing;

        options.iterationCount = 1;
        options.iterationInfinite = false;
        options.hasIterationCount = false;
        options.iterationCountInvalid = false;
        JSValue iterationVal = JS_GetPropertyStr(ctx, obj, "iterationCount");
        if (!JS_IsException(iterationVal) && !JS_IsUndefined(iterationVal) && !JS_IsNull(iterationVal))
        {
            options.hasIterationCount = true;
            if (JS_IsString(iterationVal))
            {
                const char *iterUtf8 = JS_ToCString(ctx, iterationVal);
                if (iterUtf8)
                {
                    std::string iterStr = iterUtf8;
                    JS_FreeCString(ctx, iterUtf8);
                    std::transform(iterStr.begin(), iterStr.end(), iterStr.begin(), ::tolower);
                    if (iterStr == "infinite")
                        options.iterationInfinite = true;
                    else
                        options.iterationCountInvalid = true;
                }
            }
            else
            {
                int count = 0;
                if (GetIntProp(ctx, obj, "iterationCount", count))
                {
                    if (count >= 1)
                        options.iterationCount = count;
                    else
                        options.iterationCountInvalid = true;
                }
                else
                    options.iterationCountInvalid = true;
            }
        }
        JS_FreeValue(ctx, iterationVal);

        JSValue keyframesVal = JS_GetPropertyStr(ctx, obj, "keyframes");
        if (!JS_IsException(keyframesVal) && !JS_IsUndefined(keyframesVal) && !JS_IsNull(keyframesVal))
        {
            options.hasKeyframes = true;
            if (JS_IsArray(keyframesVal))
                ParseAnimationKeyframesArray(ctx, keyframesVal, options);
            else if (JS_IsObject(keyframesVal))
                ParseAnimationKeyframesObject(ctx, keyframesVal, options);
            else
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes must be an array or object with percent keys";
            }
        }
        JS_FreeValue(ctx, keyframesVal);

        if (options.hasKeyframes)
        {
            JSValue toCheck = JS_GetPropertyStr(ctx, obj, "to");
            JSValue fromCheck = JS_GetPropertyStr(ctx, obj, "from");
            const bool hasTo = JS_IsObject(toCheck);
            const bool hasFrom = JS_IsObject(fromCheck);
            JS_FreeValue(ctx, toCheck);
            JS_FreeValue(ctx, fromCheck);
            if (hasTo || hasFrom)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"use either keyframes or from/to, not both";
            }

            if (!options.keyframesInvalid)
            {
                if (options.keyframes.size() < 2)
                {
                    options.keyframesInvalid = true;
                    options.keyframesError = L"keyframes requires at least 2 stops";
                }
                else
                {
                    std::sort(options.keyframes.begin(), options.keyframes.end(), [](const AnimationKeyframeOptions &a, const AnimationKeyframeOptions &b)
                              { return a.offset < b.offset; });
                    for (size_t i = 1; i < options.keyframes.size(); ++i)
                    {
                        if (options.keyframes[i].offset <= options.keyframes[i - 1].offset)
                        {
                            options.keyframesInvalid = true;
                            options.keyframesError = L"keyframes offsets must be strictly increasing";
                            break;
                        }
                    }
                }
            }
            return;
        }

        JSValue toVal = JS_GetPropertyStr(ctx, obj, "to");
        if (JS_IsObject(toVal))
        {
            ParseAnimationTargetObject(
                ctx,
                toVal,
                options.hasX,
                options.x,
                options.hasY,
                options.y,
                options.hasWidth,
                options.width,
                options.hasHeight,
                options.height,
                options.hasRotate,
                options.rotate,
                options.hasFontSize,
                options.fontSize,
                options.hasFontWeight,
                options.fontWeight,
                options.hasLetterSpacing,
                options.letterSpacing,
                options.hasFontColor,
                options.fontColorR,
                options.fontColorG,
                options.fontColorB,
                options.fontAlpha);
        }
        JS_FreeValue(ctx, toVal);

        JSValue fromVal = JS_GetPropertyStr(ctx, obj, "from");
        if (JS_IsObject(fromVal))
        {
            ParseAnimationTargetObject(
                ctx,
                fromVal,
                options.fromHasX,
                options.fromX,
                options.fromHasY,
                options.fromY,
                options.fromHasWidth,
                options.fromWidth,
                options.fromHasHeight,
                options.fromHeight,
                options.fromHasRotate,
                options.fromRotate,
                options.fromHasFontSize,
                options.fromFontSize,
                options.fromHasFontWeight,
                options.fromFontWeight,
                options.fromHasLetterSpacing,
                options.fromLetterSpacing,
                options.fromHasFontColor,
                options.fromFontColorR,
                options.fromFontColorG,
                options.fromFontColorB,
                options.fromFontAlpha);
        }
        JS_FreeValue(ctx, fromVal);
    }
}
