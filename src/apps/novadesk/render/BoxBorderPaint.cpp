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

    void FillRectSide(ID2D1DeviceContext* context, const D2D1_RECT_F& rect, ID2D1Brush* brush)
    {
        if (IsRenderableRect(rect))
            context->FillRectangle(rect, brush);
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

    const bool top = IsSolid(params.styleTop);
    const bool right = IsSolid(params.styleRight);
    const bool bottom = IsSolid(params.styleBottom);
    const bool left = IsSolid(params.styleLeft);
    if (!top && !right && !bottom && !left)
        return;

    const float w = std::min(
        params.strokeWidth,
        std::max(0.0f, std::min(rect.rect.right - rect.rect.left, rect.rect.bottom - rect.rect.top) * 0.5f));
    if (w <= 0.0f)
        return;

    const float radiusX = std::min(rect.radiusX, (rect.rect.right - rect.rect.left) * 0.5f);
    const float radiusY = std::min(rect.radiusY, (rect.rect.bottom - rect.rect.top) * 0.5f);
    const bool allSolid = top && right && bottom && left;

    const D2D1_ROUNDED_RECT outer = D2D1::RoundedRect(rect.rect, radiusX, radiusY);
    if (allSolid)
    {
        FillBorderBand(context, outer, w, strokeBrush);
        return;
    }

    // Chromium's non-uniform border path paints individual sides and clips/masks
    // their joins. NovaDesk now supports only solid/none, so rectangular side
    // bands are sufficient for mixed visibility; all-solid rounded borders use
    // the exact outer-minus-inner band above.
    const float L = rect.rect.left;
    const float T = rect.rect.top;
    const float R = rect.rect.right;
    const float B = rect.rect.bottom;

    if (top)
        FillRectSide(context, D2D1::RectF(L, T, R, std::min(B, T + w)), strokeBrush);
    if (right)
        FillRectSide(context, D2D1::RectF(std::max(L, R - w), T, R, B), strokeBrush);
    if (bottom)
        FillRectSide(context, D2D1::RectF(L, std::max(T, B - w), R, B), strokeBrush);
    if (left)
        FillRectSide(context, D2D1::RectF(L, T, std::min(R, L + w), B), strokeBrush);

}
