/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <array>
#include <string>
#include <vector>

#include <d2d1.h>
#include <windows.h>

#include "../../../render/BarElement.h"
#include "../../../render/BitmapElement.h"
#include "../../../render/ButtonElement.h"
#include "../../../render/Element.h"
#include "../../../render/ElementLayoutBox.h"
#include "../../../render/ImageElement.h"
#include "../../../render/LineElement.h"
#include "../../../render/RoundLineElement.h"
#include "../../../render/TextElement.h"

namespace PropertyParser
{
    struct ElementOptions
    {
        std::wstring id;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool hasSolidColor = false;
        COLORREF solidColor = 0;
        BYTE solidAlpha = 0;
        int solidColorRadius = 0;
        GradientInfo solidGradient;
        int bevelType = 0;
        int bevelWidth = 0;
        COLORREF bevelColor = RGB(255, 255, 255);
        BYTE bevelAlpha = 200;
        GradientInfo bevelGradient;
        COLORREF bevelColor2 = RGB(0, 0, 0);
        BYTE bevelAlpha2 = 150;
        GradientInfo bevelGradient2;
        int paddingLeft = 0;
        int paddingTop = 0;
        int paddingRight = 0;
        int paddingBottom = 0;
        bool antialias = true;
        bool hasPixelHitTest = false;
        bool pixelHitTest = false;
        bool show = true;
        std::wstring containerId;
        std::wstring groupId;
        bool mouseEventCursor = true;
        std::wstring mouseEventCursorName;
        std::wstring cursorsDir;
        float rotate = 0.0f;
        bool hasTransformMatrix = false;
        std::vector<float> transformMatrix;
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
        int onDragStartCallbackId = -1;
        int onDragCallbackId = -1;
        int onDragEndCallbackId = -1;
        std::wstring tooltipText;
        std::wstring tooltipTitle;
        std::wstring tooltipIcon;
        int tooltipMaxWidth = 0;
        int tooltipMaxHeight = 0;
        bool tooltipBalloon = false;
        bool tooltipDisabled = false;
    };

    struct GeneralImageOptions : public ElementOptions
    {
        ImageFlipMode imageFlip = IMAGE_FLIP_NONE;
        bool hasImageCrop = false;
        float imageCropX = 0.0f;
        float imageCropY = 0.0f;
        float imageCropW = 0.0f;
        float imageCropH = 0.0f;
        ImageCropOrigin imageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
        bool useExifOrientation = false;
        BYTE imageAlpha = 255;
        bool grayscale = false;
        bool hasColorMatrix = false;
        std::array<float, 20> colorMatrix{};
        bool hasImageTint = false;
        COLORREF imageTint = RGB(255, 255, 255);
        BYTE imageTintAlpha = 255;
    };

    struct ImageOptions : public GeneralImageOptions
    {
        std::wstring path;
        ImageAspectRatio preserveAspectRatio = IMAGE_ASPECT_STRETCH;
        bool hasScaleMargins = false;
        float scaleMarginLeft = 0.0f;
        float scaleMarginTop = 0.0f;
        float scaleMarginRight = 0.0f;
        float scaleMarginBottom = 0.0f;
        bool tile = false;
    };

    struct ButtonOptions : public GeneralImageOptions
    {
        std::wstring buttonImageName;
        int buttonActionCallbackId = -1;
    };

    struct BitmapOptions : public GeneralImageOptions
    {
        double value = 0.0;
        std::wstring bitmapImageName;
        int bitmapFrames = 1;
        bool bitmapZeroFrame = false;
        bool bitmapExtend = false;
        double minValue = 0.0;
        double maxValue = 1.0;
        std::wstring bitmapOrientation = L"auto";
        int bitmapDigits = 0;
        BitmapAlign bitmapAlign = BITMAP_ALIGN_LEFT;
        int bitmapSeparation = 0;
    };

