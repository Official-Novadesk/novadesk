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
    void ParseShapeOptions(JSContext *ctx, JSValueConst obj, ShapeOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        std::wstring shapeType = GetStringProp(ctx, obj, "type");
        if (!shapeType.empty())
            options.shapeType = shapeType;
        GetFloatProp(ctx, obj, "strokeWidth", options.strokeWidth);

        std::wstring stroke = GetStringProp(ctx, obj, "strokeColor");
        bool hasStroke = false;
        ParseGradientOrColor(stroke, options.strokeColor, options.strokeAlpha, options.strokeGradient, hasStroke);

        std::wstring fill = GetStringProp(ctx, obj, "fillColor");
        bool hasFill = false;
        ParseGradientOrColor(fill, options.fillColor, options.fillAlpha, options.fillGradient, hasFill);

        bool hasRadiusX = GetFloatProp(ctx, obj, "radiusX", options.radiusX);
        bool hasRadiusY = GetFloatProp(ctx, obj, "radiusY", options.radiusY);
        float uniformRadius = 0.0f;
        if (GetFloatProp(ctx, obj, "radius", uniformRadius))
        {
            // Legacy alias: radius applies to both X/Y when specific values are not provided.
            if (!hasRadiusX)
                options.radiusX = uniformRadius;
            if (!hasRadiusY)
                options.radiusY = uniformRadius;
        }
        GetFloatProp(ctx, obj, "startX", options.startX);
        GetFloatProp(ctx, obj, "startY", options.startY);
        GetFloatProp(ctx, obj, "endX", options.endX);
        GetFloatProp(ctx, obj, "endY", options.endY);
        std::wstring curveType = GetStringProp(ctx, obj, "curveType");
        if (!curveType.empty())
            options.curveType = curveType;
        GetFloatProp(ctx, obj, "controlX", options.controlX);
        GetFloatProp(ctx, obj, "controlY", options.controlY);
        GetFloatProp(ctx, obj, "control2X", options.control2X);
        GetFloatProp(ctx, obj, "control2Y", options.control2Y);
        GetFloatProp(ctx, obj, "startAngle", options.startArcAngle);
        GetFloatProp(ctx, obj, "endAngle", options.endArcAngle);
        GetBoolProp(ctx, obj, "clockwise", options.arcClockwise);
        std::wstring pathData = GetStringProp(ctx, obj, "pathData");
        if (!pathData.empty())
            options.pathData = pathData;

        std::wstring cap = GetStringProp(ctx, obj, "strokeStartCap");
        if (!cap.empty())
            options.strokeStartCap = GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeEndCap");
        if (!cap.empty())
            options.strokeEndCap = GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeDashCap");
        if (!cap.empty())
            options.strokeDashCap = GetCapStyle(cap);

        std::wstring join = GetStringProp(ctx, obj, "strokeLineJoin");
        if (!join.empty())
            options.strokeLineJoin = GetLineJoin(join);
        GetFloatProp(ctx, obj, "strokeDashOffset", options.strokeDashOffset);
        GetFloatArrayProp(ctx, obj, "strokeDashes", options.strokeDashes, 1);

        // Combine shape options (legacy-only): type/base/ops/consume.
        std::wstring lowerType = options.shapeType;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::towlower);
        options.isCombine = (lowerType == L"combine");
        std::wstring combineBaseId = GetStringProp(ctx, obj, "base");
        if (!combineBaseId.empty())
            options.combineBaseId = combineBaseId;
        options.hasCombineConsumeAll = GetBoolProp(ctx, obj, "consume", options.combineConsumeAll);

        JSValue ops = JS_GetPropertyStr(ctx, obj, "ops");
        if (JS_IsArray(ops))
        {
            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, ops, "length");
            if (JS_ToUint32(ctx, &len, lenV) == 0)
            {
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue it = JS_GetPropertyUint32(ctx, ops, i);
                    if (JS_IsObject(it))
                    {
                        ShapeCombineOp op;
                        op.id = GetStringProp(ctx, it, "id");
                        op.mode = ParseCombineMode(GetStringProp(ctx, it, "mode"));
                        if (GetBoolProp(ctx, it, "consume", op.consume))
                            op.hasConsume = true;
                        if (!op.id.empty())
                            options.combineOps.push_back(op);
                    }
                    JS_FreeValue(ctx, it);
                }
            }
            JS_FreeValue(ctx, lenV);
        }
        JS_FreeValue(ctx, ops);
    }
    void ApplyShapeOptions(ShapeElement *element, const ShapeOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);

        if (options.strokeGradient.type != GRADIENT_NONE)
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
            element->SetStrokeGradient(options.strokeGradient);
        }
        else
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
        }

        if (options.fillGradient.type != GRADIENT_NONE)
            element->SetFillGradient(options.fillGradient);
        else
            element->SetFill(options.fillColor, options.fillAlpha);

        element->SetRadii(options.radiusX, options.radiusY);
        element->SetLinePoints(options.startX, options.startY, options.endX, options.endY);
        element->SetArcParams(options.startArcAngle, options.endArcAngle, options.arcClockwise);
        element->SetPathData(options.pathData);
        element->SetCurveParams(
            options.startX,
            options.startY,
            options.controlX,
            options.controlY,
            options.control2X,
            options.control2Y,
            options.endX,
            options.endY,
            options.curveType);

        element->SetStrokeStyle(
            options.strokeStartCap,
            options.strokeEndCap,
            options.strokeDashCap,
            options.strokeLineJoin,
            options.strokeDashOffset,
            options.strokeDashes);
    }
    void PreFillShapeOptions(ShapeOptions &options, ShapeElement *element)
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
        options.strokeWidth = element->GetStrokeWidth();
        options.strokeColor = element->GetStrokeColor();
        options.strokeAlpha = element->GetStrokeAlpha();
        options.strokeGradient = element->GetStrokeGradient();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.fillGradient = element->GetFillGradient();
        options.radiusX = element->GetRadiusX();
        options.radiusY = element->GetRadiusY();
        options.startX = element->GetStartX();
        options.startY = element->GetStartY();
        options.endX = element->GetEndX();
        options.endY = element->GetEndY();
        options.curveType = element->GetCurveType();
        options.controlX = element->GetControlX();
        options.controlY = element->GetControlY();
        options.control2X = element->GetControl2X();
        options.control2Y = element->GetControl2Y();
        options.startArcAngle = element->GetStartAngle();
        options.endArcAngle = element->GetEndAngle();
        options.arcClockwise = element->IsClockwise();
        options.pathData = element->GetPathData();
        options.strokeStartCap = element->GetStrokeStartCap();
        options.strokeEndCap = element->GetStrokeEndCap();
        options.strokeDashCap = element->GetStrokeDashCap();
        options.strokeLineJoin = element->GetStrokeLineJoin();
        options.strokeDashOffset = element->GetStrokeDashOffset();
        options.strokeDashes = element->GetStrokeDashes();
    }
}
