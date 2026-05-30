/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BoxBorderPaint.h"
#include "Direct2DHelper.h"
#include <algorithm>
#include <cmath>
#include <vector>

D2D1_ROUNDED_RECT BoxBorderPaint::BuildBorderGeometryRect(const D2D1_ROUNDED_RECT& elementRect,
    const BoxBorderPaintParams& params)
{
    D2D1_ROUNDED_RECT borderRect = elementRect;
    float radiusX = params.elementRadiusX;
    float radiusY = params.elementRadiusY;
    const float bw = params.strokeWidth;
    if (params.position == BoxBorder::Position::Outside)
    {
        borderRect.rect.left -= bw;
        borderRect.rect.top -= bw;
        borderRect.rect.right += bw;
        borderRect.rect.bottom += bw;
        if (radiusX > 0.0f)
            radiusX += bw;
        if (radiusY > 0.0f)
            radiusY += bw;
    }
    else if (params.position == BoxBorder::Position::Center)
    {
        const float half = bw * 0.5f;
        borderRect.rect.left -= half;
        borderRect.rect.top -= half;
        borderRect.rect.right += half;
        borderRect.rect.bottom += half;
        if (radiusX > 0.0f)
            radiusX += half;
        if (radiusY > 0.0f)
            radiusY += half;
    }
    borderRect.radiusX = radiusX;
    borderRect.radiusY = radiusY;
    return borderRect;
}

void BoxBorderPaint::PaintForElement(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& elementRect,
    const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush)
{
    if (!context || !strokeBrush || params.strokeWidth <= 0.0f)
        return;
    const D2D1_ROUNDED_RECT borderRect = BuildBorderGeometryRect(elementRect, params);
    if (params.position == BoxBorder::Position::Inside)
    {
        D2D1_LAYER_PARAMETERS1 clipParams = D2D1::LayerParameters1(
            elementRect.rect,
            nullptr,
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS1_NONE);
        context->PushLayer(clipParams, nullptr);
        Paint(context, borderRect, params, strokeBrush);
        context->PopLayer();
        return;
    }
    Paint(context, borderRect, params, strokeBrush);
}