    struct RotatorOptions : public GeneralImageOptions
    {
        double value = 0.0;
        std::wstring rotatorImageName;
        double minValue = 0.0;
        double maxValue = 1.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        double startAngle = 0.0;
        double rotationAngle = 6.283185307179586; // 2 * PI
        int valueRemainder = 0;
    };
    
    struct AreaGraphOptions : public ElementOptions
    {
        std::vector<float> data;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        bool autoRange = false;
        COLORREF lineColor = RGB(0, 180, 255);
        GradientInfo lineGradient;
        float lineWidth = 1.0f;
        COLORREF fillColor = RGB(0, 180, 255);
        BYTE fillAlpha = 50;
        GradientInfo fillGradient;
        int maxPoints = 0;
        COLORREF gridColor = RGB(100, 100, 100);
        BYTE gridAlpha = 100;
        GradientInfo gridGradient;
        bool gridVisible = true;
        int gridX = 20;
        int gridY = 20;
        bool graphStartLeft = false;
        bool flip = false;
    };

    struct TextOptions : public ElementOptions
    {
        std::wstring text;
        std::wstring fontFace = L"Arial";
        int fontSize = 12;
        COLORREF fontColor = RGB(0, 0, 0);
        BYTE alpha = 255;
        int fontWeight = 400;
        bool italic = false;
        TextAlignment textAlign = TEXT_ALIGN_LEFT_TOP;
        TextClip clip = TEXT_CLIP_NONE;
        std::wstring fontPath;
        std::vector<TextShadow> shadows;
        GradientInfo fontGradient;
        float letterSpacing = 0.0f;
        bool underLine = false;
        bool strikeThrough = false;
        TextCase textCase = TEXT_CASE_NORMAL;
    };

    struct BarOptions : public ElementOptions
    {
        float value = 0.0f;
        BarOrientation orientation = BAR_HORIZONTAL;
        int barCornerRadius = 0;
        bool hasBarColor = false;
        COLORREF barColor = RGB(0, 255, 0);
        BYTE barAlpha = 255;
        GradientInfo barGradient;
    };

    struct RoundLineOptions : public ElementOptions
    {
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

    struct LineOptions : public ElementOptions
    {
        int lineCount = 1;
        std::vector<std::vector<float>> dataSets;
        std::vector<COLORREF> lineColors;
        std::vector<BYTE> lineAlphas;
        std::vector<GradientInfo> lineGradients;
        std::vector<float> scaleValues;
        float lineWidth = 1.0f;
        int maxPoints = 0;
        bool horizontalLines = false;
        COLORREF horizontalLineColor = RGB(0, 0, 0);
        BYTE horizontalLineAlpha = 255;
        GradientInfo horizontalLineGradient;
        bool graphStartLeft = false;               // false = right
        bool graphHorizontalOrientation = false;   // false = vertical
        bool flip = false;
        D2D1_STROKE_TRANSFORM_TYPE transformStroke = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
        bool autoRange = false;
        float scaleMin = 0.0f;
        float scaleMax = 100.0f;
    };

    struct HistogramOptions : public ElementOptions
    {
        std::vector<float> data;
        std::vector<float> data2;
        bool autoRange = false;
        bool graphStartLeft = false;             // false = right
        bool graphHorizontalOrientation = false; // false = vertical
        bool flip = false;

        COLORREF primaryColor = RGB(0, 128, 0);
        BYTE primaryAlpha = 255;
        GradientInfo primaryGradient;
        COLORREF secondaryColor = RGB(255, 0, 0);
        BYTE secondaryAlpha = 255;
        GradientInfo secondaryGradient;
        COLORREF bothColor = RGB(255, 255, 0);
        BYTE bothAlpha = 255;
        GradientInfo bothGradient;
    };

    struct ShapeCombineOp
    {
        std::wstring id;
        D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_UNION;
        bool hasConsume = false;
        bool consume = false;
    };

