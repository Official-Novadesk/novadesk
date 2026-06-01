/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cwctype>
#include <string>
namespace PropertyParser
{
    using namespace Js;
    void ParseLayoutBoxOptions(JSContext *ctx, JSValueConst obj, LayoutBoxOptions &options, const std::wstring &baseDir)
    {
        ParseShapeOptions(ctx, obj, options.shape, baseDir);

        options.shape.shapeType = L"rectangle";
        options.shape.fillAlpha = 0;
        options.shape.solidAlpha = 0;
        options.shape.strokeAlpha = 0;
        options.shape.strokeWidth = 0.0f;
        options.shape.hasSolidColor = false;
        options.shape.solidGradient.type = GRADIENT_NONE;

        int v = 0;
        if (GetIntProp(ctx, obj, "width", v) && v > 0)
            options.shape.width = v;
        if (GetIntProp(ctx, obj, "height", v) && v > 0)
            options.shape.height = v;

        std::wstring bg = GetStringProp(ctx, obj, "backgroundColor");
        if (!bg.empty())
        {
            COLORREF c = RGB(0, 0, 0);
            BYTE a = 255;
            if (ColorUtil::ParseRGBA(bg, c, a))
            {
                options.shape.fillColor = c;
                options.shape.fillAlpha = a;
            }
        }

        std::wstring borderColor = GetStringProp(ctx, obj, "borderColor");
        if (!borderColor.empty())
        {
            COLORREF c = RGB(0, 0, 0);
            BYTE a = 255;
            if (ColorUtil::ParseRGBA(borderColor, c, a))
            {
                options.shape.strokeColor = c;
                options.shape.strokeAlpha = a;
                options.shape.strokeGradient.type = GRADIENT_NONE;
            }
        }

        int borderWidth = 0;
        if (GetIntProp(ctx, obj, "borderWidth", borderWidth))
            options.shape.strokeWidth = static_cast<float>(borderWidth);

        int radius = 0;
        if (GetIntProp(ctx, obj, "borderRadius", radius))
        {
            const float r = static_cast<float>(radius);
            options.shape.radiusX = r;
            options.shape.radiusY = r;
        }

        double opacity = 0.0;
        JSValue opacityV = JS_GetPropertyStr(ctx, obj, "opacity");
        if (!JS_IsUndefined(opacityV) && !JS_IsNull(opacityV) && JS_ToFloat64(ctx, &opacity, opacityV) == 0)
        {
            if (opacity <= 1.0)
                opacity *= 255.0;
            if (opacity < 0.0) opacity = 0.0;
            if (opacity > 255.0) opacity = 255.0;
            const BYTE a = static_cast<BYTE>(opacity);
            options.shape.solidAlpha = a;
            options.shape.fillAlpha = a;
            options.shape.strokeAlpha = a;
        }
        JS_FreeValue(ctx, opacityV);

        auto parseBorderStyleString = [](const std::wstring &str) -> ElementLayoutBox::BorderStyle
        {
            std::wstring lower = str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            if (lower == L"none" || lower == L"hidden") return ElementLayoutBox::BorderStyle::None;
            if (lower == L"inset") return ElementLayoutBox::BorderStyle::Inset;
            if (lower == L"outset") return ElementLayoutBox::BorderStyle::Outset;
            if (lower == L"groove") return ElementLayoutBox::BorderStyle::Groove;
            if (lower == L"ridge") return ElementLayoutBox::BorderStyle::Ridge;
            return ElementLayoutBox::BorderStyle::Solid;
        };

        JSValue styleV = JS_GetPropertyStr(ctx, obj, "borderStyle");
        if (!JS_IsUndefined(styleV) && !JS_IsNull(styleV))
        {
            options.hasBorderStyle = true;
            if (JS_IsString(styleV))
            {
                const char *s = JS_ToCString(ctx, styleV);
                if (s)
                {
                    auto parsed = parseBorderStyleString(Utils::ToWString(s));
                    options.borderTop = options.borderRight = options.borderBottom = options.borderLeft = parsed;
                    JS_FreeCString(ctx, s);
                }
            }
            else if (JS_IsArray(styleV))
            {
                uint32_t len = 0;
                JSValue lenV = JS_GetPropertyStr(ctx, styleV, "length");
                JS_ToUint32(ctx, &len, lenV);
                JS_FreeValue(ctx, lenV);

                std::vector<ElementLayoutBox::BorderStyle> styles;
                styles.reserve(len);
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue item = JS_GetPropertyUint32(ctx, styleV, i);
                    const char *s = JS_ToCString(ctx, item);
                    if (s)
                    {
                        styles.push_back(parseBorderStyleString(Utils::ToWString(s)));
                        JS_FreeCString(ctx, s);
                    }
                    else
                    {
                        styles.push_back(ElementLayoutBox::BorderStyle::Solid);
                    }
                    JS_FreeValue(ctx, item);
                }

                if (styles.size() == 1)
                {
                    options.borderTop = options.borderRight = options.borderBottom = options.borderLeft = styles[0];
                }
                else if (styles.size() == 2)
                {
                    options.borderTop = options.borderBottom = styles[0];
                    options.borderRight = options.borderLeft = styles[1];
                }
                else if (styles.size() == 3)
                {
                    options.borderTop = styles[0];
                    options.borderRight = options.borderLeft = styles[1];
                    options.borderBottom = styles[2];
                }
                else if (styles.size() >= 4)
                {
                    options.borderTop = styles[0];
                    options.borderRight = styles[1];
                    options.borderBottom = styles[2];
                    options.borderLeft = styles[3];
                }
            }
        }
        JS_FreeValue(ctx, styleV);

