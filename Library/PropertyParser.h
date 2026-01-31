/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSApi/duktape/duktape.h"
#include "Widget.h"
#include "Element.h"
#include "TextElement.h"
#include "ImageElement.h"
#include "BarElement.h"

namespace PropertyParser
{
    /*
    ** Common options for all elements
    */
    struct ElementOptions {
        std::wstring id;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        
        // Background / Gradient
        bool hasSolidColor = false;
        COLORREF solidColor = 0;
        BYTE solidAlpha = 0;
        int solidColorRadius = 0;

        bool hasGradient = false;
        COLORREF solidColor2 = 0;
        BYTE solidAlpha2 = 0;
        float gradientAngle = 0.0f;

        // Bevel
        int bevelType = 0;
        int bevelWidth = 0;
        COLORREF bevelColor = RGB(255, 255, 255);
        BYTE bevelAlpha = 200;
        COLORREF bevelColor2 = RGB(0, 0, 0);
        BYTE bevelAlpha2 = 150;

        // Padding
        int paddingLeft = 0;
        int paddingTop = 0;
        int paddingRight = 0;
        int paddingBottom = 0;

        // Mouse Actions
        // Mouse Actions (String based actions removed)


        // Callback IDs
        int onLeftMouseUpCallbackId = -1;
        int onLeftMouseDownCallbackId = -1;
        int onLeftDoubleClickCallbackId = -1;
        int onRightMouseUpCallbackId = -1;
        int onRightMouseDownCallbackId = -1;
        int onRightDoubleClickCallbackId = -1;
        int onMiddleMouseUpCallbackId = -1;
        int onMiddleMouseDownCallbackId = -1;
        int onMiddleDoubleClickCallbackId = -1;
        
        int onX1MouseUpCallbackId = -1;
        int onX1MouseDownCallbackId = -1;
        int onX1DoubleClickCallbackId = -1;
        int onX2MouseUpCallbackId = -1;
        int onX2MouseDownCallbackId = -1;
        int onX2DoubleClickCallbackId = -1;
        
        int onScrollUpCallbackId = -1;
        int onScrollDownCallbackId = -1;
        int onScrollLeftCallbackId = -1;
        int onScrollRightCallbackId = -1;
        
        int onMouseOverCallbackId = -1;
        int onMouseLeaveCallbackId = -1;

        // Tooltip properties
        std::wstring tooltipText;
        std::wstring tooltipTitle;
        std::wstring tooltipIcon;
        int tooltipMaxWidth = 0;
        int tooltipMaxHeight = 0;
        bool tooltipBalloon = false;

        bool antialias = true;
        
        float rotate = 0.0f;
        bool hasTransformMatrix = false;
        std::vector<float> transformMatrix;
    };

    /*
    ** Options for creating/updating Image elements
    */
    struct ImageOptions : public ElementOptions {
        std::wstring path;
        ImageAspectRatio preserveAspectRatio = IMAGE_ASPECT_STRETCH;
        
        bool hasImageTint = false;
        COLORREF imageTint = 0;
        BYTE imageTintAlpha = 255;
        
        BYTE imageAlpha = 255;
        bool grayscale = false;
        bool tile = false;
        
        bool hasColorMatrix = false;
        std::vector<float> colorMatrix;
    };

    /*
    ** Options for creating/updating Text elements
    */
    struct TextOptions : public ElementOptions {
        std::wstring text;
        std::wstring fontFace = L"Arial";
        int fontSize = 12;
        COLORREF fontColor = RGB(0, 0, 0);
        BYTE alpha = 255;
        int fontWeight = 400;
        bool italic = false;
        TextAlignment textAlign = TEXT_ALIGN_LEFT_TOP;
        TextClipString clip = TEXT_CLIP_NONE;
        
        std::vector<TextShadow> shadows;
    };

    /*
    ** Options for creating/updating Bar elements
    */
    struct BarOptions : public ElementOptions {
        float value = 0.0f;
        BarOrientation orientation = BAR_HORIZONTAL;
        
        int barCornerRadius = 0;

        bool hasBarColor = false;
        COLORREF barColor = RGB(0, 255, 0);
        BYTE barAlpha = 255;

        bool hasBarGradient = false;
        COLORREF barColor2 = 0;
        BYTE barAlpha2 = 255;
        float barGradientAngle = 0.0f;
    };

    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options, const std::wstring& baseDir = L"");
    void ParseImageOptions(duk_context* ctx, ImageOptions& options, const std::wstring& baseDir = L"");
    void ParseTextOptions(duk_context* ctx, TextOptions& options, const std::wstring& baseDir = L"");
    void ParseBarOptions(duk_context* ctx, BarOptions& options, const std::wstring& baseDir = L"");
    void ApplyWidgetProperties(duk_context* ctx, Widget* widget, const std::wstring& baseDir = L"");

    void PushWidgetProperties(duk_context* ctx, Widget* widget);
    void ParseElementOptions(duk_context* ctx, Element* element);
    void PushElementProperties(duk_context* ctx, Element* element);
    void PreFillElementOptions(ElementOptions& options, Element* element);
    void PreFillTextOptions(TextOptions& options, TextElement* element);
    void PreFillImageOptions(ImageOptions& options, ImageElement* element);
    void PreFillBarOptions(BarOptions& options, BarElement* element);
    void ApplyElementOptions(Element* element, const ElementOptions& options);

    void ApplyImageOptions(ImageElement* element, const ImageOptions& options);
    void ApplyTextOptions(TextElement* element, const TextOptions& options);
    void ApplyBarOptions(BarElement* element, const BarOptions& options);
}
