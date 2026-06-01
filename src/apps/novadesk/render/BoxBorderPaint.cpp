/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BoxBorderPaint.h"
#include <algorithm>
#include <wrl/client.h>

namespace
{
    bool IsSolid(BoxBorder::Style style)
    {
        return style == BoxBorder::Style::Solid;
    }

    bool IsVisible(BoxBorder::Style style)
    {
        return style == BoxBorder::Style::Solid ||
            style == BoxBorder::Style::Inset ||
            style == BoxBorder::Style::Outset;
    }

    COLORREF DarkenColor(COLORREF color)
    {
        return RGB(
            static_cast<BYTE>(GetRValue(color) * 0.55f),
            static_cast<BYTE>(GetGValue(color) * 0.55f),
            static_cast<BYTE>(GetBValue(color) * 0.55f));
    }

    D2D1_COLOR_F ToD2DColor(COLORREF color, BYTE alpha)
    {
        return D2D1::ColorF(
            GetRValue(color) / 255.0f,
            GetGValue(color) / 255.0f,
            GetBValue(color) / 255.0f,
            alpha / 255.0f);
    }

    D2D1_ROUNDED_RECT InsetRoundedRect(D2D1_ROUNDED_RECT rect, float amount)
    {
        rect.rect.left += amount;
        rect.rect.top += amount;
        rect.rect.right -= amount;
        rect.rect.bottom -= amount;
        rect.radiusX = std::max(0.0f, rect.radiusX - amount);
        rect.radiusY = std::max(0.0f, rect.radiusY - amount);
        return rect;
    }

    bool IsRenderableRect(const D2D1_RECT_F& rect)
    {
        return rect.right > rect.left && rect.bottom > rect.top;
    }

    void FillBorderBand(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& outer,
        float width, ID2D1Brush* brush)
    {
        if (!context || !brush || width <= 0.0f || !IsRenderableRect(outer.rect))
            return;

        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        context->GetFactory(&factory);
        if (!factory)
            return;

        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> outerGeometry;
        if (FAILED(factory->CreateRoundedRectangleGeometry(outer, &outerGeometry)))
            return;

        const D2D1_ROUNDED_RECT inner = InsetRoundedRect(outer, width);
        if (!IsRenderableRect(inner.rect))
        {
            context->FillGeometry(outerGeometry.Get(), brush);
            return;
        }

        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> innerGeometry;
        Microsoft::WRL::ComPtr<ID2D1PathGeometry> bandGeometry;
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(factory->CreateRoundedRectangleGeometry(inner, &innerGeometry)) ||
            FAILED(factory->CreatePathGeometry(&bandGeometry)) ||
            FAILED(bandGeometry->Open(&sink)))
        {
            return;
        }

        if (SUCCEEDED(outerGeometry->CombineWithGeometry(
            innerGeometry.Get(),
            D2D1_COMBINE_MODE_EXCLUDE,
            nullptr,
            sink.Get())))
        {
            sink->Close();
            context->FillGeometry(bandGeometry.Get(), brush);
        }
    }

    void FillPolygon(ID2D1DeviceContext* context, ID2D1Factory* factory,
        const D2D1_POINT_2F* points, size_t count, ID2D1Brush* brush)
    {
        if (!context || !factory || !points || count < 3 || !brush)
            return;

        Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(factory->CreatePathGeometry(&geometry)) ||
            FAILED(geometry->Open(&sink)))
        {
            return;
        }

        sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
        for (size_t i = 1; i < count; ++i)
            sink->AddLine(points[i]);
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        if (SUCCEEDED(sink->Close()))
            context->FillGeometry(geometry.Get(), brush);
    }
}

D2D1_ROUNDED_RECT BoxBorderPaint::BuildBorderGeometryRect(const D2D1_ROUNDED_RECT& elementRect,
    const BoxBorderPaintParams&)
{
    return elementRect;
}

void BoxBorderPaint::PaintForElement(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& elementRect,
    const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush)
{
    if (!context || !strokeBrush || params.strokeWidth <= 0.0f)
        return;

    D2D1_LAYER_PARAMETERS1 clipParams = D2D1::LayerParameters1(
        elementRect.rect,
        nullptr,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::Matrix3x2F::Identity(),
        1.0f,
        nullptr,
        D2D1_LAYER_OPTIONS1_NONE);
    context->PushLayer(clipParams, nullptr);
    Paint(context, elementRect, params, strokeBrush);
    context->PopLayer();
}

