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
    void ParseTextOptions(JSContext *ctx, JSValueConst obj, TextOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        {
            JSValue textProp = JS_GetPropertyStr(ctx, obj, "text");
            if (!JS_IsException(textProp) && !JS_IsUndefined(textProp) && !JS_IsNull(textProp))
            {
                if (JS_IsString(textProp))
                {
                    const char *s = JS_ToCString(ctx, textProp);
                    if (s)
                    {
                        options.text = Utils::ToWString(s);
                        JS_FreeCString(ctx, s);
                    }
                }
            }
            JS_FreeValue(ctx, textProp);
        }
        std::wstring fontFace = GetStringProp(ctx, obj, "fontFace");
        if (!fontFace.empty())
            options.fontFace = fontFace;
        GetIntProp(ctx, obj, "fontSize", options.fontSize);

        std::wstring fc = GetStringProp(ctx, obj, "fontColor");
        if (!fc.empty())
        {
            ParseGradientOrColor(fc, options.fontColor, options.alpha, options.fontGradient, options.hasSolidColor);
        }

        GetFloatProp(ctx, obj, "letterSpacing", options.letterSpacing);
        GetBoolProp(ctx, obj, "underLine", options.underLine);
        GetBoolProp(ctx, obj, "strikeThrough", options.strikeThrough);

        std::wstring caseStr = GetStringProp(ctx, obj, "case");
        if (caseStr == L"upper")
            options.textCase = TEXT_CASE_UPPER;
        else if (caseStr == L"lower")
            options.textCase = TEXT_CASE_LOWER;
        else if (caseStr == L"capitalize")
            options.textCase = TEXT_CASE_CAPITALIZE;
        else if (caseStr == L"sentence")
            options.textCase = TEXT_CASE_SENTENCE;

        JSValue fw = JS_GetPropertyStr(ctx, obj, "fontWeight");
        if (JS_IsNumber(fw))
        {
            int32_t w = 400;
            if (JS_ToInt32(ctx, &w, fw) == 0)
                options.fontWeight = w;
        }
        else if (JS_IsString(fw))
        {
            const char *s = JS_ToCString(ctx, fw);
            if (s)
            {
                std::wstring weight = Utils::ToWString(s);
                std::transform(weight.begin(), weight.end(), weight.begin(), ::towlower);
                if (weight == L"thin")
                    options.fontWeight = 100;
                else if (weight == L"extralight" || weight == L"ultralight")
                    options.fontWeight = 200;
                else if (weight == L"light")
                    options.fontWeight = 300;
                else if (weight == L"normal" || weight == L"regular")
                    options.fontWeight = 400;
                else if (weight == L"medium")
                    options.fontWeight = 500;
                else if (weight == L"semibold" || weight == L"demibold")
                    options.fontWeight = 600;
                else if (weight == L"bold")
                    options.fontWeight = 700;
                else if (weight == L"extrabold" || weight == L"ultrabold")
                    options.fontWeight = 800;
                else if (weight == L"black" || weight == L"heavy")
                    options.fontWeight = 900;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, fw);

        std::wstring fontPath = GetStringProp(ctx, obj, "fontPath");
        if (!fontPath.empty())
        {
            options.fontPath = PathUtils::ResolvePath(fontPath, baseDir);
        }

        std::wstring style = GetStringProp(ctx, obj, "fontStyle");
        if (!style.empty())
            options.italic = (style == L"italic");

        std::wstring align = GetStringProp(ctx, obj, "textAlign");
        if (align.empty())
            align = GetStringProp(ctx, obj, "align");
        
        if (!align.empty())
        {
            std::transform(align.begin(), align.end(), align.begin(), ::towlower);
            if (align == L"left" || align == L"lefttop")
                options.textAlign = TEXT_ALIGN_LEFT_TOP;
            else if (align == L"center" || align == L"centertop")
                options.textAlign = TEXT_ALIGN_CENTER_TOP;
            else if (align == L"right" || align == L"righttop")
                options.textAlign = TEXT_ALIGN_RIGHT_TOP;
            else if (align == L"leftcenter")
                options.textAlign = TEXT_ALIGN_LEFT_CENTER;
            else if (align == L"centercenter" || align == L"middlecenter" || align == L"middle")
                options.textAlign = TEXT_ALIGN_CENTER_CENTER;
            else if (align == L"rightcenter")
                options.textAlign = TEXT_ALIGN_RIGHT_CENTER;
            else if (align == L"leftbottom")
                options.textAlign = TEXT_ALIGN_LEFT_BOTTOM;
            else if (align == L"centerbottom")
                options.textAlign = TEXT_ALIGN_CENTER_BOTTOM;
            else if (align == L"rightbottom")
                options.textAlign = TEXT_ALIGN_RIGHT_BOTTOM;
        }

        std::wstring clip = GetStringProp(ctx, obj, "textClip");
        if (!clip.empty())
        {
            if (clip == L"none")
                options.clip = TEXT_CLIP_NONE;
            else if (clip == L"on" || clip == L"clip")
                options.clip = TEXT_CLIP_ON;
            else if (clip == L"wrap")
                options.clip = TEXT_CLIP_WRAP;
            else if (clip == L"ellipsis")
                options.clip = TEXT_CLIP_ELLIPSIS;
        }

        ParseTextShadows(ctx, obj, options.shadows);
    }
    void ApplyTextOptions(TextElement *element, const TextOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetText(options.text);
        element->SetFontFace(options.fontFace);
        element->SetFontSize(options.fontSize);
        element->SetFontColor(options.fontColor, options.alpha);
        element->SetFontWeight(options.fontWeight);
        element->SetItalic(options.italic);
        element->SetTextAlign(options.textAlign);
        element->SetClip(options.clip);
        if (!options.fontPath.empty())
            element->SetFontPath(options.fontPath);
        element->SetShadows(options.shadows);
        element->SetFontGradient(options.fontGradient);
        element->SetLetterSpacing(options.letterSpacing);
        element->SetUnderline(options.underLine);
        element->SetStrikethrough(options.strikeThrough);
        element->SetTextCase(options.textCase);
    }
    void PreFillTextOptions(TextOptions &options, TextElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);

        options.text = element->GetText();
        options.fontFace = element->GetFontFace();
        options.fontSize = element->GetFontSize();
        options.fontColor = element->GetFontColor();
        options.alpha = element->GetFontAlpha();
        options.fontWeight = element->GetFontWeight();
        options.italic = element->IsItalic();
        options.textAlign = element->GetTextAlign();
        options.clip = element->GettextClip();
        options.fontPath = element->GetFontPath();
        options.shadows = element->GetShadows();
        options.fontGradient = element->GetFontGradient();
        options.letterSpacing = element->GetLetterSpacing();
        options.underLine = element->GetUnderline();
        options.strikeThrough = element->GetStrikethrough();
        options.textCase = element->GetTextCase();
    }
}
