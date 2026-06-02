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
#include "../../render/AreaGraphElement.h"
#include "../../render/HistogramElement.h"
namespace PropertyParser
{
    using namespace Js;
    void ParseAreaGraphOptions(JSContext *ctx, JSValueConst obj, AreaGraphOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        JSValue dataVal = JS_GetPropertyStr(ctx, obj, "data");
        if (JS_IsArray(dataVal))
        {
            uint32_t len = 0;
            JSValue lenVal = JS_GetPropertyStr(ctx, dataVal, "length");
            JS_ToUint32(ctx, &len, lenVal);
            JS_FreeValue(ctx, lenVal);

            options.data.clear();
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue v = JS_GetPropertyUint32(ctx, dataVal, i);
                double d = 0.0;
                JS_ToFloat64(ctx, &d, v);
                options.data.push_back(static_cast<float>(d));
                JS_FreeValue(ctx, v);
            }
        }
        JS_FreeValue(ctx, dataVal);

        GetFloatProp(ctx, obj, "minValue", options.minValue);
        GetFloatProp(ctx, obj, "maxValue", options.maxValue);
        GetBoolProp(ctx, obj, "autoRange", options.autoRange);

        std::wstring lc = GetStringProp(ctx, obj, "lineColor");
        if (!lc.empty())
        {
            GradientInfo parsedGradient;
            BYTE parsedAlpha = 255;
            if (ParseGradientString(lc, parsedGradient))
            {
                options.lineGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(lc, options.lineColor, parsedAlpha))
            {
                options.lineGradient = GradientInfo();
            }
        }
        GetFloatProp(ctx, obj, "lineWidth", options.lineWidth);

