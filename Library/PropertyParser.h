/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSApi/duktape/duktape.h"
#include <d2d1.h>
#include <vector>
#include "Widget.h"
#include "Element.h"
#include "TextElement.h"
#include "ImageElement.h"
#include "BarElement.h"
#include "RoundLineElement.h"
#include "ShapeElement.h"

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

        GradientInfo solidGradient;

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
        
        std::wstring fontPath;
        std::vector<TextShadow> shadows;
        GradientInfo fontGradient;
        float letterSpacing = 0.0f;
        bool underLine = false;
        bool strikeThrough = false;
        TextCase textCase = TEXT_CASE_NORMAL;
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

        GradientInfo barGradient;
    };

    struct RoundLineOptions : public ElementOptions {
        float value = 0.0f;
        int radius = 0;
        int thickness = 2;
        int endThickness = -1;
        float startAngle = 0.0f;
        float totalAngle = 360.0f;
        bool clockwise = true;
        RoundLineCap startCap = ROUNDLINE_CAP_FLAT;
        RoundLineCap endCap = ROUNDLINE_CAP_FLAT;
        std::vector<float> dashArray;
        int ticks = 0;

        bool hasLineColor = false;
        COLORREF lineColor = RGB(0, 255, 0);
        BYTE lineAlpha = 255;

        bool hasLineColorBg = false;
        COLORREF lineColorBg = RGB(50, 50, 50);
        BYTE lineAlphaBg = 255;

        GradientInfo lineGradient;
        GradientInfo lineGradientBg;
    };

    struct ShapeOptions : public ElementOptions {
        std::wstring shapeType; // "rectangle", "ellipse", "line"
        
        float strokeWidth = 1.0f;
        COLORREF strokeColor = RGB(0,0,0);
        BYTE strokeAlpha = 255;
        GradientInfo strokeGradient;
        
        COLORREF fillColor = RGB(255,255,255);
        BYTE fillAlpha = 255;
        GradientInfo fillGradient;

        float radiusX = 0.0f;
        float radiusY = 0.0f;
        
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;

        std::wstring curveType = L"quadratic"; // "quadratic" or "cubic"
        float controlX = 0.0f;
        float controlY = 0.0f;
        float control2X = 0.0f;
        float control2Y = 0.0f;

        float startAngle = 0.0f;
        float endAngle = 90.0f;
        bool clockwise = true;

        std::wstring pathData;
        
        D2D1_CAP_STYLE strokeStartCap = D2D1_CAP_STYLE_FLAT;
        D2D1_CAP_STYLE strokeEndCap = D2D1_CAP_STYLE_FLAT;
        D2D1_CAP_STYLE strokeDashCap = D2D1_CAP_STYLE_FLAT;
        D2D1_LINE_JOIN strokeLineJoin = D2D1_LINE_JOIN_MITER;
        float strokeDashOffset = 0.0f;
        std::vector<float> strokeDashes;

        struct CombineOp {
            std::wstring id;
            D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_UNION;
            bool consume = false;
            bool hasConsume = false;
        };

        bool isCombine = false;
        std::wstring combineBaseId;
        std::vector<CombineOp> combineOps;
        bool combineConsumeAll = false;
        bool hasCombineConsumeAll = false;
    };

    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options, const std::wstring& baseDir = L"");
    void ParseElementOptions(duk_context* ctx, Element* element);
    void ParseImageOptions(duk_context* ctx, ImageOptions& options, const std::wstring& baseDir = L"");
    void ParseTextOptions(duk_context* ctx, TextOptions& options, const std::wstring& baseDir = L"");
    void ParseBarOptions(duk_context* ctx, BarOptions& options, const std::wstring& baseDir = L"");
    void ParseRoundLineOptions(duk_context* ctx, RoundLineOptions& options, const std::wstring& baseDir = L"");
    void ParseShapeOptions(duk_context* ctx, ShapeOptions& options, const std::wstring& baseDir = L"");
 
    void PushWidgetProperties(duk_context* ctx, Widget* widget);
    void PushElementProperties(duk_context* ctx, Element* element);

    void PreFillElementOptions(ElementOptions& options, Element* element);
    void PreFillTextOptions(TextOptions& options, TextElement* element);
    void PreFillImageOptions(ImageOptions& options, ImageElement* element);
    void PreFillBarOptions(BarOptions& options, BarElement* element);
    void PreFillRoundLineOptions(RoundLineOptions& options, RoundLineElement* element);
    void PreFillShapeOptions(ShapeOptions& options, ShapeElement* element);

    void ApplyWidgetProperties(duk_context* ctx, Widget* widget, const std::wstring& baseDir = L"");
    void ApplyElementOptions(Element* element, const ElementOptions& options);
    void ApplyImageOptions(ImageElement* element, const ImageOptions& options);
    void ApplyTextOptions(TextElement* element, const TextOptions& options);
    void ApplyBarOptions(BarElement* element, const BarOptions& options);
    void ApplyRoundLineOptions(RoundLineElement* element, const RoundLineOptions& options);
    void ApplyShapeOptions(ShapeElement* element, const ShapeOptions& options);

}