        options.boxShadows.clear();
        options.hasBoxShadowError = false;
        options.boxShadowError.clear();
        JSValue shadowV = JS_GetPropertyStr(ctx, obj, "boxShadow");
        if (!JS_IsUndefined(shadowV) && !JS_IsNull(shadowV))
        {
            auto parseShadowObject = [&](JSValueConst shadowObj, LayoutBoxShadowOptions &outShadow) -> bool
            {
                if (!JS_IsObject(shadowObj) || JS_IsArray(shadowObj))
                    return false;
                GetFloatProp(ctx, shadowObj, "x", outShadow.x);
                GetFloatProp(ctx, shadowObj, "y", outShadow.y);
                GetFloatProp(ctx, shadowObj, "blur", outShadow.blur);
                GetFloatProp(ctx, shadowObj, "spread", outShadow.spread);
                GetBoolProp(ctx, shadowObj, "inset", outShadow.inset);
                std::wstring color = GetStringProp(ctx, shadowObj, "color");
                if (!color.empty())
                    ColorUtil::ParseRGBA(color, outShadow.color, outShadow.alpha);
                return true;
            };

            if (JS_IsString(shadowV))
            {
                options.hasBoxShadowError = true;
                options.boxShadowError = L"addLayoutBox: boxShadow string syntax is not supported; use object or object[]";
            }
            else if (JS_IsArray(shadowV))
            {
                uint32_t len = 0;
                JSValue lenV = JS_GetPropertyStr(ctx, shadowV, "length");
                JS_ToUint32(ctx, &len, lenV);
                JS_FreeValue(ctx, lenV);
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue item = JS_GetPropertyUint32(ctx, shadowV, i);
                    LayoutBoxShadowOptions sh;
                    if (!parseShadowObject(item, sh))
                    {
                        options.hasBoxShadowError = true;
                        options.boxShadowError = L"addLayoutBox: each boxShadow array item must be an object";
                        JS_FreeValue(ctx, item);
                        break;
                    }
                    options.boxShadows.push_back(sh);
                    JS_FreeValue(ctx, item);
                }
            }
            else
            {
                LayoutBoxShadowOptions sh;
                if (!parseShadowObject(shadowV, sh))
                {
                    options.hasBoxShadowError = true;
                    options.boxShadowError = L"addLayoutBox: boxShadow must be an object or object[]";
                }
                else
                {
                    options.boxShadows.push_back(sh);
                }
            }
        }
        JS_FreeValue(ctx, shadowV);

        options.direction = GetStringProp(ctx, obj, "direction");
        if (options.direction.empty())
        {
            JSValue styleDir = JS_GetPropertyStr(ctx, obj, "style");
            if (JS_IsObject(styleDir))
                options.direction = GetStringProp(ctx, styleDir, "direction");
            JS_FreeValue(ctx, styleDir);
        }
        if (options.direction.empty())
            options.direction = L"column";
        std::transform(options.direction.begin(), options.direction.end(), options.direction.begin(), ::towlower);
        if (options.direction != L"row")
            options.direction = L"column";

        if (!GetIntProp(ctx, obj, "gap", options.gap))
        {
            JSValue styleGap = JS_GetPropertyStr(ctx, obj, "style");
            if (JS_IsObject(styleGap))
            {
                int tmp = 0;
                if (GetIntProp(ctx, styleGap, "gap", tmp))
                    options.gap = tmp;
            }
            JS_FreeValue(ctx, styleGap);
        }

        options.align = GetStringProp(ctx, obj, "align");
        if (options.align.empty())
            options.align = GetStringProp(ctx, obj, "alignItems");
        if (!options.align.empty())
            std::transform(options.align.begin(), options.align.end(), options.align.begin(), ::towlower);

        options.justify = GetStringProp(ctx, obj, "justify");
        if (options.justify.empty())
            options.justify = GetStringProp(ctx, obj, "justifyContent");
        if (!options.justify.empty())
            std::transform(options.justify.begin(), options.justify.end(), options.justify.begin(), ::towlower);

        int pad = 0;
        if (GetIntProp(ctx, obj, "padding", pad))
            options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = pad;

        JSValue paddingVal = JS_GetPropertyStr(ctx, obj, "padding");
        if (JS_IsArray(paddingVal))
        {
            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, paddingVal, "length");
            JS_ToUint32(ctx, &len, lenV);
            JS_FreeValue(ctx, lenV);

            std::vector<int> values;
            values.reserve(len);
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue iv = JS_GetPropertyUint32(ctx, paddingVal, i);
                int32_t pv = 0;
                if (JS_ToInt32(ctx, &pv, iv) == 0)
                    values.push_back(static_cast<int>(pv));
                JS_FreeValue(ctx, iv);
            }

            if (values.size() == 1)
            {
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = values[0];
            }
            else if (values.size() == 2)
            {
                options.paddingLeft = options.paddingRight = values[0];
                options.paddingTop = options.paddingBottom = values[1];
            }
            else if (values.size() >= 4)
            {
                options.paddingLeft = values[0];
                options.paddingTop = values[1];
                options.paddingRight = values[2];
                options.paddingBottom = values[3];
            }
        }
        JS_FreeValue(ctx, paddingVal);

        JSValue stylePadding = JS_GetPropertyStr(ctx, obj, "style");
        if (JS_IsObject(stylePadding))
        {
            int padX = 0;
            int padY = 0;
            int minWidth = 0;
            int minHeight = 0;
            if (GetIntProp(ctx, stylePadding, "padding", pad))
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = pad;
            if (GetIntProp(ctx, stylePadding, "paddingX", padX))
                options.paddingLeft = options.paddingRight = padX;
            if (GetIntProp(ctx, stylePadding, "paddingY", padY))
                options.paddingTop = options.paddingBottom = padY;
            std::wstring alignItems = GetStringProp(ctx, stylePadding, "alignItems");
            if (!alignItems.empty())
            {
                std::transform(alignItems.begin(), alignItems.end(), alignItems.begin(), ::towlower);
                options.align = alignItems;
            }
            std::wstring justifyContent = GetStringProp(ctx, stylePadding, "justifyContent");
            if (!justifyContent.empty())
            {
                std::transform(justifyContent.begin(), justifyContent.end(), justifyContent.begin(), ::towlower);
                options.justify = justifyContent;
            }
            if (GetIntProp(ctx, stylePadding, "minWidth", minWidth) && minWidth > 0)
                options.minWidth = minWidth;
            if (GetIntProp(ctx, stylePadding, "minHeight", minHeight) && minHeight > 0)
                options.minHeight = minHeight;
        }
        JS_FreeValue(ctx, stylePadding);
    }
    void ApplyLayoutBoxOptions(ElementLayoutBox *element, const LayoutBoxOptions &options)
    {
        if (!element)
            return;

        ApplyShapeOptions(element, options.shape);

        if (options.hasBorderStyle)
        {
            element->SetBorderStyle(
                options.borderTop,
                options.borderRight,
                options.borderBottom,
                options.borderLeft);
        }

        std::vector<ElementLayoutBox::BoxShadow> shadows;
        shadows.reserve(options.boxShadows.size());
        for (const auto &shadow : options.boxShadows)
        {
            ElementLayoutBox::BoxShadow outShadow;
            outShadow.x = shadow.x;
            outShadow.y = shadow.y;
            outShadow.blur = shadow.blur;
            outShadow.spread = shadow.spread;
            outShadow.color = shadow.color;
            outShadow.alpha = shadow.alpha;
            outShadow.inset = shadow.inset;
            shadows.push_back(outShadow);
        }
        element->SetBoxShadows(shadows);
    }
    void PreFillLayoutBoxOptions(
        LayoutBoxOptions &options,
        ElementLayoutBox *element,
        const std::wstring *direction,
        const int *gap,
        const std::wstring *align,
        const std::wstring *justify,
        const int *paddingLeft,
        const int *paddingTop,
        const int *paddingRight,
        const int *paddingBottom,
        const int *minWidth,
        const int *minHeight)
    {
        if (!element)
            return;

        PreFillShapeOptions(options.shape, element);
        options.shape.shapeType = L"rectangle";

        options.hasBorderStyle = true;
        options.borderTop = element->GetBorderStyleTop();
        options.borderRight = element->GetBorderStyleRight();
        options.borderBottom = element->GetBorderStyleBottom();
        options.borderLeft = element->GetBorderStyleLeft();

        options.boxShadows.clear();
        const auto &currentShadows = element->GetBoxShadows();
        options.boxShadows.reserve(currentShadows.size());
        for (const auto &shadow : currentShadows)
        {
            LayoutBoxShadowOptions outShadow;
            outShadow.x = shadow.x;
            outShadow.y = shadow.y;
            outShadow.blur = shadow.blur;
            outShadow.spread = shadow.spread;
            outShadow.color = shadow.color;
            outShadow.alpha = shadow.alpha;
            outShadow.inset = shadow.inset;
            options.boxShadows.push_back(outShadow);
        }

        options.direction = direction ? *direction : L"column";
        options.gap = gap ? *gap : 0;
        options.align = align ? *align : L"";
        options.justify = justify ? *justify : L"";
        options.paddingLeft = paddingLeft ? *paddingLeft : element->GetPaddingLeft();
        options.paddingTop = paddingTop ? *paddingTop : element->GetPaddingTop();
        options.paddingRight = paddingRight ? *paddingRight : element->GetPaddingRight();
        options.paddingBottom = paddingBottom ? *paddingBottom : element->GetPaddingBottom();
        options.minWidth = minWidth ? *minWidth : 0;
        options.minHeight = minHeight ? *minHeight : 0;
    }
}
