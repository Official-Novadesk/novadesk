/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "duktape/duktape.h"
#include "Widget.h"
#include "Element.h"

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
        COLORREF bevelColor1 = RGB(255, 255, 255);
        BYTE bevelAlpha1 = 200;
        COLORREF bevelColor2 = RGB(0, 0, 0);
        BYTE bevelAlpha2 = 150;

        // Padding
        int paddingLeft = 0;
        int paddingTop = 0;
        int paddingRight = 0;
        int paddingBottom = 0;

        // Mouse Actions
        std::wstring onLeftMouseUp;
        std::wstring onLeftMouseDown;
        std::wstring onLeftDoubleClick;
        std::wstring onRightMouseUp;
        std::wstring onRightMouseDown;
        std::wstring onRightDoubleClick;
        std::wstring onMiddleMouseUp;
        std::wstring onMiddleMouseDown;
        std::wstring onMiddleDoubleClick;
        
        std::wstring onX1MouseUp;
        std::wstring onX1MouseDown;
        std::wstring onX1DoubleClick;
        std::wstring onX2MouseUp;
        std::wstring onX2MouseDown;
        std::wstring onX2DoubleClick;
        
        std::wstring onScrollUp;
        std::wstring onScrollDown;
        std::wstring onScrollLeft;
        std::wstring onScrollRight;
        
        std::wstring onMouseOver;
        std::wstring onMouseLeave;

        bool antialias = true;
        
        float rotate = 0.0f;
    };

    /*
    ** Options for creating/updating Image elements
    */
    struct ImageOptions : public ElementOptions {
        std::wstring path;
        int preserveAspectRatio = 0;
        
        bool hasImageTint = false;
        COLORREF imageTint = 0;
        BYTE imageTintAlpha = 255;
        
        BYTE imageAlpha = 255;
        bool grayscale = false;
        bool tile = false;
        
        bool hasTransformMatrix = false;
        std::vector<float> transformMatrix;
        
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
        bool bold = false;
        bool italic = false;
        Alignment textAlign = ALIGN_LEFT_TOP;
        ClipString clip = CLIP_NONE;
        int clipW = -1;
        int clipH = -1;
    };

    /*
    ** Parse WidgetOptions from a Duktape object at the top of the stack.
    ** Optionally loads settings if an 'id' is present.
    */
    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options);

    /*
    ** Parse ImageOptions from a Duktape object at the top of the stack.
    */
    void ParseImageOptions(duk_context* ctx, ImageOptions& options);

    /*
    ** Parse TextOptions from a Duktape object at the top of the stack.
    */
    void ParseTextOptions(duk_context* ctx, TextOptions& options);

    /*
    ** Apply properties from a Duktape object to an existing Widget instance.
    */
    void ApplyWidgetProperties(duk_context* ctx, Widget* widget);

    /*
    ** Push a Widget's current properties as a JavaScript object onto the stack.
    */
    void PushWidgetProperties(duk_context* ctx, Widget* widget);

    /*
    ** Parse mouse/scroll event handlers and apply them to an Element.
    ** This is the legacy version that parses and applies in one step.
    */
    void ParseElementOptions(duk_context* ctx, Element* element);

    /*
    ** Apply properties from an ElementOptions struct to an Element instance.
    */
    void ApplyElementOptions(Element* element, const ElementOptions& options);
}