void BoxBorderPaint::Paint(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& rect,
    const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush)
{
    if (!context || !strokeBrush || params.strokeWidth <= 0.0f)
        return;

    const bool top = IsVisible(params.styleTop);
    const bool right = IsVisible(params.styleRight);
    const bool bottom = IsVisible(params.styleBottom);
    const bool left = IsVisible(params.styleLeft);
    if (!top && !right && !bottom && !left)
        return;

    const float w = std::min(
        params.strokeWidth,
        std::max(0.0f, std::min(rect.rect.right - rect.rect.left, rect.rect.bottom - rect.rect.top) * 0.5f));
    if (w <= 0.0f)
        return;

    const float radiusX = std::min(rect.radiusX, (rect.rect.right - rect.rect.left) * 0.5f);
    const float radiusY = std::min(rect.radiusY, (rect.rect.bottom - rect.rect.top) * 0.5f);
    const bool allSolid = IsSolid(params.styleTop) &&
        IsSolid(params.styleRight) &&
        IsSolid(params.styleBottom) &&
        IsSolid(params.styleLeft);

    const D2D1_ROUNDED_RECT outer = D2D1::RoundedRect(rect.rect, radiusX, radiusY);
    if (allSolid)
    {
        FillBorderBand(context, outer, w, strokeBrush);
        return;
    }

    // Chromium's non-uniform border path paints individual sides and clips/masks
    // their joins. Solid rounded borders use the exact outer-minus-inner band
    // above; inset/outset and mixed visibility are painted per side.
    const float L = rect.rect.left;
    const float T = rect.rect.top;
    const float R = rect.rect.right;
    const float B = rect.rect.bottom;
    const float joinOverlap = std::min(0.75f, w * 0.2f);

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> baseBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> darkBrush;
    context->CreateSolidColorBrush(ToD2DColor(params.strokeColor, params.strokeAlpha), &baseBrush);
    context->CreateSolidColorBrush(ToD2DColor(DarkenColor(params.strokeColor), params.strokeAlpha), &darkBrush);
    auto brushForSide = [&](BoxBorder::Style style, int side) -> ID2D1Brush*
        {
            if (style == BoxBorder::Style::Inset && (side == 0 || side == 3))
            {
                if (darkBrush.Get())
                    return darkBrush.Get();
                return strokeBrush;
            }
            if (style == BoxBorder::Style::Outset && (side == 1 || side == 2))
            {
                if (darkBrush.Get())
                    return darkBrush.Get();
                return strokeBrush;
            }
            if (style == BoxBorder::Style::Inset || style == BoxBorder::Style::Outset)
            {
                if (baseBrush.Get())
                    return baseBrush.Get();
                return strokeBrush;
            }
            return strokeBrush;
        };

    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
    context->GetFactory(&factory);
    if (!factory)
        return;

    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> outerGeometry;
    const bool shouldClipRounded = (radiusX > 0.0f || radiusY > 0.0f) &&
        SUCCEEDED(factory->CreateRoundedRectangleGeometry(outer, &outerGeometry));
    if (shouldClipRounded)
    {
        D2D1_LAYER_PARAMETERS1 layerParams = D2D1::LayerParameters1(
            rect.rect,
            outerGeometry.Get(),
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS1_NONE);
        context->PushLayer(layerParams, nullptr);
    }

    if (top)
    {
        const D2D1_POINT_2F points[] = {
            D2D1::Point2F(L, T),
            D2D1::Point2F(R, T),
            D2D1::Point2F(R - w + joinOverlap, T + w),
            D2D1::Point2F(L + w - joinOverlap, T + w)
        };
        FillPolygon(context, factory.Get(), points, 4, brushForSide(params.styleTop, 0));
    }
    if (right)
    {
        const D2D1_POINT_2F points[] = {
            D2D1::Point2F(R, T),
            D2D1::Point2F(R, B),
            D2D1::Point2F(R - w, B - w + joinOverlap),
            D2D1::Point2F(R - w, T + w - joinOverlap)
        };
        FillPolygon(context, factory.Get(), points, 4, brushForSide(params.styleRight, 1));
    }
    if (bottom)
    {
        const D2D1_POINT_2F points[] = {
            D2D1::Point2F(R, B),
            D2D1::Point2F(L, B),
            D2D1::Point2F(L + w - joinOverlap, B - w),
            D2D1::Point2F(R - w + joinOverlap, B - w)
        };
        FillPolygon(context, factory.Get(), points, 4, brushForSide(params.styleBottom, 2));
    }
    if (left)
    {
        const D2D1_POINT_2F points[] = {
            D2D1::Point2F(L, B),
            D2D1::Point2F(L, T),
            D2D1::Point2F(L + w, T + w - joinOverlap),
            D2D1::Point2F(L + w, B - w + joinOverlap)
        };
        FillPolygon(context, factory.Get(), points, 4, brushForSide(params.styleLeft, 3));
    }

    if (shouldClipRounded)
        context->PopLayer();
}