    struct ShapeOptions : public ElementOptions
    {
        bool isCombine = false;
        std::wstring shapeType;
        float strokeWidth = 1.0f;
        COLORREF strokeColor = RGB(0, 0, 0);
        BYTE strokeAlpha = 255;
        GradientInfo strokeGradient;
        COLORREF fillColor = RGB(255, 255, 255);
        BYTE fillAlpha = 255;
        GradientInfo fillGradient;
        float radiusX = 0.0f;
        float radiusY = 0.0f;
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        std::wstring curveType = L"quadratic";
        float controlX = 0.0f;
        float controlY = 0.0f;
        float control2X = 0.0f;
        float control2Y = 0.0f;
        float startArcAngle = 0.0f;
        float endArcAngle = 90.0f;
        bool arcClockwise = true;
        std::wstring pathData;
        D2D1_CAP_STYLE strokeStartCap = D2D1_CAP_STYLE_FLAT;
        D2D1_CAP_STYLE strokeEndCap = D2D1_CAP_STYLE_FLAT;
        D2D1_CAP_STYLE strokeDashCap = D2D1_CAP_STYLE_FLAT;
        D2D1_LINE_JOIN strokeLineJoin = D2D1_LINE_JOIN_MITER;
        float strokeDashOffset = 0.0f;
        std::vector<float> strokeDashes;
        std::wstring combineBaseId;
        std::vector<ShapeCombineOp> combineOps;
        bool hasCombineConsumeAll = false;
        bool combineConsumeAll = false;
    };

    struct LayoutBoxShadowOptions
    {
        float x = 0.0f;
        float y = 0.0f;
        float blur = 0.0f;
        float spread = 0.0f;
        COLORREF color = RGB(0, 0, 0);
        BYTE alpha = 255;
        bool inset = false;
    };

    struct LayoutBoxOptions
    {
        ShapeOptions shape;
        bool hasBorderStyle = false;
        ElementLayoutBox::BorderStyle borderTop = ElementLayoutBox::BorderStyle::Solid;
        ElementLayoutBox::BorderStyle borderRight = ElementLayoutBox::BorderStyle::Solid;
        ElementLayoutBox::BorderStyle borderBottom = ElementLayoutBox::BorderStyle::Solid;
        ElementLayoutBox::BorderStyle borderLeft = ElementLayoutBox::BorderStyle::Solid;
        std::vector<LayoutBoxShadowOptions> boxShadows;
        bool hasBoxShadowError = false;
        std::wstring boxShadowError;

        std::wstring direction = L"ltr";
        std::wstring flexDirection = L"row";
        int gap = 0;
        std::wstring align;
        std::wstring justify;
        int paddingLeft = 0;
        int paddingTop = 0;
        int paddingRight = 0;
        int paddingBottom = 0;
        ElementLayoutBox::DisplayType displayType = ElementLayoutBox::DisplayType::Flex;
    };

    struct AnimationKeyframeOptions
    {
        float offset = 0.0f;
        bool hasOffset = false;
        std::wstring easing;
        bool hasX = false;
        bool hasY = false;
        bool hasWidth = false;
        bool hasHeight = false;
        bool hasRotate = false;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float rotate = 0.0f;
        bool hasFontSize = false;
        bool hasFontWeight = false;
        bool hasLetterSpacing = false;
        bool hasFontColor = false;
        float fontSize = 12.0f;
        float fontWeight = 400.0f;
        float letterSpacing = 0.0f;
        float fontColorR = 0.0f;
        float fontColorG = 0.0f;
        float fontColorB = 0.0f;
        float fontAlpha = 255.0f;

        bool HasAnyProps() const
        {
            return hasX || hasY || hasWidth || hasHeight || hasRotate ||
                   hasFontSize || hasFontWeight || hasLetterSpacing || hasFontColor;
        }

        bool HasTextProps() const
        {
            return hasFontSize || hasFontWeight || hasLetterSpacing || hasFontColor;
        }
    };