        std::wstring fc = GetStringProp(ctx, obj, "fillColor");
        if (!fc.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(fc, parsedGradient))
            {
                options.fillGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(fc, options.fillColor, options.fillAlpha))
            {
                options.fillGradient = GradientInfo();
            }
        }
        GetIntProp(ctx, obj, "maxPoints", options.maxPoints);

        std::wstring gc = GetStringProp(ctx, obj, "gridColor");
        if (!gc.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(gc, parsedGradient))
            {
                options.gridGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(gc, options.gridColor, options.gridAlpha))
            {
                options.gridGradient = GradientInfo();
            }
        }

        GetIntProp(ctx, obj, "gridX", options.gridX);
        GetIntProp(ctx, obj, "gridY", options.gridY);
        GetBoolProp(ctx, obj, "gridVisible", options.gridVisible);
        GetBoolProp(ctx, obj, "graphStartLeft", options.graphStartLeft);
        GetBoolProp(ctx, obj, "flip", options.flip);
    }
    void ParseBarOptions(JSContext *ctx, JSValueConst obj, BarOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        GetFloatProp(ctx, obj, "value", options.value);

        std::wstring orientation = GetStringProp(ctx, obj, "orientation");
        if (orientation == L"vertical")
            options.orientation = BAR_VERTICAL;
        else if (orientation == L"horizontal")
            options.orientation = BAR_HORIZONTAL;
        GetIntProp(ctx, obj, "barCornerRadius", options.barCornerRadius);

        std::wstring bc = GetStringProp(ctx, obj, "barColor");
        ParseGradientOrColor(bc, options.barColor, options.barAlpha, options.barGradient, options.hasBarColor);
    }

    void ParseLineOptions(JSContext *ctx, JSValueConst obj, LineOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetIntProp(ctx, obj, "lineCount", options.lineCount);
        if (options.lineCount < 1)
        {
            options.lineCount = 1;
        }

        GetFloatProp(ctx, obj, "lineWidth", options.lineWidth);
        if (options.lineWidth < 1.0f)
        {
            options.lineWidth = 1.0f;
        }
        GetIntProp(ctx, obj, "maxPoints", options.maxPoints);
        if (options.maxPoints < 0)
        {
            options.maxPoints = 0;
        }

        GetBoolProp(ctx, obj, "horizontalLines", options.horizontalLines);
        std::wstring horizontalLineColor = GetStringProp(ctx, obj, "horizontalLineColor");
        if (!horizontalLineColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(horizontalLineColor, parsedGradient))
            {
                options.horizontalLineGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(horizontalLineColor, options.horizontalLineColor, options.horizontalLineAlpha))
            {
                options.horizontalLineGradient = GradientInfo();
            }
        }

        std::wstring graphStart = GetStringProp(ctx, obj, "graphStart");
        std::transform(graphStart.begin(), graphStart.end(), graphStart.begin(), ::towlower);
        if (graphStart == L"left")
        {
            options.graphStartLeft = true;
        }
        else if (graphStart == L"right")
        {
            options.graphStartLeft = false;
        }

        std::wstring graphOrientation = GetStringProp(ctx, obj, "graphOrientation");
        std::transform(graphOrientation.begin(), graphOrientation.end(), graphOrientation.begin(), ::towlower);
        if (graphOrientation == L"horizontal")
        {
            options.graphHorizontalOrientation = true;
        }
        else if (graphOrientation == L"vertical")
        {
            options.graphHorizontalOrientation = false;
        }

        GetBoolProp(ctx, obj, "flip", options.flip);

        std::wstring transformStroke = GetStringProp(ctx, obj, "transformStroke");
        std::transform(transformStroke.begin(), transformStroke.end(), transformStroke.begin(), ::towlower);
        if (transformStroke == L"fixed")
        {
            options.transformStroke = D2D1_STROKE_TRANSFORM_TYPE_FIXED;
        }
        else if (transformStroke == L"normal")
        {
            options.transformStroke = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
        }

        GetBoolProp(ctx, obj, "autoRange", options.autoRange);
        GetFloatProp(ctx, obj, "rangeMin", options.scaleMin);
        GetFloatProp(ctx, obj, "rangeMax", options.scaleMax);
        if (options.scaleMax < options.scaleMin)
        {
            std::swap(options.scaleMin, options.scaleMax);
        }
        if (fabsf(options.scaleMax - options.scaleMin) < 0.000001f)
        {
            options.scaleMax = options.scaleMin + 1.0f;
        }

        const size_t desiredLineCount = (size_t)options.lineCount;

        if (options.dataSets.size() != desiredLineCount)
        {
            options.dataSets.resize(desiredLineCount);
        }

        if (options.lineColors.size() < desiredLineCount)
        {
            options.lineColors.resize(desiredLineCount, RGB(255, 255, 255));
        }
        else if (options.lineColors.size() > desiredLineCount)
        {
            options.lineColors.resize(desiredLineCount);
        }

        if (options.lineAlphas.size() < desiredLineCount)
        {
            options.lineAlphas.resize(desiredLineCount, 255);
        }
        else if (options.lineAlphas.size() > desiredLineCount)
        {
            options.lineAlphas.resize(desiredLineCount);
        }

        if (options.lineGradients.size() < desiredLineCount)
        {
            options.lineGradients.resize(desiredLineCount);
        }
        else if (options.lineGradients.size() > desiredLineCount)
        {
            options.lineGradients.resize(desiredLineCount);
        }

        if (options.scaleValues.size() < desiredLineCount)
        {
            options.scaleValues.resize(desiredLineCount, 1.0f);
        }
        else if (options.scaleValues.size() > desiredLineCount)
        {
            options.scaleValues.resize(desiredLineCount);
        }

        for (int i = 0; i < options.lineCount; ++i)
        {
            std::wstring dataKey = (i == 0) ? L"data" : (L"data" + std::to_wstring(i + 1));
            std::vector<float> data;
            std::string dataKeyUtf8 = Utils::ToString(dataKey);
            if (GetFloatArrayProp(ctx, obj, dataKeyUtf8.c_str(), data, 1))
            {
                options.dataSets[(size_t)i] = std::move(data);
            }

            std::wstring colorKey = (i == 0) ? L"lineColor" : (L"lineColor" + std::to_wstring(i + 1));
            std::string colorKeyUtf8 = Utils::ToString(colorKey);
            std::wstring colorValue = GetStringProp(ctx, obj, colorKeyUtf8.c_str());
            if (!colorValue.empty())
            {
                GradientInfo parsedGradient;
                if (ParseGradientString(colorValue, parsedGradient))
                {
                    options.lineGradients[(size_t)i] = parsedGradient;
                }
                else if (ColorUtil::ParseRGBA(colorValue, options.lineColors[(size_t)i], options.lineAlphas[(size_t)i]))
                {
                    options.lineGradients[(size_t)i] = GradientInfo();
                }
            }

            std::wstring scaleKey = (i == 0) ? L"lineScale" : (L"lineScale" + std::to_wstring(i + 1));
            std::string scaleKeyUtf8 = Utils::ToString(scaleKey);
            float scaleValue = 1.0f;
            if (GetFloatProp(ctx, obj, scaleKeyUtf8.c_str(), scaleValue))
            {
                if (!std::isfinite(scaleValue))
                {
                    scaleValue = 1.0f;
                }
                options.scaleValues[(size_t)i] = scaleValue;
            }
        }
    }

    void ParseHistogramOptions(JSContext *ctx, JSValueConst obj, HistogramOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetFloatArrayPropAllowEmpty(ctx, obj, "data", options.data);
        GetFloatArrayPropAllowEmpty(ctx, obj, "data2", options.data2);

        GetBoolProp(ctx, obj, "autoRange", options.autoRange);
        GetBoolProp(ctx, obj, "flip", options.flip);

        std::wstring graphStart = GetStringProp(ctx, obj, "graphStart");
        std::transform(graphStart.begin(), graphStart.end(), graphStart.begin(), ::towlower);
        if (graphStart == L"left")
            options.graphStartLeft = true;
        else if (graphStart == L"right")
            options.graphStartLeft = false;

        std::wstring graphOrientation = GetStringProp(ctx, obj, "graphOrientation");
        std::transform(graphOrientation.begin(), graphOrientation.end(), graphOrientation.begin(), ::towlower);
        if (graphOrientation == L"horizontal")
            options.graphHorizontalOrientation = true;
        else if (graphOrientation == L"vertical")
            options.graphHorizontalOrientation = false;

        std::wstring primaryColor = GetStringProp(ctx, obj, "primaryColor");
        if (!primaryColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(primaryColor, parsedGradient))
            {
                options.primaryGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(primaryColor, options.primaryColor, options.primaryAlpha))
            {
                options.primaryGradient = GradientInfo();
            }
        }

        std::wstring secondaryColor = GetStringProp(ctx, obj, "secondaryColor");
        if (!secondaryColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(secondaryColor, parsedGradient))
            {
                options.secondaryGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(secondaryColor, options.secondaryColor, options.secondaryAlpha))
            {
                options.secondaryGradient = GradientInfo();
            }
        }

        std::wstring bothColor = GetStringProp(ctx, obj, "bothColor");
        if (!bothColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(bothColor, parsedGradient))
            {
                options.bothGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(bothColor, options.bothColor, options.bothAlpha))
            {
                options.bothGradient = GradientInfo();
            }
        }

    }

    void ParseRoundLineOptions(JSContext *ctx, JSValueConst obj, RoundLineOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetFloatProp(ctx, obj, "value", options.value);
        GetIntProp(ctx, obj, "radius", options.radius);
        GetIntProp(ctx, obj, "thickness", options.thickness);
        GetIntProp(ctx, obj, "endThickness", options.endThickness);
        GetFloatProp(ctx, obj, "startAngle", options.startAngle);
        GetFloatProp(ctx, obj, "totalAngle", options.totalAngle);
        GetBoolProp(ctx, obj, "clockwise", options.clockwise);

        std::wstring capType = GetStringProp(ctx, obj, "capType");
        if (capType == L"round")
            options.startCap = options.endCap = ROUNDLINE_CAP_ROUND;
        else if (capType == L"flat")
            options.startCap = options.endCap = ROUNDLINE_CAP_FLAT;

        std::wstring startCap = GetStringProp(ctx, obj, "startCap");
        if (startCap == L"round")
            options.startCap = ROUNDLINE_CAP_ROUND;
        else if (startCap == L"flat")
            options.startCap = ROUNDLINE_CAP_FLAT;

        std::wstring endCap = GetStringProp(ctx, obj, "endCap");
        if (endCap == L"round")
            options.endCap = ROUNDLINE_CAP_ROUND;
        else if (endCap == L"flat")
            options.endCap = ROUNDLINE_CAP_FLAT;

        std::vector<float> dash;
        if (GetFloatArrayProp(ctx, obj, "dashArray", dash, 1))
            options.dashArray = dash;

        GetIntProp(ctx, obj, "ticks", options.ticks);

        std::wstring lc = GetStringProp(ctx, obj, "lineColor");
        ParseGradientOrColor(lc, options.lineColor, options.lineAlpha, options.lineGradient, options.hasLineColor);

        std::wstring lcb = GetStringProp(ctx, obj, "lineColorBg");
        ParseGradientOrColor(lcb, options.lineColorBg, options.lineAlphaBg, options.lineGradientBg, options.hasLineColorBg);
    }
    void ApplyAreaGraphOptions(AreaGraphElement *element, const AreaGraphOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        element->SetData(options.data);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);
        element->SetAutoRange(options.autoRange);
        element->SetLineColor(options.lineColor);
        element->SetLineGradient(options.lineGradient);
        element->SetLineWidth(options.lineWidth);
        element->SetFillColor(options.fillColor, options.fillAlpha);
        element->SetFillGradient(options.fillGradient);
        element->SetMaxPoints(options.maxPoints);
        element->SetGridColor(options.gridColor, options.gridAlpha);
        element->SetGridGradient(options.gridGradient);
        element->SetGridVisible(options.gridVisible);
        element->SetGridXSpacing(options.gridX);
        element->SetGridYSpacing(options.gridY);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetFlip(options.flip);
    }

    void ApplyBarOptions(BarElement *element, const BarOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetOrientation(options.orientation);
        element->SetBarCornerRadius(options.barCornerRadius);
        if (options.barGradient.type != GRADIENT_NONE)
            element->SetBarGradient(options.barGradient);
        else if (options.hasBarColor)
            element->SetBarColor(options.barColor, options.barAlpha);
    }

    void ApplyLineOptions(LineElement *element, const LineOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetLineCount(options.lineCount);
        element->SetDataSets(options.dataSets);
        element->SetLineColors(options.lineColors, options.lineAlphas);
        element->SetLineGradients(options.lineGradients);
        element->SetScaleValues(options.scaleValues);
        element->SetLineWidth(options.lineWidth);
        element->SetMaxPoints(options.maxPoints);
        element->SetHorizontalLines(options.horizontalLines);
        element->SetHorizontalLineColor(options.horizontalLineColor, options.horizontalLineAlpha);
        element->SetHorizontalLineGradient(options.horizontalLineGradient);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetGraphHorizontalOrientation(options.graphHorizontalOrientation);
        element->SetFlip(options.flip);
        element->SetStrokeTransformType(options.transformStroke);
        element->SetAutoRange(options.autoRange);
        element->SetScaleRange(options.scaleMin, options.scaleMax);
    }

    void ApplyHistogramOptions(HistogramElement *element, const HistogramOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        element->SetData(options.data);
        element->SetData2(options.data2);
        element->SetAutoRange(options.autoRange);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetGraphHorizontalOrientation(options.graphHorizontalOrientation);
        element->SetFlip(options.flip);
        element->SetPrimaryColor(options.primaryColor, options.primaryAlpha);
        element->SetSecondaryColor(options.secondaryColor, options.secondaryAlpha);
        element->SetBothColor(options.bothColor, options.bothAlpha);
        element->SetPrimaryGradient(options.primaryGradient);
        element->SetSecondaryGradient(options.secondaryGradient);
        element->SetBothGradient(options.bothGradient);
    }

    void ApplyRoundLineOptions(RoundLineElement *element, const RoundLineOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetRadius(options.radius);
        element->SetThickness(options.thickness);
        element->SetEndThickness(options.endThickness);
        element->SetStartAngle(options.startAngle);
        element->SetTotalAngle(options.totalAngle);
        element->SetClockwise(options.clockwise);
        element->SetStartCap(options.startCap);
        element->SetEndCap(options.endCap);
        element->SetDashArray(options.dashArray);
        element->SetTicks(options.ticks);
        if (options.lineGradient.type != GRADIENT_NONE)
            element->SetLineGradient(options.lineGradient);
        else if (options.hasLineColor)
            element->SetLineColor(options.lineColor, options.lineAlpha);
        if (options.lineGradientBg.type != GRADIENT_NONE)
            element->SetLineGradientBg(options.lineGradientBg);
        else if (options.hasLineColorBg)
            element->SetLineColorBg(options.lineColorBg, options.lineAlphaBg);
    }
    void PreFillAreaGraphOptions(AreaGraphOptions &options, AreaGraphElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);
        options.data = element->GetData();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();
        options.autoRange = element->GetAutoRange();
        options.lineColor = element->GetLineColor();
        options.lineGradient = element->GetLineGradient();
        options.lineWidth = element->GetLineWidth();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.fillGradient = element->GetFillGradient();
        options.maxPoints = element->GetMaxPoints();
        options.gridColor = element->GetGridColor();
        options.gridAlpha = element->GetGridAlpha();
        options.gridGradient = element->GetGridGradient();
        options.gridVisible = element->GetGridVisible();
        options.gridX = element->GetGridXSpacing();
        options.gridY = element->GetGridYSpacing();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.flip = element->GetFlip();
    }

    void PreFillBarOptions(BarOptions &options, BarElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.value = element->GetValue();
        options.orientation = element->GetOrientation();
        options.barCornerRadius = element->GetBarCornerRadius();
        options.hasBarColor = element->HasBarColor();
        options.barColor = element->GetBarColor();
        options.barAlpha = element->GetBarAlpha();
        options.barGradient = element->GetBarGradient();
    }

    void PreFillLineOptions(LineOptions &options, LineElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }

        options.lineCount = element->GetLineCount();
        options.dataSets = element->GetDataSets();
        options.lineColors = element->GetLineColors();
        options.lineAlphas = element->GetLineAlphas();
        options.lineGradients = element->GetLineGradients();
        options.scaleValues = element->GetScaleValues();
        options.lineWidth = element->GetLineWidth();
        options.maxPoints = element->GetMaxPoints();
        options.horizontalLines = element->GetHorizontalLines();
        options.horizontalLineColor = element->GetHorizontalLineColor();
        options.horizontalLineAlpha = element->GetHorizontalLineAlpha();
        options.horizontalLineGradient = element->GetHorizontalLineGradient();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.graphHorizontalOrientation = element->GetGraphHorizontalOrientation();
        options.flip = element->GetFlip();
        options.transformStroke = element->GetStrokeTransformType();
        options.autoRange = element->GetAutoRange();
        options.scaleMin = element->GetScaleMin();
        options.scaleMax = element->GetScaleMax();
    }

    void PreFillHistogramOptions(HistogramOptions &options, HistogramElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        options.data = element->GetData();
        options.data2 = element->GetData2();
        options.autoRange = element->GetAutoRange();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.graphHorizontalOrientation = element->GetGraphHorizontalOrientation();
        options.flip = element->GetFlip();

        options.primaryColor = element->GetPrimaryColor();
        options.primaryAlpha = element->GetPrimaryAlpha();
        options.secondaryColor = element->GetSecondaryColor();
        options.secondaryAlpha = element->GetSecondaryAlpha();
        options.bothColor = element->GetBothColor();
        options.bothAlpha = element->GetBothAlpha();
        options.primaryGradient = element->GetPrimaryGradient();
        options.secondaryGradient = element->GetSecondaryGradient();
        options.bothGradient = element->GetBothGradient();
    }

    void PreFillRoundLineOptions(RoundLineOptions &options, RoundLineElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.value = element->GetValue();
        options.radius = element->GetRadius();
        options.thickness = element->GetThickness();
        options.endThickness = element->GetEndThickness();
        options.startAngle = element->GetStartAngle();
        options.totalAngle = element->GetTotalAngle();
        options.clockwise = element->IsClockwise();
        options.startCap = element->GetStartCap();
        options.endCap = element->GetEndCap();
        options.dashArray = element->GetDashArray();
        options.ticks = element->GetTicks();
        options.hasLineColor = element->HasLineColor();
        options.lineColor = element->GetLineColor();
        options.lineAlpha = element->GetLineAlpha();
        options.hasLineColorBg = element->HasLineColorBg();
        options.lineColorBg = element->GetLineColorBg();
        options.lineAlphaBg = element->GetLineAlphaBg();
        options.lineGradient = element->GetLineGradient();
        options.lineGradientBg = element->GetLineGradientBg();
    }
}