void BoxBorderPaint::Paint(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& rect,
    const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush)
{
    if (params.styleTop == BoxBorder::Style::None && params.styleRight == BoxBorder::Style::None &&
        params.styleBottom == BoxBorder::Style::None && params.styleLeft == BoxBorder::Style::None)
        return;
    if (params.styleTop == BoxBorder::Style::Hidden && params.styleRight == BoxBorder::Style::Hidden &&
        params.styleBottom == BoxBorder::Style::Hidden && params.styleLeft == BoxBorder::Style::Hidden)
        return;
    const bool sameStyle = (params.styleTop == params.styleRight &&
        params.styleRight == params.styleBottom &&
        params.styleBottom == params.styleLeft);
    auto getStrokeStyle = [&](BoxBorder::Style style, ID2D1StrokeStyle1** outStyle)
        {
            *outStyle = nullptr;
            ID2D1Factory1* factory = Direct2D::GetFactory();
            if (!factory)
                return;
            D2D1_STROKE_STYLE_PROPERTIES1 props = {};
            props.startCap = params.strokeStartCap;
            props.endCap = params.strokeEndCap;
            props.dashCap = params.strokeDashCap;
            props.lineJoin = params.strokeLineJoin;
            props.miterLimit = 10.0f;
            props.dashOffset = params.strokeDashOffset;
            props.transformType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
            if (style == BoxBorder::Style::Dotted)
            {
                props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
                props.startCap = D2D1_CAP_STYLE_ROUND;
                props.endCap = D2D1_CAP_STYLE_ROUND;
                props.dashCap = D2D1_CAP_STYLE_ROUND;
                float dashes[] = { 0.0f, 2.0f };
                factory->CreateStrokeStyle(props, dashes, 2, outStyle);
            }
            else if (style == BoxBorder::Style::Dashed)
            {
                props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
                float dashes[] = { 3.0f, 1.0f };
                factory->CreateStrokeStyle(props, dashes, 2, outStyle);
            }
            else
            {
                props.dashStyle = D2D1_DASH_STYLE_SOLID;
                factory->CreateStrokeStyle(props, nullptr, 0, outStyle);
            }
        };
    auto insetRoundedRect = [](D2D1_ROUNDED_RECT rr, float amount) -> D2D1_ROUNDED_RECT
        {
            rr.rect.left += amount;
            rr.rect.top += amount;
            rr.rect.right -= amount;
            rr.rect.bottom -= amount;
            if (rr.radiusX > 0.0f)
                rr.radiusX = std::max(0.0f, rr.radiusX - amount);
            if (rr.radiusY > 0.0f)
                rr.radiusY = std::max(0.0f, rr.radiusY - amount);
            return rr;
        };
    auto strokePathForBorder = [&](const D2D1_ROUNDED_RECT& outer, float strokeWidth) -> D2D1_ROUNDED_RECT
        {
            // `outer` is the outer edge of the border band; stroke is centered half a width inward.
            return insetRoundedRect(outer, params.strokeWidth * 0.5f);
        };
    auto drawStyleRect = [&](const D2D1_ROUNDED_RECT& r, BoxBorder::Style style, ID2D1Brush* brush, float width)
        {
            if (style == BoxBorder::Style::None || style == BoxBorder::Style::Hidden)
                return;
            Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> sstyle;
            getStrokeStyle(style, &sstyle);
            if (style == BoxBorder::Style::Double)
            {
                float third = width / 3.0f;
                if (params.position == BoxBorder::Position::Inside)
                {
                    D2D1_ROUNDED_RECT r1 = insetRoundedRect(r, third * 0.5f);
                    D2D1_ROUNDED_RECT r2 = insetRoundedRect(r, width - third * 0.5f);
                    if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                    {
                        context->DrawRectangle(r1.rect, brush, third, sstyle.Get());
                        context->DrawRectangle(r2.rect, brush, third, sstyle.Get());
                    }
                    else
                    {
                        context->DrawRoundedRectangle(r1, brush, third, sstyle.Get());
                        context->DrawRoundedRectangle(r2, brush, third, sstyle.Get());
                    }
                }
                else if (params.position == BoxBorder::Position::Center)
                {
                    const D2D1_ROUNDED_RECT base = strokePathForBorder(r, width);
                    D2D1_ROUNDED_RECT r1 = insetRoundedRect(base, -third);
                    D2D1_ROUNDED_RECT r2 = insetRoundedRect(base, third);
                    if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                    {
                        context->DrawRectangle(r1.rect, brush, third, sstyle.Get());
                        context->DrawRectangle(r2.rect, brush, third, sstyle.Get());
                    }
                    else
                    {
                        context->DrawRoundedRectangle(r1, brush, third, sstyle.Get());
                        context->DrawRoundedRectangle(r2, brush, third, sstyle.Get());
                    }
                }
                else
                {
                    const D2D1_ROUNDED_RECT base = strokePathForBorder(r, width);
                    D2D1_ROUNDED_RECT r1 = insetRoundedRect(base, -third);
                    D2D1_ROUNDED_RECT r2 = insetRoundedRect(base, third);
                    if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                    {
                        context->DrawRectangle(r1.rect, brush, third, sstyle.Get());
                        context->DrawRectangle(r2.rect, brush, third, sstyle.Get());
                    }
                    else
                    {
                        context->DrawRoundedRectangle(r1, brush, third, sstyle.Get());
                        context->DrawRoundedRectangle(r2, brush, third, sstyle.Get());
                    }
                }
            }
            else if (style == BoxBorder::Style::Groove || style == BoxBorder::Style::Ridge ||
                style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset)
            {
                COLORREF baseColor = params.strokeColor;
                BYTE rC = GetRValue(baseColor), gC = GetGValue(baseColor), bC = GetBValue(baseColor);
                COLORREF lightC = RGB(std::min(255, rC + 50), std::min(255, gC + 50), std::min(255, bC + 50));
                COLORREF darkC = RGB(std::max(0, rC - 50), std::max(0, gC - 50), std::max(0, bC - 50));
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> lightBrush, darkBrush;
                context->CreateSolidColorBrush(Direct2D::ColorToD2D(lightC, params.strokeAlpha / 255.0f), &lightBrush);
                context->CreateSolidColorBrush(Direct2D::ColorToD2D(darkC, params.strokeAlpha / 255.0f), &darkBrush);
                ID2D1Brush* topLeftBrush = nullptr;
                ID2D1Brush* bottomRightBrush = nullptr;
                if (style == BoxBorder::Style::Groove)
                {
                    topLeftBrush = darkBrush.Get();
                    bottomRightBrush = lightBrush.Get();
                }
                else if (style == BoxBorder::Style::Ridge)
                {
                    topLeftBrush = lightBrush.Get();
                    bottomRightBrush = darkBrush.Get();
                }
                else if (style == BoxBorder::Style::Inset)
                {
                    topLeftBrush = darkBrush.Get();
                    bottomRightBrush = lightBrush.Get();
                }
                else if (style == BoxBorder::Style::Outset)
                {
                    topLeftBrush = lightBrush.Get();
                    bottomRightBrush = darkBrush.Get();
                }
                if (params.elementRadiusX == 0.0f && params.elementRadiusY == 0.0f)
                {
                    D2D1_ANTIALIAS_MODE oldMode = context->GetAntialiasMode();
                    context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                    float L = r.rect.left, T = r.rect.top;
                    float R = r.rect.right, B = r.rect.bottom;
                    float w = width;
                    auto drawPolygon = [&](const D2D1_POINT_2F* points, int count, ID2D1Brush* pBrush)
                        {
                            if (!pBrush)
                                return;
                            Microsoft::WRL::ComPtr<ID2D1Factory> factory;
                            context->GetFactory(&factory);
                            if (!factory)
                                return;
                            Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeom;
                            if (SUCCEEDED(factory->CreatePathGeometry(&pathGeom)))
                            {
                                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                                if (SUCCEEDED(pathGeom->Open(&sink)))
                                {
                                    sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
                                    sink->AddLines(points + 1, count - 1);
                                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                    sink->Close();
                                    context->FillGeometry(pathGeom.Get(), pBrush);
                                }
                            }
                        };
                    if (style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset)
                    {
                        ID2D1Brush* tbBrush = topLeftBrush;
                        ID2D1Brush* lrBrush = topLeftBrush;
                        ID2D1Brush* bbBrush = bottomRightBrush;
                        ID2D1Brush* rrBrush = bottomRightBrush;
                        D2D1_POINT_2F topPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(L + w, T + w) };
                        drawPolygon(topPts, 4, tbBrush);
                        D2D1_POINT_2F leftPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(L + w, T + w),
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(L, B) };
                        drawPolygon(leftPts, 4, lrBrush);
                        D2D1_POINT_2F bottomPts[] = {
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(R - w, B - w),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(L, B) };
                        drawPolygon(bottomPts, 4, bbBrush);
                        D2D1_POINT_2F rightPts[] = {
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(R - w, B - w) };
                        drawPolygon(rightPts, 4, rrBrush);
                    }
                    else if (style == BoxBorder::Style::Groove || style == BoxBorder::Style::Ridge)
                    {
                        float h = w * 0.5f;
                        ID2D1Brush* outerTopLeftBrush = (style == BoxBorder::Style::Groove) ? darkBrush.Get() : lightBrush.Get();
                        ID2D1Brush* innerTopLeftBrush = (style == BoxBorder::Style::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* outerBotRightBrush = (style == BoxBorder::Style::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* innerBotRightBrush = (style == BoxBorder::Style::Groove) ? darkBrush.Get() : lightBrush.Get();
                        D2D1_POINT_2F topOuterPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(L + h, T + h) };
                        drawPolygon(topOuterPts, 4, outerTopLeftBrush);
                        D2D1_POINT_2F topInnerPts[] = {
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(L + w, T + w) };
                        drawPolygon(topInnerPts, 4, innerTopLeftBrush);
                        D2D1_POINT_2F leftOuterPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(L + h, B - h),
                            D2D1::Point2F(L, B) };
                        drawPolygon(leftOuterPts, 4, outerTopLeftBrush);
                        D2D1_POINT_2F leftInnerPts[] = {
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(L + w, T + w),
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(L + h, B - h) };
                        drawPolygon(leftInnerPts, 4, innerTopLeftBrush);
                        D2D1_POINT_2F bottomOuterPts[] = {
                            D2D1::Point2F(L + h, B - h),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(L, B) };
                        drawPolygon(bottomOuterPts, 4, outerBotRightBrush);
                        D2D1_POINT_2F bottomInnerPts[] = {
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(R - w, B - w),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(L + h, B - h) };
                        drawPolygon(bottomInnerPts, 4, innerBotRightBrush);
                        D2D1_POINT_2F rightOuterPts[] = {
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(R - h, B - h) };
                        drawPolygon(rightOuterPts, 4, outerBotRightBrush);
                        D2D1_POINT_2F rightInnerPts[] = {
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(R - w, B - w) };
                        drawPolygon(rightInnerPts, 4, innerBotRightBrush);
                    }
                    context->SetAntialiasMode(oldMode);
                }
                else
                {
                    float half = width / 2.0f;
                    float L = r.rect.left, T = r.rect.top;
                    float R = r.rect.right, B = r.rect.bottom;
                    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
                    context->GetFactory(&d2dFactory);
                    if (d2dFactory)
                    {
                        const float padAlong = width * 4.0f + 100.0f;
                        const float dx = R - L;
                        const float dy = T - B;
                        const float len = sqrtf(dx * dx + dy * dy);
                        D2D1_POINT_2F pTR_ext = D2D1::Point2F(R, T);
                        D2D1_POINT_2F pBL_ext = D2D1::Point2F(L, B);
                        float tlPerpX = -padAlong;
                        float tlPerpY = -padAlong;
                        float brPerpX = padAlong;
                        float brPerpY = padAlong;
                        if (len > 0.001f)
                        {
                            const float ux = dx / len;
                            const float uy = dy / len;
                            // Half-plane clips must reach every corner. A fixed perpendicular
                            // extent only worked for small boxes; scale with the diagonal length.
                            const float padPerp = std::max(padAlong, len * 0.5f + width * 2.0f);
                            pTR_ext = D2D1::Point2F(R + ux * padAlong, T + uy * padAlong);
                            pBL_ext = D2D1::Point2F(L - ux * padAlong, B - uy * padAlong);
                            tlPerpX = uy * padPerp;
                            tlPerpY = -ux * padPerp;
                            brPerpX = -tlPerpX;
                            brPerpY = -tlPerpY;
                        }
                        Microsoft::WRL::ComPtr<ID2D1PathGeometry> tlClip;
                        {
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            if (SUCCEEDED(d2dFactory->CreatePathGeometry(&tlClip)) &&
                                SUCCEEDED(tlClip->Open(&sink)))
                            {
                                sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                                sink->AddLine(pTR_ext);
                                sink->AddLine(D2D1::Point2F(pTR_ext.x + tlPerpX, pTR_ext.y + tlPerpY));
                                sink->AddLine(D2D1::Point2F(pBL_ext.x + tlPerpX, pBL_ext.y + tlPerpY));
                                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                sink->Close();
                            }
                        }
                        Microsoft::WRL::ComPtr<ID2D1PathGeometry> brClip;
                        {
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            if (SUCCEEDED(d2dFactory->CreatePathGeometry(&brClip)) &&
                                SUCCEEDED(brClip->Open(&sink)))
                            {
                                sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                                sink->AddLine(pTR_ext);
                                sink->AddLine(D2D1::Point2F(pTR_ext.x + brPerpX, pTR_ext.y + brPerpY));
                                sink->AddLine(D2D1::Point2F(pBL_ext.x + brPerpX, pBL_ext.y + brPerpY));
                                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                sink->Close();
                            }
                        }
                        auto drawSplitRing = [&](const D2D1_ROUNDED_RECT& rr, float sw,
                            ID2D1Brush* tlBrush, ID2D1Brush* brBrush)
                            {
                                if (tlClip && tlBrush)
                                {
                                    auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), tlClip.Get());
                                    context->PushLayer(lp, nullptr);
                                    context->DrawRoundedRectangle(rr, tlBrush, sw, sstyle.Get());
                                    context->PopLayer();
                                }
                                if (brClip && brBrush)
                                {
                                    auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), brClip.Get());
                                    context->PushLayer(lp, nullptr);
                                    context->DrawRoundedRectangle(rr, brBrush, sw, sstyle.Get());
                                    context->PopLayer();
                                }
                            };
                        const D2D1_ROUNDED_RECT bandPath = strokePathForBorder(r, width);
                        if (style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset)
                        {
                            drawSplitRing(bandPath, width, topLeftBrush, bottomRightBrush);
                        }
                        else
                        {
                            D2D1_ROUNDED_RECT rOuter = bandPath;
                            D2D1_ROUNDED_RECT rInner = bandPath;
                            rOuter.rect.left -= half / 2;
                            rOuter.rect.top -= half / 2;
                            rOuter.rect.right += half / 2;
                            rOuter.rect.bottom += half / 2;
                            rInner.rect.left += half / 2;
                            rInner.rect.top += half / 2;
                            rInner.rect.right -= half / 2;
                            rInner.rect.bottom -= half / 2;
                            rOuter.radiusX = bandPath.radiusX + half / 2;
                            rOuter.radiusY = bandPath.radiusY + half / 2;
                            rInner.radiusX = std::max(0.0f, bandPath.radiusX - half / 2);
                            rInner.radiusY = std::max(0.0f, bandPath.radiusY - half / 2);
                            ID2D1Brush* outerTl = (style == BoxBorder::Style::Groove) ? darkBrush.Get() : lightBrush.Get();
                            ID2D1Brush* outerBr = (style == BoxBorder::Style::Groove) ? lightBrush.Get() : darkBrush.Get();
                            ID2D1Brush* innerTl = (style == BoxBorder::Style::Groove) ? lightBrush.Get() : darkBrush.Get();
                            ID2D1Brush* innerBr = (style == BoxBorder::Style::Groove) ? darkBrush.Get() : lightBrush.Get();
                            drawSplitRing(rOuter, half, outerTl, outerBr);
                            drawSplitRing(rInner, half, innerTl, innerBr);
                        }
                    }
                }
            }
            else if (style == BoxBorder::Style::Dotted)
            {
                const D2D1_ROUNDED_RECT dotPath = strokePathForBorder(r, width);
                float L = dotPath.rect.left, T = dotPath.rect.top;
                float R = dotPath.rect.right, B = dotPath.rect.bottom;
                float rx = std::min(dotPath.radiusX, (R - L) * 0.5f);
                float ry = std::min(dotPath.radiusY, (B - T) * 0.5f);
                float L_top = R - L - 2.0f * rx;
                float L_bottom = L_top;
                float L_left = B - T - 2.0f * ry;
                float L_right = L_left;
                float L_arc = (rx > 0.0f || ry > 0.0f)
                    ? 1.570796f * sqrtf((rx * rx + ry * ry) / 2.0f)
                    : 0.0f;
                float perimeter = L_top + L_right + L_bottom + L_left + 4.0f * L_arc;
                if (perimeter > 0.001f)
                {
                    float ideal_spacing = width * 2.0f;
                    float r_dot = width / 2.0f;
                    auto getPoint = [&](float t, float& x, float& y)
                        {
                            if (t < 0.0f)
                                t = 0.0f;
                            if (t > perimeter)
                                t = perimeter;
                            if (t < L_top)
                            {
                                x = L + rx + t;
                                y = T;
                                return;
                            }
                            t -= L_top;
                            if (t < L_arc)
                            {
                                float theta = -1.570796f + 1.570796f * (t / L_arc);
                                x = R - rx + rx * cosf(theta);
                                y = T + ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_right)
                            {
                                x = R;
                                y = T + ry + t;
                                return;
                            }
                            t -= L_right;
                            if (t < L_arc)
                            {
                                float theta = 0.0f + 1.570796f * (t / L_arc);
                                x = R - rx + rx * cosf(theta);
                                y = B - ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_bottom)
                            {
                                x = R - rx - t;
                                y = B;
                                return;
                            }
                            t -= L_bottom;
                            if (t < L_arc)
                            {
                                float theta = 1.570796f + 1.570796f * (t / L_arc);
                                x = L + rx + rx * cosf(theta);
                                y = B - ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_left)
                            {
                                x = L;
                                y = B - ry - t;
                                return;
                            }
                            t -= L_left;
                            {
                                float theta = 3.1415927f + 1.570796f * (t / L_arc);
                                x = L + rx + rx * cosf(theta);
                                y = T + ry + ry * sinf(theta);
                            }
                        };
                    const float segStarts[] = {
                        0.0f,
                        L_top,
                        L_top + L_arc,
                        L_top + L_arc + L_right,
                        L_top + L_arc + L_right + L_arc,
                        L_top + L_arc + L_right + L_arc + L_bottom,
                        L_top + L_arc + L_right + L_arc + L_bottom + L_arc,
                        L_top + L_arc + L_right + L_arc + L_bottom + L_arc + L_left,
                        perimeter };
                    auto placeDotsOnSegment = [&](float tStart, float tEnd, bool skipFirst)
                        {
                            const float segLen = tEnd - tStart;
                            if (segLen <= 0.001f)
                                return;
                            const int n = std::max(1, static_cast<int>(segLen / ideal_spacing + 0.5f));
                            const int iStart = skipFirst ? 1 : 0;
                            for (int i = iStart; i <= n; ++i)
                            {
                                const float t = tStart + segLen * static_cast<float>(i) / static_cast<float>(n);
                                float x = 0.0f, y = 0.0f;
                                getPoint(t, x, y);
                                context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), r_dot, r_dot), brush);
                            }
                        };
                    for (int s = 0; s < 8; ++s)
                    {
                        if (segStarts[s + 1] - segStarts[s] <= 0.001f)
                            continue;
                        placeDotsOnSegment(segStarts[s], segStarts[s + 1], s != 0);
                    }
                }
            }
            else if (style == BoxBorder::Style::Dashed && params.elementRadiusX == 0.0f && params.elementRadiusY == 0.0f)
            {
                float left = r.rect.left;
                float top = r.rect.top;
                float right = r.rect.right;
                float bottom = r.rect.bottom;
                float dashLen = width * 3.0f;
                float gap = width * 1.0f;
                float period = dashLen + gap;
                context->FillRectangle(D2D1::RectF(left, top, left + dashLen, top + width), brush);
                context->FillRectangle(D2D1::RectF(left, top, left + width, top + dashLen), brush);
                context->FillRectangle(D2D1::RectF(right - dashLen, top, right, top + width), brush);
                context->FillRectangle(D2D1::RectF(right - width, top, right, top + dashLen), brush);
                context->FillRectangle(D2D1::RectF(right - dashLen, bottom - width, right, bottom), brush);
                context->FillRectangle(D2D1::RectF(right - width, bottom - dashLen, right, bottom), brush);
                context->FillRectangle(D2D1::RectF(left, bottom - width, left + dashLen, bottom), brush);
                context->FillRectangle(D2D1::RectF(left, bottom - dashLen, left + width, bottom), brush);
                auto fillDashes = [&](float runStart, float runEnd, bool isHoriz, float edgeCoord)
                    {
                        float runLen = runEnd - runStart;
                        if (runLen <= 0.0f)
                            return;
                        int n = (runLen >= gap + dashLen)
                            ? static_cast<int>((runLen - gap) / period)
                            : 0;
                        if (n < 1)
                        {
                            if (runLen >= width)
                            {
                                const float dashWidth = std::min(dashLen, runLen);
                                const float inset = (runLen - dashWidth) * 0.5f;
                                const float s = runStart + inset;
                                const float e = s + dashWidth;
                                if (isHoriz)
                                    context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + width), brush);
                                else
                                    context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + width, e), brush);
                            }
                            return;
                        }
                        float gapActual = (runLen - static_cast<float>(n) * dashLen) / static_cast<float>(n + 1);
                        for (int i = 0; i < n; ++i)
                        {
                            float s = runStart + gapActual + static_cast<float>(i) * (dashLen + gapActual);
                            float e = s + dashLen;
                            if (isHoriz)
                                context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + width), brush);
                            else
                                context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + width, e), brush);
                        }
                    };
                fillDashes(left + dashLen, right - dashLen, true, top);
                fillDashes(left + dashLen, right - dashLen, true, bottom - width);
                fillDashes(top + dashLen, bottom - dashLen, false, left);
                fillDashes(top + dashLen, bottom - dashLen, false, right - width);
            }
            else
            {
                const D2D1_ROUNDED_RECT strokeR = strokePathForBorder(r, width);
                if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                {
                    context->DrawRectangle(strokeR.rect, brush, width, sstyle.Get());
                }
                else
                {
                    context->DrawRoundedRectangle(strokeR, brush, width, sstyle.Get());
                }
            }
        };
    if (sameStyle)
    {
        drawStyleRect(rect, params.styleTop, strokeBrush, params.strokeWidth);
    }
    else
    {
        const float w = params.strokeWidth;
        const float eps = std::max(0.5f, w * 0.125f);
        const float rx = std::min(rect.radiusX, (rect.rect.right - rect.rect.left) * 0.5f);
        const float ry = std::min(rect.radiusY, (rect.rect.bottom - rect.rect.top) * 0.5f);
        const bool hasRadius = rx > 0.0f || ry > 0.0f;
        const bool crispSnap = !hasRadius &&
            params.position != BoxBorder::Position::Inside &&
            params.position != BoxBorder::Position::Center;
        const float L = hasRadius ? rect.rect.left : (crispSnap ? (std::floor(rect.rect.left) + 0.5f) : rect.rect.left);
        const float T = hasRadius ? rect.rect.top : (crispSnap ? (std::floor(rect.rect.top) + 0.5f) : rect.rect.top);
        const float R = hasRadius ? rect.rect.right : (crispSnap ? (std::floor(rect.rect.right) + 0.5f) : rect.rect.right);
        const float B = hasRadius ? rect.rect.bottom : (crispSnap ? (std::floor(rect.rect.bottom) + 0.5f) : rect.rect.bottom);
        if (w <= 0.0f || R <= L || B <= T)
            return;
        ID2D1Factory1* factory = Direct2D::GetFactory();
        if (!factory)
            return;
        auto makePolyClip = [&](const D2D1_POINT_2F* pts) -> Microsoft::WRL::ComPtr<ID2D1Geometry>
            {
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink)))
                    return nullptr;
                sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(pts[1]);
                sink->AddLine(pts[2]);
                sink->AddLine(pts[3]);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                Microsoft::WRL::ComPtr<ID2D1Geometry> g = path;
                return g;
            };
        const float aaPad = eps * 0.5f;
        const float outerPad = (params.position == BoxBorder::Position::Outside) ? eps :
            (params.position == BoxBorder::Position::Center) ? aaPad : 0.0f;
        const float innerPad = (params.position == BoxBorder::Position::Inside) ? 0.0f :
            (params.position == BoxBorder::Position::Center) ? aaPad : eps;
        D2D1_POINT_2F topPts[4] = {
            D2D1::Point2F(L - outerPad, T - outerPad), D2D1::Point2F(R + outerPad, T - outerPad),
            D2D1::Point2F(R - w + innerPad, T + w + innerPad), D2D1::Point2F(L + w - innerPad, T + w + innerPad) };
        D2D1_POINT_2F rightPts[4] = {
            D2D1::Point2F(R + outerPad, T - outerPad), D2D1::Point2F(R + outerPad, B + outerPad),
            D2D1::Point2F(R - w - innerPad, B - w + innerPad), D2D1::Point2F(R - w - innerPad, T + w - innerPad) };
        D2D1_POINT_2F bottomPts[4] = {
            D2D1::Point2F(L - outerPad, B + outerPad), D2D1::Point2F(R + outerPad, B + outerPad),
            D2D1::Point2F(R - w + innerPad, B - w - innerPad), D2D1::Point2F(L + w - innerPad, B - w - innerPad) };
        D2D1_POINT_2F leftPts[4] = {
            D2D1::Point2F(L - outerPad, T - outerPad), D2D1::Point2F(L - outerPad, B + outerPad),
            D2D1::Point2F(L + w + innerPad, B - w + innerPad), D2D1::Point2F(L + w + innerPad, T + w - innerPad) };
        auto topClip = makePolyClip(topPts);
        auto rightClip = makePolyClip(rightPts);
        auto bottomClip = makePolyClip(bottomPts);
        auto leftClip = makePolyClip(leftPts);
        struct SidePaint
        {
            BoxBorder::Style style;
            Microsoft::WRL::ComPtr<ID2D1Geometry> clip;
            int priority;
            D2D1_POINT_2F p0;
            D2D1_POINT_2F p1;
            int index;
        };
        auto fillPolygon = [&](const D2D1_POINT_2F* pts, int count, ID2D1Brush* brush = nullptr)
            {
                if (count < 3)
                    return;
                if (!brush)
                    brush = strokeBrush;
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink)))
                    return;
                sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
                for (int i = 1; i < count; ++i)
                    sink->AddLine(pts[i]);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                context->FillGeometry(path.Get(), brush);
            };
        auto drawBlinkSolidSideWithBrush = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2, ID2D1Brush* brush)
            {
                if (x2 <= x1 || y2 <= y1)
                    return;
                if (adjacentWidth1 == 0.0f && adjacentWidth2 == 0.0f)
                {
                    context->FillRectangle(D2D1::RectF(x1, y1, x2, y2), brush);
                    return;
                }
                D2D1_POINT_2F quad[4];
                switch (side)
                {
                case 0:
                    quad[0] = D2D1::Point2F(x1 + std::max(-adjacentWidth1, 0.0f), y1);
                    quad[1] = D2D1::Point2F(x1 + std::max(adjacentWidth1, 0.0f), y2);
                    quad[2] = D2D1::Point2F(x2 - std::max(adjacentWidth2, 0.0f), y2);
                    quad[3] = D2D1::Point2F(x2 - std::max(-adjacentWidth2, 0.0f), y1);
                    break;
                case 2:
                    quad[0] = D2D1::Point2F(x1 + std::max(adjacentWidth1, 0.0f), y1);
                    quad[1] = D2D1::Point2F(x1 + std::max(-adjacentWidth1, 0.0f), y2);
                    quad[2] = D2D1::Point2F(x2 - std::max(-adjacentWidth2, 0.0f), y2);
                    quad[3] = D2D1::Point2F(x2 - std::max(adjacentWidth2, 0.0f), y1);
                    break;
                case 3:
                    quad[0] = D2D1::Point2F(x1, y1 + std::max(-adjacentWidth1, 0.0f));
                    quad[1] = D2D1::Point2F(x1, y2 - std::max(-adjacentWidth2, 0.0f));
                    quad[2] = D2D1::Point2F(x2, y2 - std::max(adjacentWidth2, 0.0f));
                    quad[3] = D2D1::Point2F(x2, y1 + std::max(adjacentWidth1, 0.0f));
                    break;
                case 1:
                default:
                    quad[0] = D2D1::Point2F(x1, y1 + std::max(adjacentWidth1, 0.0f));
                    quad[1] = D2D1::Point2F(x1, y2 - std::max(adjacentWidth2, 0.0f));
                    quad[2] = D2D1::Point2F(x2, y2 - std::max(-adjacentWidth2, 0.0f));
                    quad[3] = D2D1::Point2F(x2, y1 + std::max(-adjacentWidth1, 0.0f));
                    break;
                }
                fillPolygon(quad, 4, brush);
            };
        auto drawBlinkSolidSide = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                drawBlinkSolidSideWithBrush(side, x1, y1, x2, y2, adjacentWidth1, adjacentWidth2, strokeBrush);
            };
        auto drawBlinkDoubleSide = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                const float thickness = (side == 0 || side == 2) ? (y2 - y1) : (x2 - x1);
                const float length = (side == 0 || side == 2) ? (x2 - x1) : (y2 - y1);
                if (thickness <= 0.0f || length <= 0.0f)
                    return;
                const float third = std::max(1.0f, std::floor((thickness + 1.0f) / 3.0f));
                if (adjacentWidth1 == 0.0f && adjacentWidth2 == 0.0f)
                {
                    if (side == 0 || side == 2)
                    {
                        context->FillRectangle(D2D1::RectF(x1, y1, x2, y1 + third), strokeBrush);
                        context->FillRectangle(D2D1::RectF(x1, y2 - third, x2, y2), strokeBrush);
                    }
                    else
                    {
                        context->FillRectangle(D2D1::RectF(x1, y1, x1 + third, y2), strokeBrush);
                        context->FillRectangle(D2D1::RectF(x2 - third, y1, x2, y2), strokeBrush);
                    }
                    return;
                }
                auto bigThird = [](float value)
                    {
                        return value > 0.0f ? std::floor((value + 1.0f) / 3.0f) : std::ceil((value - 1.0f) / 3.0f);
                    };
                const float adjacent1BigThird = bigThird(adjacentWidth1);
                const float adjacent2BigThird = bigThird(adjacentWidth2);
                switch (side)
                {
                case 0:
                    drawBlinkSolidSide(side,
                        x1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1,
                        x2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1 + third,
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2 - third,
                        x2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2,
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 3:
                    drawBlinkSolidSide(side,
                        x1,
                        y1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x1 + third,
                        y2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x2 - third,
                        y1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x2,
                        y2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 2:
                    drawBlinkSolidSide(side,
                        x1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1,
                        x2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1 + third,
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2 - third,
                        x2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2,
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 1:
                default:
                    drawBlinkSolidSide(side,
                        x1,
                        y1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x1 + third,
                        y2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x2 - third,
                        y1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x2,
                        y2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                }
            };
        COLORREF baseColor = params.strokeColor;
        BYTE rC = GetRValue(baseColor), gC = GetGValue(baseColor), bC = GetBValue(baseColor);
        COLORREF lightC = RGB(std::min(255, rC + 50), std::min(255, gC + 50), std::min(255, bC + 50));
        COLORREF darkC = RGB(std::max(0, rC - 50), std::max(0, gC - 50), std::max(0, bC - 50));
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> lightBrush, darkBrush;
        context->CreateSolidColorBrush(Direct2D::ColorToD2D(lightC, params.strokeAlpha / 255.0f), &lightBrush);
        context->CreateSolidColorBrush(Direct2D::ColorToD2D(darkC, params.strokeAlpha / 255.0f), &darkBrush);
        auto drawBlink3DSide = [&](int side, BoxBorder::Style style, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                const float thickness = (side == 0 || side == 2) ? (y2 - y1) : (x2 - x1);
                if (thickness <= 0.0f)
                    return;
                auto bigHalf = [](float value)
                    {
                        return value > 0.0f ? std::floor((value + 1.0f) / 2.0f) : std::ceil((value - 1.0f) / 2.0f);
                    };
                const float adjacent1BigHalf = bigHalf(adjacentWidth1);
                const float adjacent2BigHalf = bigHalf(adjacentWidth2);
                ID2D1Brush* insetBrush = (side == 0 || side == 3) ? darkBrush.Get() : lightBrush.Get();
                ID2D1Brush* outsetBrush = (side == 0 || side == 3) ? lightBrush.Get() : darkBrush.Get();
                if (style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset)
                {
                    ID2D1Brush* solidBrush = (style == BoxBorder::Style::Inset) ? insetBrush : outsetBrush;
                    drawBlinkSolidSideWithBrush(side, x1, y1, x2, y2, adjacentWidth1, adjacentWidth2, solidBrush);
                    return;
                }
                ID2D1Brush* firstBrush = (style == BoxBorder::Style::Groove) ? insetBrush : outsetBrush;
                ID2D1Brush* secondBrush = (style == BoxBorder::Style::Groove) ? outsetBrush : insetBrush;
                switch (side)
                {
                case 0:
                    drawBlinkSolidSideWithBrush(0,
                        x1 + std::max(-adjacentWidth1, 0.0f) / 2.0f,
                        y1,
                        x2 - std::max(-adjacentWidth2, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        adjacent1BigHalf, adjacent2BigHalf, firstBrush);
                    drawBlinkSolidSideWithBrush(0,
                        x1 + std::max(adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        x2 - std::max(adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        y2,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, secondBrush);
                    break;
                case 2:
                    drawBlinkSolidSideWithBrush(2,
                        x1 + std::max(adjacentWidth1, 0.0f) / 2.0f,
                        y1,
                        x2 - std::max(adjacentWidth2, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        adjacent1BigHalf, adjacent2BigHalf, secondBrush);
                    drawBlinkSolidSideWithBrush(2,
                        x1 + std::max(-adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        x2 - std::max(-adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        y2,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, firstBrush);
                    break;
                case 3:
                    drawBlinkSolidSideWithBrush(3,
                        x1,
                        y1 + std::max(-adjacentWidth1, 0.0f) / 2.0f,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y2 - std::max(-adjacentWidth2, 0.0f) / 2.0f,
                        adjacent1BigHalf, adjacent2BigHalf, firstBrush);
                    drawBlinkSolidSideWithBrush(3,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y1 + std::max(adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        x2,
                        y2 - std::max(adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, secondBrush);
                    break;
                case 1:
                default:
                    drawBlinkSolidSideWithBrush(1,
                        x1,
                        y1 + std::max(adjacentWidth1, 0.0f) / 2.0f,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y2 - std::max(adjacentWidth2, 0.0f) / 2.0f,
                        adjacent1BigHalf, adjacent2BigHalf, secondBrush);
                    drawBlinkSolidSideWithBrush(1,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y1 + std::max(-adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        x2,
                        y2 - std::max(-adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, firstBrush);
                    break;
                }
            };
        auto stylePriority = [](BoxBorder::Style s) -> int
            {
                if (s == BoxBorder::Style::Dotted || s == BoxBorder::Style::Dashed || s == BoxBorder::Style::Double)
                    return 1;
                if (s == BoxBorder::Style::None || s == BoxBorder::Style::Hidden)
                    return 0;
                return 2;
            };
        const D2D1_POINT_2F topP0 = D2D1::Point2F(L + w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F topP1 = D2D1::Point2F(R - w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F rightP0 = D2D1::Point2F(R - w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F rightP1 = D2D1::Point2F(R - w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F bottomP0 = D2D1::Point2F(R - w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F bottomP1 = D2D1::Point2F(L + w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F leftP0 = D2D1::Point2F(L + w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F leftP1 = D2D1::Point2F(L + w * 0.5f, T + w * 0.5f);
        std::vector<SidePaint> sides = {
            {params.styleTop, topClip, stylePriority(params.styleTop), topP0, topP1, 0},
            {params.styleRight, rightClip, stylePriority(params.styleRight), rightP0, rightP1, 1},
            {params.styleBottom, bottomClip, stylePriority(params.styleBottom), bottomP0, bottomP1, 2},
            {params.styleLeft, leftClip, stylePriority(params.styleLeft), leftP0, leftP1, 3} };
        std::stable_sort(sides.begin(), sides.end(), [](const SidePaint& a, const SidePaint& b)
            { return a.priority < b.priority; });
        auto isVisibleStyleForSide = [](BoxBorder::Style style)
            {
                return style != BoxBorder::Style::None && style != BoxBorder::Style::Hidden;
            };
        auto is3DStyleForSide = [](BoxBorder::Style style)
            {
                return style == BoxBorder::Style::Groove || style == BoxBorder::Style::Ridge ||
                    style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset;
            };
        const float il = L + w;
        const float it = T + w;
        const float ir = R - w;
        const float ib = B - w;
        const float irx = std::max(0.0f, rx - w);
        const float iry = std::max(0.0f, ry - w);
        const float L_top = R - L - 2.0f * rx;
        const float L_bottom = L_top;
        const float L_left = B - T - 2.0f * ry;
        const float L_right = L_left;
        auto quarterEllipseArcLength = [](float radX, float radY) -> float
            {
                if (radX <= 0.0f && radY <= 0.0f)
                    return 0.0f;
                if (radX <= 0.0f)
                    radX = radY;
                if (radY <= 0.0f)
                    radY = radX;
                const float a = std::max(radX, radY);
                const float b = std::min(radX, radY);
                if (std::fabs(a - b) < 0.001f)
                    return 1.5707963267948966f * a;
                const float h = ((a - b) * (a - b)) / ((a + b) * (a + b));
                const float fullPerimeter = 3.141592653589793f * (a + b) *
                    (1.0f + (3.0f * h) / (10.0f + sqrtf(4.0f - 3.0f * h)));
                return fullPerimeter * 0.25f;
            };
        const float L_arc = quarterEllipseArcLength(rx, ry);
        const float perimeter = L_top + L_right + L_bottom + L_left + 4.0f * L_arc;
        auto locatePerimeterT = [&](float t, float& edgeRadX, float& edgeRadY, float& cx, float& cy,
            bool outer, int& segment) -> float
            {
                if (t < 0.0f)
                    t = 0.0f;
                if (t > perimeter)
                    t = perimeter;
                if (t < L_top)
                {
                    segment = 0;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (L + rx) : (il + irx);
                    cy = outer ? (T + ry) : (it + iry);
                    return t;
                }
                t -= L_top;
                if (t < L_arc)
                {
                    segment = 1;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (R - rx) : (ir - irx);
                    cy = outer ? (T + ry) : (it + iry);
                    return t;
                }
                t -= L_arc;
                if (t < L_right)
                {
                    segment = 2;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (R - rx) : (ir - irx);
                    cy = outer ? (T + ry) : (it + iry);
                    return t;
                }
                t -= L_right;
                if (t < L_arc)
                {
                    segment = 3;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (R - rx) : (ir - irx);
                    cy = outer ? (B - ry) : (ib - iry);
                    return t;
                }
                t -= L_arc;
                if (t < L_bottom)
                {
                    segment = 4;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (L + rx) : (il + irx);
                    cy = outer ? (B - ry) : (ib - iry);
                    return t;
                }
                t -= L_bottom;
                if (t < L_arc)
                {
                    segment = 5;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (L + rx) : (il + irx);
                    cy = outer ? (B - ry) : (ib - iry);
                    return t;
                }
                t -= L_arc;
                if (t < L_left)
                {
                    segment = 6;
                    edgeRadX = outer ? rx : irx;
                    edgeRadY = outer ? ry : iry;
                    cx = outer ? (L + rx) : (il + irx);
                    cy = outer ? (T + ry) : (it + iry);
                    return t;
                }
                t -= L_left;
                segment = 7;
                edgeRadX = outer ? rx : irx;
                edgeRadY = outer ? ry : iry;
                cx = outer ? (L + rx) : (il + irx);
                cy = outer ? (T + ry) : (it + iry);
                return t;
            };
        auto getOuterEdgePoint = [&](float t, float& x, float& y)
            {
                float edgeRadX = 0.0f, edgeRadY = 0.0f, cx = 0.0f, cy = 0.0f;
                int segment = 0;
                float local = locatePerimeterT(t, edgeRadX, edgeRadY, cx, cy, true, segment);
                switch (segment)
                {
                case 0:
                    x = L + rx + local;
                    y = T;
                    return;
                case 1:
                {
                    float theta = -1.570796f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 2:
                    x = R;
                    y = T + ry + local;
                    return;
                case 3:
                {
                    float theta = 0.0f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 4:
                    x = R - rx - local;
                    y = B;
                    return;
                case 5:
                {
                    float theta = 1.570796f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 6:
                    x = L;
                    y = B - ry - local;
                    return;
                case 7:
                default:
                {
                    float theta = 3.1415927f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                }
            };
        auto getInnerEdgePoint = [&](float t, float& x, float& y)
            {
                float edgeRadX = 0.0f, edgeRadY = 0.0f, cx = 0.0f, cy = 0.0f;
                int segment = 0;
                float local = locatePerimeterT(t, edgeRadX, edgeRadY, cx, cy, false, segment);
                switch (segment)
                {
                case 0:
                    x = il + irx + local;
                    y = it;
                    return;
                case 1:
                {
                    float theta = -1.570796f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 2:
                    x = ir;
                    y = it + iry + local;
                    return;
                case 3:
                {
                    float theta = 0.0f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 4:
                    x = ir - irx - local;
                    y = ib;
                    return;
                case 5:
                {
                    float theta = 1.570796f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                case 6:
                    x = il;
                    y = ib - iry - local;
                    return;
                case 7:
                default:
                {
                    float theta = 3.1415927f + 1.570796f * (local / L_arc);
                    x = cx + edgeRadX * cosf(theta);
                    y = cy + edgeRadY * sinf(theta);
                    return;
                }
                }
            };
        auto getMidlinePoint = [&](float t, float& x, float& y)
            {
                float ox = 0.0f, oy = 0.0f, ix = 0.0f, iy = 0.0f;
                getOuterEdgePoint(t, ox, oy);
                getInnerEdgePoint(t, ix, iy);
                x = (ox + ix) * 0.5f;
                y = (oy + iy) * 0.5f;
            };
        auto getEdgePointAtDepth = [&](float t, float depth, float& x, float& y)
            {
                float ox = 0.0f, oy = 0.0f, ix = 0.0f, iy = 0.0f;
                getOuterEdgePoint(t, ox, oy);
                getInnerEdgePoint(t, ix, iy);
                x = ox + (ix - ox) * depth;
                y = oy + (iy - oy) * depth;
            };
        // Each side owns half of its start/end corner arcs (CSS 45deg corner split).
        const float sideArcSpan = L_arc * 0.5f;
        auto sideSegmentLength = [&](int sideIndex) -> float
            {
                switch (sideIndex)
                {
                case 0:
                    return L_top + 2.0f * sideArcSpan;
                case 1:
                    return L_right + 2.0f * sideArcSpan;
                case 2:
                    return L_bottom + 2.0f * sideArcSpan;
                case 3:
                    return L_left + 2.0f * sideArcSpan;
                default:
                    return 0.0f;
                }
            };
        auto sideSegmentStart = [&](int sideIndex) -> float
            {
                switch (sideIndex)
                {
                case 0:
                    return perimeter - sideArcSpan;
                case 1:
                    return L_top + sideArcSpan;
                case 2:
                    return L_top + L_arc + L_right + sideArcSpan;
                case 3:
                    return L_top + L_arc + L_right + sideArcSpan + L_bottom + L_arc;
                default:
                    return 0.0f;
                }
            };
        const float joinPad = hasRadius ? std::max(0.75f, w * 0.06f) : 0.0f;
        auto wrapPerimeterT = [&](float t) -> float
            {
                if (perimeter <= 0.0f)
                    return 0.0f;
                while (t < 0.0f)
                    t += perimeter;
                while (t >= perimeter)
                    t -= perimeter;
                return t;
            };
        auto mapSideT = [&](int sideIndex, float localT) -> float
            {
                const float start = sideSegmentStart(sideIndex);
                if (sideIndex == 0)
                    return wrapPerimeterT(start + localT);
                return start + localT;
            };
        auto fillRoundedSideBandBetween = [&](int sideIndex, float depth0, float depth1, ID2D1Brush* brush)
            {
                if (!brush)
                    return;
                const float segLen = sideSegmentLength(sideIndex);
                if (segLen <= 0.001f)
                    return;
                const float spanLen = segLen + 2.0f * joinPad;
                const int steps = std::max(8, static_cast<int>(spanLen / 1.5f));
                std::vector<D2D1_POINT_2F> poly;
                poly.reserve(static_cast<size_t>((steps + 1) * 2));
                for (int i = 0; i <= steps; ++i)
                {
                    const float localT = -joinPad + spanLen * static_cast<float>(i) / static_cast<float>(steps);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), depth0, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                for (int i = steps; i >= 0; --i)
                {
                    const float localT = -joinPad + spanLen * static_cast<float>(i) / static_cast<float>(steps);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), depth1, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                fillPolygon(poly.data(), static_cast<int>(poly.size()), brush);
            };
        auto fillRoundedSideBand = [&](int sideIndex, ID2D1Brush* brush)
            {
                fillRoundedSideBandBetween(sideIndex, 0.0f, 1.0f, brush);
            };
        auto createRoundedSideBandClip = [&](int sideIndex) -> Microsoft::WRL::ComPtr<ID2D1Geometry>
            {
                const float segLen = sideSegmentLength(sideIndex);
                if (segLen <= 0.001f)
                    return nullptr;
                const float spanLen = segLen + 2.0f * joinPad;
                const int steps = std::max(8, static_cast<int>(spanLen / 1.5f));
                std::vector<D2D1_POINT_2F> poly;
                poly.reserve(static_cast<size_t>((steps + 1) * 2));
                for (int i = 0; i <= steps; ++i)
                {
                    const float localT = -joinPad + spanLen * static_cast<float>(i) / static_cast<float>(steps);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), 0.0f, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                for (int i = steps; i >= 0; --i)
                {
                    const float localT = -joinPad + spanLen * static_cast<float>(i) / static_cast<float>(steps);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), 1.0f, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink)))
                    return nullptr;
                sink->BeginFigure(poly[0], D2D1_FIGURE_BEGIN_FILLED);
                for (size_t i = 1; i < poly.size(); ++i)
                    sink->AddLine(poly[i]);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                Microsoft::WRL::ComPtr<ID2D1Geometry> geometry = path;
                return geometry;
            };
        auto pushSideBandClip = [&](int sideIndex)
            {
                Microsoft::WRL::ComPtr<ID2D1Geometry> clip = createRoundedSideBandClip(sideIndex);
                if (!clip)
                    return;
                D2D1_LAYER_PARAMETERS1 params = D2D1::LayerParameters1(
                    D2D1::InfiniteRect(),
                    clip.Get(),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::Matrix3x2F::Identity(),
                    1.0f,
                    nullptr,
                    D2D1_LAYER_OPTIONS1_NONE);
                context->PushLayer(params, nullptr);
            };
        auto drawRoundedSideDotted = [&](int sideIndex)
            {
                const float segLen = sideSegmentLength(sideIndex);
                if (segLen <= 0.001f)
                    return;
                const float idealSpacing = w * 2.0f;
                int numDots = std::max(1, static_cast<int>(segLen / idealSpacing + 0.5f));
                const float step = segLen / static_cast<float>(numDots);
                const float dotRadius = w * 0.5f;
                for (int i = 0; i < numDots; ++i)
                {
                    const float localT = step * (static_cast<float>(i) + 0.5f);
                    if (localT <= 0.0f || localT >= segLen)
                        continue;
                    float x = 0.0f, y = 0.0f;
                    getMidlinePoint(mapSideT(sideIndex, localT), x, y);
                    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), dotRadius, dotRadius), strokeBrush);
                }
            };
        auto fillRoundedDashStrip = [&](int sideIndex, float localStart, float localEnd)
            {
                const float segLen = sideSegmentLength(sideIndex);
                const float clipStart = std::max(-joinPad, localStart);
                const float clipEnd = std::min(segLen + joinPad, localEnd);
                const float stripLen = clipEnd - clipStart;
                if (stripLen <= 0.001f)
                    return;
                const int samples = std::max(2, static_cast<int>(stripLen / 2.0f));
                std::vector<D2D1_POINT_2F> poly;
                poly.reserve(static_cast<size_t>((samples + 1) * 2));
                for (int i = 0; i <= samples; ++i)
                {
                    const float localT = clipStart + stripLen * static_cast<float>(i) / static_cast<float>(samples);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), 0.0f, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                for (int i = samples; i >= 0; --i)
                {
                    const float localT = clipStart + stripLen * static_cast<float>(i) / static_cast<float>(samples);
                    float x = 0.0f, y = 0.0f;
                    getEdgePointAtDepth(mapSideT(sideIndex, localT), 1.0f, x, y);
                    poly.push_back(D2D1::Point2F(x, y));
                }
                fillPolygon(poly.data(), static_cast<int>(poly.size()), strokeBrush);
            };
        auto drawRoundedSideDashed = [&](int sideIndex)
            {
                const float segLen = sideSegmentLength(sideIndex);
                if (segLen <= 0.001f)
                    return;
                const float dashLen = w * 3.0f;
                const float gap = w * 1.0f;
                const float period = dashLen + gap;
                // Only reserve the half-corner arcs; using dashLen here consumes the straight edge
                // and leaves too little room for interior dashes on small rounded boxes.
                const float cornerRun = std::min(sideArcSpan + joinPad, segLen * 0.5f);
                if (cornerRun > 0.001f)
                {
                    fillRoundedDashStrip(sideIndex, 0.0f, cornerRun);
                    fillRoundedDashStrip(sideIndex, segLen - cornerRun, segLen);
                }
                const float runStart = cornerRun;
                const float runEnd = segLen - cornerRun;
                const float runLen = runEnd - runStart;
                if (runLen <= 0.001f)
                    return;
                int dashCount = (runLen >= gap + dashLen)
                    ? static_cast<int>((runLen - gap) / period)
                    : 0;
                if (dashCount < 1)
                {
                    if (runLen >= w)
                    {
                        const float dashWidth = std::min(dashLen, runLen);
                        const float inset = (runLen - dashWidth) * 0.5f;
                        fillRoundedDashStrip(sideIndex, runStart + inset, runStart + inset + dashWidth);
                    }
                    return;
                }
                const float gapActual = (runLen - static_cast<float>(dashCount) * dashLen) /
                    static_cast<float>(dashCount + 1);
                for (int i = 0; i < dashCount; ++i)
                {
                    const float dashStart = runStart + gapActual + static_cast<float>(i) * (dashLen + gapActual);
                    const float dashEnd = dashStart + dashLen;
                    fillRoundedDashStrip(sideIndex, dashStart, dashEnd);
                }
            };
        auto drawRectSideDashed = [&](int sideIndex)
            {
                const float dashLen = w * 3.0f;
                const float gap = w * 1.0f;
                const float period = dashLen + gap;
                auto fillEdgeDashes = [&](float runStart, float runEnd, bool isHoriz, float edgeCoord)
                    {
                        const float runLen = runEnd - runStart;
                        if (runLen <= 0.0f)
                            return;
                        int dashCount = (runLen >= gap + dashLen)
                            ? static_cast<int>((runLen - gap) / period)
                            : 0;
                        if (dashCount < 1)
                        {
                            if (runLen >= w)
                            {
                                const float dashWidth = std::min(dashLen, runLen);
                                const float inset = (runLen - dashWidth) * 0.5f;
                                const float s = runStart + inset;
                                const float e = s + dashWidth;
                                if (isHoriz)
                                    context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + w), strokeBrush);
                                else
                                    context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + w, e), strokeBrush);
                            }
                            return;
                        }
                        const float gapActual = (runLen - static_cast<float>(dashCount) * dashLen) /
                            static_cast<float>(dashCount + 1);
                        for (int i = 0; i < dashCount; ++i)
                        {
                            const float s = runStart + gapActual + static_cast<float>(i) * (dashLen + gapActual);
                            const float e = s + dashLen;
                            if (isHoriz)
                                context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + w), strokeBrush);
                            else
                                context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + w, e), strokeBrush);
                        }
                    };
                switch (sideIndex)
                {
                case 0:
                    context->FillRectangle(D2D1::RectF(L, T, L + dashLen, T + w), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, T, L + w, T + dashLen), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - dashLen, T, R, T + w), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - w, T, R, T + dashLen), strokeBrush);
                    fillEdgeDashes(L + dashLen, R - dashLen, true, T);
                    break;
                case 1:
                    context->FillRectangle(D2D1::RectF(R - dashLen, T, R, T + w), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - w, T, R, T + dashLen), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - dashLen, B - w, R, B), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - w, B - dashLen, R, B), strokeBrush);
                    fillEdgeDashes(T + dashLen, B - dashLen, false, R - w);
                    break;
                case 2:
                    context->FillRectangle(D2D1::RectF(R - dashLen, B - w, R, B), strokeBrush);
                    context->FillRectangle(D2D1::RectF(R - w, B - dashLen, R, B), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, B - w, L + dashLen, B), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, B - dashLen, L + w, B), strokeBrush);
                    fillEdgeDashes(L + dashLen, R - dashLen, true, B - w);
                    break;
                case 3:
                default:
                    context->FillRectangle(D2D1::RectF(L, T, L + dashLen, T + w), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, T, L + w, T + dashLen), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, B - w, L + dashLen, B), strokeBrush);
                    context->FillRectangle(D2D1::RectF(L, B - dashLen, L + w, B), strokeBrush);
                    fillEdgeDashes(T + dashLen, B - dashLen, false, L);
                    break;
                }
            };
        for (const auto& side : sides)
        {
            if (!side.clip || side.style == BoxBorder::Style::None || side.style == BoxBorder::Style::Hidden)
                continue;
            const bool useSideClip = !hasRadius &&
                (side.style == BoxBorder::Style::Dotted || side.style == BoxBorder::Style::Dashed);
            if (useSideClip)
            {
                D2D1_LAYER_PARAMETERS1 params = D2D1::LayerParameters1(
                    D2D1::InfiniteRect(),
                    side.clip.Get(),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::Matrix3x2F::Identity(),
                    1.0f,
                    nullptr,
                    D2D1_LAYER_OPTIONS1_NONE);
                context->PushLayer(params, nullptr);
            }
            if (side.style == BoxBorder::Style::Dotted || side.style == BoxBorder::Style::Dashed)
            {
                if (hasRadius)
                {
                    pushSideBandClip(side.index);
                    if (side.style == BoxBorder::Style::Dotted)
                        drawRoundedSideDotted(side.index);
                    else
                        drawRoundedSideDashed(side.index);
                    context->PopLayer();
                }
                else
                {
                const float dx = side.p1.x - side.p0.x;
                const float dy = side.p1.y - side.p0.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f)
                {
                    const float ux = dx / len;
                    const float uy = dy / len;
                    if (side.style == BoxBorder::Style::Dotted)
                    {
                        const float targetSpacing = w * 2.0f;
                        int count = std::max(2, static_cast<int>(len / targetSpacing + 0.5f) + 1);
                        for (int i = 0; i < count; ++i)
                        {
                            const float t = len * static_cast<float>(i) / static_cast<float>(count - 1);
                            const D2D1_POINT_2F pt = D2D1::Point2F(
                                side.p0.x + ux * t,
                                side.p0.y + uy * t);
                            context->FillEllipse(D2D1::Ellipse(pt, w * 0.5f, w * 0.5f), strokeBrush);
                        }
                    }
                    else
                    {
                        drawRectSideDashed(side.index);
                    }
                }
                }
            }
            else if (side.style == BoxBorder::Style::Solid)
            {
                if (hasRadius)
                {
                    fillRoundedSideBand(side.index, strokeBrush);
                }
                else
                {
                    const BoxBorder::Style adjacentStyle1 = side.index == 0 ? params.styleLeft : side.index == 1 ? params.styleTop
                        : side.index == 2 ? params.styleLeft
                        : params.styleTop;
                    const BoxBorder::Style adjacentStyle2 = side.index == 0 ? params.styleRight : side.index == 1 ? params.styleBottom
                        : side.index == 2 ? params.styleRight
                        : params.styleBottom;
                    const float adjacent1 = (side.index == 2 && is3DStyleForSide(adjacentStyle1)) ? 0.0f : (isVisibleStyleForSide(adjacentStyle1) ? w : 0.0f);
                    const float adjacent2 = isVisibleStyleForSide(adjacentStyle2) ? w : 0.0f;
                    if (side.index == 0)
                        drawBlinkSolidSide(0, L, T, R, T + w, adjacent1, adjacent2);
                    else if (side.index == 1)
                        drawBlinkSolidSide(1, R - w, T, R, B, adjacent1, adjacent2);
                    else if (side.index == 2)
                        drawBlinkSolidSide(2, L, B - w, R, B, adjacent1, adjacent2);
                    else
                        drawBlinkSolidSide(3, L, T, L + w, B, adjacent1, adjacent2);
                }
            }
            else if (side.style == BoxBorder::Style::Double)
            {
                if (hasRadius)
                {
                    const float third = 1.0f / 3.0f;
                    const float twoThirds = 2.0f * third;
                    fillRoundedSideBandBetween(side.index, 0.0f, third, strokeBrush);
                    fillRoundedSideBandBetween(side.index, twoThirds, 1.0f, strokeBrush);
                }
                else
                {
                    const float adjacent1 = isVisibleStyleForSide(side.index == 0 ? params.styleLeft : side.index == 1 ? params.styleTop
                        : side.index == 2 ? params.styleLeft
                        : params.styleTop)
                        ? w
                        : 0.0f;
                    const float adjacent2 = isVisibleStyleForSide(side.index == 0 ? params.styleRight : side.index == 1 ? params.styleBottom
                        : side.index == 2 ? params.styleRight
                        : params.styleBottom)
                        ? w
                        : 0.0f;
                    if (side.index == 0)
                        drawBlinkDoubleSide(0, L, T, R, T + w, adjacent1, adjacent2);
                    else if (side.index == 1)
                        drawBlinkDoubleSide(1, R - w, T, R, B, adjacent1, adjacent2);
                    else if (side.index == 2)
                        drawBlinkDoubleSide(2, L, B - w, R, B, adjacent1, adjacent2);
                    else
                        drawBlinkDoubleSide(3, L, T, L + w, B, adjacent1, adjacent2);
                }
            }
            else
            {
                if (hasRadius)
                {
                    ID2D1Brush* insetBrush = (side.index == 0 || side.index == 3) ? darkBrush.Get() : lightBrush.Get();
                    ID2D1Brush* outsetBrush = (side.index == 0 || side.index == 3) ? lightBrush.Get() : darkBrush.Get();
                    if (side.style == BoxBorder::Style::Inset)
                        fillRoundedSideBand(side.index, insetBrush);
                    else if (side.style == BoxBorder::Style::Outset)
                        fillRoundedSideBand(side.index, outsetBrush);
                    else if (side.style == BoxBorder::Style::Groove)
                    {
                        fillRoundedSideBandBetween(side.index, 0.0f, 0.5f, insetBrush);
                        fillRoundedSideBandBetween(side.index, 0.5f, 1.0f, outsetBrush);
                    }
                    else
                    {
                        fillRoundedSideBandBetween(side.index, 0.0f, 0.5f, outsetBrush);
                        fillRoundedSideBandBetween(side.index, 0.5f, 1.0f, insetBrush);
                    }
                }
                else
                {
                    const float adjacent1 = isVisibleStyleForSide(side.index == 0 ? params.styleLeft : side.index == 1 ? params.styleTop
                        : side.index == 2 ? params.styleLeft
                        : params.styleTop)
                        ? w
                        : 0.0f;
                    const float adjacent2 = isVisibleStyleForSide(side.index == 0 ? params.styleRight : side.index == 1 ? params.styleBottom
                        : side.index == 2 ? params.styleRight
                        : params.styleBottom)
                        ? w
                        : 0.0f;
                    if (side.index == 0)
                        drawBlink3DSide(0, side.style, L, T, R, T + w, adjacent1, adjacent2);
                    else if (side.index == 1)
                        drawBlink3DSide(1, side.style, R - w, T, R, B, adjacent1, adjacent2);
                    else if (side.index == 2)
                        drawBlink3DSide(2, side.style, L, B - w, R, B, adjacent1, adjacent2);
                    else
                        drawBlink3DSide(3, side.style, L, T, L + w, B, adjacent1, adjacent2);
                }
            }
            if (useSideClip)
                context->PopLayer();
        }
        auto isVisibleStyle = [](BoxBorder::Style s)
            {
                return s != BoxBorder::Style::None && s != BoxBorder::Style::Hidden;
            };
        auto isComplexJoinStyle = [](BoxBorder::Style s)
            {
                return s == BoxBorder::Style::Dotted || s == BoxBorder::Style::Dashed ||
                    s == BoxBorder::Style::Double || s == BoxBorder::Style::Groove ||
                    s == BoxBorder::Style::Ridge || s == BoxBorder::Style::Inset ||
                    s == BoxBorder::Style::Outset;
            };
        auto fillCornerBevel = [&](D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_POINT_2F c)
            {
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> g;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&g)) || FAILED(g->Open(&sink)))
                    return;
                sink->BeginFigure(a, D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(b);
                sink->AddLine(c);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                context->FillGeometry(g.Get(), strokeBrush);
            };
        if (hasRadius)
        {
            auto fillRoundedCornerPatch = [&](float junctionT, ID2D1Brush* brush)
                {
                    if (!brush)
                        return;
                    const float t0 = wrapPerimeterT(junctionT - joinPad);
                    const float t1 = wrapPerimeterT(junctionT + joinPad);
                    const float tJ = wrapPerimeterT(junctionT);
                    float o0x = 0.0f, o0y = 0.0f, o1x = 0.0f, o1y = 0.0f, ojx = 0.0f, ojy = 0.0f;
                    float i0x = 0.0f, i0y = 0.0f, i1x = 0.0f, i1y = 0.0f, ijx = 0.0f, ijy = 0.0f;
                    getOuterEdgePoint(t0, o0x, o0y);
                    getOuterEdgePoint(t1, o1x, o1y);
                    getOuterEdgePoint(tJ, ojx, ojy);
                    getInnerEdgePoint(t0, i0x, i0y);
                    getInnerEdgePoint(t1, i1x, i1y);
                    getInnerEdgePoint(tJ, ijx, ijy);
                    const D2D1_POINT_2F pts[] = {
                        D2D1::Point2F(o0x, o0y),
                        D2D1::Point2F(ojx, ojy),
                        D2D1::Point2F(o1x, o1y),
                        D2D1::Point2F(i1x, i1y),
                        D2D1::Point2F(ijx, ijy),
                        D2D1::Point2F(i0x, i0y) };
                    fillPolygon(pts, 6, brush);
                };
            const float junctionTL = perimeter - sideArcSpan;
            const float junctionTR = L_top + sideArcSpan;
            const float junctionBR = L_top + L_arc + L_right + sideArcSpan;
            const float junctionBL = L_top + L_arc + L_right + sideArcSpan + L_bottom + L_arc;
            if (isVisibleStyleForSide(params.styleTop) && isVisibleStyleForSide(params.styleLeft))
                fillRoundedCornerPatch(junctionTL, strokeBrush);
            if (isVisibleStyleForSide(params.styleTop) && isVisibleStyleForSide(params.styleRight))
                fillRoundedCornerPatch(junctionTR, strokeBrush);
            if (isVisibleStyleForSide(params.styleBottom) && isVisibleStyleForSide(params.styleRight))
                fillRoundedCornerPatch(junctionBR, strokeBrush);
            if (isVisibleStyleForSide(params.styleBottom) && isVisibleStyleForSide(params.styleLeft))
                fillRoundedCornerPatch(junctionBL, strokeBrush);
        }
        else if (!hasRadius)
        {
            if (isVisibleStyle(params.styleTop) && isVisibleStyle(params.styleLeft) &&
                !isComplexJoinStyle(params.styleTop) && !isComplexJoinStyle(params.styleLeft))
                fillCornerBevel(
                    D2D1::Point2F(L, T),
                    D2D1::Point2F(L + w, T),
                    D2D1::Point2F(L, T + w));
            if (isVisibleStyle(params.styleTop) && isVisibleStyle(params.styleRight) &&
                !isComplexJoinStyle(params.styleTop) && !isComplexJoinStyle(params.styleRight))
                fillCornerBevel(
                    D2D1::Point2F(R - w, T),
                    D2D1::Point2F(R, T),
                    D2D1::Point2F(R, T + w));
            if (isVisibleStyle(params.styleBottom) && isVisibleStyle(params.styleLeft) &&
                !isComplexJoinStyle(params.styleBottom) && !isComplexJoinStyle(params.styleLeft))
                fillCornerBevel(
                    D2D1::Point2F(L, B - w),
                    D2D1::Point2F(L + w, B - w),
                    D2D1::Point2F(L, B));
            if (isVisibleStyle(params.styleBottom) && isVisibleStyle(params.styleRight) &&
                !isComplexJoinStyle(params.styleBottom) && !isComplexJoinStyle(params.styleRight))
                fillCornerBevel(
                    D2D1::Point2F(R - w, B - w),
                    D2D1::Point2F(R, B - w),
                    D2D1::Point2F(R, B));
        }
    }
}