    struct AnimationOptions
    {
        std::wstring id;
        int duration = 250;
        std::wstring easing = L"linear";
        int iterationCount = 1;
        bool iterationInfinite = false;
        bool hasIterationCount = false;
        bool iterationCountInvalid = false;
        bool hasKeyframes = false;
        bool keyframesInvalid = false;
        std::wstring keyframesError;
        bool tweenInvalid = false;
        std::wstring tweenError;
        std::vector<AnimationKeyframeOptions> keyframes;
        bool hasX = false;
        bool hasY = false;
        bool hasWidth = false;
        bool hasHeight = false;
        bool hasRotate = false;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float rotate = 0.0f;
        bool fromHasX = false;
        bool fromHasY = false;
        bool fromHasWidth = false;
        bool fromHasHeight = false;
        bool fromHasRotate = false;
        float fromX = 0.0f;
        float fromY = 0.0f;
        float fromWidth = 0.0f;
        float fromHeight = 0.0f;
        float fromRotate = 0.0f;
        bool hasFontSize = false;
        bool hasFontWeight = false;
        bool hasLetterSpacing = false;
        bool hasFontColor = false;
        float fontSize = 12.0f;
        float fontWeight = 400.0f;
        float letterSpacing = 0.0f;
        float fontColorR = 0.0f;
        float fontColorG = 0.0f;
        float fontColorB = 0.0f;
        float fontAlpha = 255.0f;
        bool fromHasFontSize = false;
        bool fromHasFontWeight = false;
        bool fromHasLetterSpacing = false;
        bool fromHasFontColor = false;
        float fromFontSize = 12.0f;
        float fromFontWeight = 400.0f;
        float fromLetterSpacing = 0.0f;
        float fromFontColorR = 0.0f;
        float fromFontColorG = 0.0f;
        float fromFontColorB = 0.0f;
        float fromFontAlpha = 255.0f;

        bool HasAnyToProps() const
        {
            if (hasKeyframes)
            {
                for (const AnimationKeyframeOptions &kf : keyframes)
                {
                    if (kf.HasAnyProps())
                        return true;
                }
                return false;
            }
            return hasX || hasY || hasWidth || hasHeight || hasRotate;
        }

        bool HasAnyTextToProps() const
        {
            for (const AnimationKeyframeOptions &kf : keyframes)
            {
                if (kf.HasTextProps())
                    return true;
            }
            return false;
        }

        bool UsesTweenMode() const
        {
            return !hasKeyframes;
        }
    };
} // namespace PropertyParser

namespace novadesk::scripting::quickjs::parser
{
    struct WidgetWindowOptions
    {
        std::wstring id = L"widget";
        int width = 0;
        int height = 0;
        bool hasWidth = false;
        bool hasHeight = false;
        int x = 0;
        int y = 0;
        bool hasX = false;
        bool hasY = false;
        bool draggable = true;
        bool hasDraggable = false;
        bool clickThrough = false;
        bool hasClickThrough = false;
        bool keepOnScreen = false;
        bool hasKeepOnScreen = false;
        bool snapEdges = true;
        bool hasSnapEdges = false;
        bool showInToolbar = false;
        bool hasShowInToolbar = false;
        std::wstring toolbarIcon;
        bool hasToolbarIcon = false;
        std::wstring toolbarTitle;
        bool hasToolbarTitle = false;
        bool show = true;
        bool hasShow = false;
        std::wstring backgroundColor = L"rgba(0,0,0,0)";
        COLORREF color = RGB(0, 0, 0);
        BYTE bgAlpha = 0;
        GradientInfo bgGradient;
        bool hasBackgroundColor = false;
        BYTE windowOpacity = 255;
        bool hasWindowOpacity = false;
        int zPos = -1;
        bool hasZPos = false;
        std::wstring scriptPath;
        bool hasScriptPath = false;
    };
} // namespace novadesk::scripting::quickjs::parser
