/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "HistogramElement.h"
#include "Direct2DHelper.h"
#include <algorithm>

HistogramElement::HistogramElement(const std::wstring &id, int x, int y, int w, int h)
    : Element(ELEMENT_HISTOGRAM, id, x, y, w, h)
{
}

bool HistogramElement::BuildAutoRange(float &outMin, float &outMax) const
{
    if (!m_AutoRange)
    {
        outMin = 0.0f;
        outMax = 100.0f;
        return true;
    }

    bool hasValue = false;
    float minV = 0.0f;
    float maxV = 0.0f;

    auto scanSeries = [&](const std::vector<float> &series)
    {
        for (float v : series)
        {
            if (!hasValue)
            {
                minV = maxV = v;
                hasValue = true;
            }
            else
            {
                minV = (std::min)(minV, v);
                maxV = (std::max)(maxV, v);
            }
        }
    };

    scanSeries(m_PrimaryData);
    scanSeries(m_SecondaryData);

    if (!hasValue)
    {
        outMin = 0.0f;
        outMax = 1.0f;
        return false;
    }

    if (maxV - minV < 0.000001f)
    {
        outMin = minV;
        outMax = minV + 1.0f;
        return true;
    }

    outMin = minV;
    outMax = maxV;
    return true;
}

float HistogramElement::SampleAtFromNewest(const std::vector<float> &series, int sampleIndex) const
{
    if (series.empty() || sampleIndex < 0)
        return 0.0f;
    const int idx = static_cast<int>(series.size()) - 1 - sampleIndex;
    if (idx < 0 || idx >= static_cast<int>(series.size()))
        return 0.0f;
    return series[static_cast<size_t>(idx)];
}

float HistogramElement::NormalizeValue(float value, float minValue, float maxValue)
{
    const float range = maxValue - minValue;
    if (range <= 0.000001f)
        return 0.0f;
    float n = (value - minValue) / range;
    if (n < 0.0f)
        n = 0.0f;
    if (n > 1.0f)
        n = 1.0f;
    return n;
}

void HistogramElement::DrawSpan(
    ID2D1DeviceContext *context,
    const D2D1_RECT_F &dstRect,
    const D2D1_RECT_F &gradientRect,
    const GradientInfo *gradient,
    COLORREF color,
    BYTE alpha)
{
    if (!context)
        return;
    if (dstRect.right <= dstRect.left || dstRect.bottom <= dstRect.top)
        return;

    Microsoft::WRL::ComPtr<ID2D1Brush> brush;
    Direct2D::CreateBrushFromGradientOrColor(
        context,
        gradientRect,
        gradient,
        color,
        alpha / 255.0f,
        brush.GetAddressOf());
    if (brush)
    {
        context->FillRectangle(dstRect, brush.Get());
    }
}

void HistogramElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show || !context)
        return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    const int width = GetWidth();
    const int height = GetHeight();
    if (width <= 0 || height <= 0)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    const float left = static_cast<float>(m_X + m_PaddingLeft);
    const float top = static_cast<float>(m_Y + m_PaddingTop);
    const float right = left + static_cast<float>(width);
    const float bottom = top + static_cast<float>(height);
    const D2D1_RECT_F gradientRect = D2D1::RectF(left, top, right, bottom);

    float minValue = 0.0f;
    float maxValue = 100.0f;
    BuildAutoRange(minValue, maxValue);

    const bool hasSecondary = !m_SecondaryData.empty();
    const int samples = m_GraphHorizontalOrientation ? height : width;

    for (int i = 0; i < samples; ++i)
    {
        const float primaryValue = SampleAtFromNewest(m_PrimaryData, i);
        const float primaryN = NormalizeValue(primaryValue, minValue, maxValue);
        const int primarySize = m_GraphHorizontalOrientation
                                    ? static_cast<int>(primaryN * static_cast<float>(width))
                                    : static_cast<int>(primaryN * static_cast<float>(height));

        int secondarySize = 0;
        if (hasSecondary)
        {
            const float secondaryValue = SampleAtFromNewest(m_SecondaryData, i);
            const float secondaryN = NormalizeValue(secondaryValue, minValue, maxValue);
            secondarySize = m_GraphHorizontalOrientation
                                ? static_cast<int>(secondaryN * static_cast<float>(width))
                                : static_cast<int>(secondaryN * static_cast<float>(height));
        }

        const int bothSize = hasSecondary ? (std::min)(primarySize, secondarySize) : 0;

        if (m_GraphHorizontalOrientation)
        {
            const int sampleY = m_Flip ? (height - 1 - i) : i;
            const float y = top + static_cast<float>(sampleY);

            if (hasSecondary && bothSize > 0)
            {
                D2D1_RECT_F bothRect = m_GraphStartLeft
                                           ? D2D1::RectF(left, y, left + static_cast<float>(bothSize), y + 1.0f)
                                           : D2D1::RectF(right - static_cast<float>(bothSize), y, right, y + 1.0f);
                DrawSpan(context, bothRect, gradientRect, &m_BothGradient, m_BothColor, m_BothAlpha);
            }

            if (hasSecondary)
            {
                const bool secondaryLarger = secondarySize > primarySize;
                const int larger = secondaryLarger ? secondarySize : primarySize;
                if (larger > bothSize)
                {
                    D2D1_RECT_F restRect = m_GraphStartLeft
                                               ? D2D1::RectF(left + static_cast<float>(bothSize), y, left + static_cast<float>(larger), y + 1.0f)
                                               : D2D1::RectF(right - static_cast<float>(larger), y, right - static_cast<float>(bothSize), y + 1.0f);
                    if (secondaryLarger)
                    {
                        DrawSpan(context, restRect, gradientRect, &m_SecondaryGradient, m_SecondaryColor, m_SecondaryAlpha);
                    }
                    else
                    {
                        DrawSpan(context, restRect, gradientRect, &m_PrimaryGradient, m_PrimaryColor, m_PrimaryAlpha);
                    }
                }
            }
            else if (primarySize > 0)
            {
                D2D1_RECT_F primaryRect = m_GraphStartLeft
                                              ? D2D1::RectF(left, y, left + static_cast<float>(primarySize), y + 1.0f)
                                              : D2D1::RectF(right - static_cast<float>(primarySize), y, right, y + 1.0f);
                DrawSpan(context, primaryRect, gradientRect, &m_PrimaryGradient, m_PrimaryColor, m_PrimaryAlpha);
            }
        }
        else
        {
            const int sampleX = m_GraphStartLeft ? i : (width - 1 - i);
            const float x = left + static_cast<float>(sampleX);

            if (hasSecondary && bothSize > 0)
            {
                D2D1_RECT_F bothRect = m_Flip
                                           ? D2D1::RectF(x, top, x + 1.0f, top + static_cast<float>(bothSize))
                                           : D2D1::RectF(x, bottom - static_cast<float>(bothSize), x + 1.0f, bottom);
                DrawSpan(context, bothRect, gradientRect, &m_BothGradient, m_BothColor, m_BothAlpha);
            }

            if (hasSecondary)
            {
                const bool secondaryLarger = secondarySize > primarySize;
                const int larger = secondaryLarger ? secondarySize : primarySize;
                if (larger > bothSize)
                {
                    D2D1_RECT_F restRect = m_Flip
                                               ? D2D1::RectF(x, top + static_cast<float>(bothSize), x + 1.0f, top + static_cast<float>(larger))
                                               : D2D1::RectF(x, bottom - static_cast<float>(larger), x + 1.0f, bottom - static_cast<float>(bothSize));
                    if (secondaryLarger)
                    {
                        DrawSpan(context, restRect, gradientRect, &m_SecondaryGradient, m_SecondaryColor, m_SecondaryAlpha);
                    }
                    else
                    {
                        DrawSpan(context, restRect, gradientRect, &m_PrimaryGradient, m_PrimaryColor, m_PrimaryAlpha);
                    }
                }
            }
            else if (primarySize > 0)
            {
                D2D1_RECT_F primaryRect = m_Flip
                                              ? D2D1::RectF(x, top, x + 1.0f, top + static_cast<float>(primarySize))
                                              : D2D1::RectF(x, bottom - static_cast<float>(primarySize), x + 1.0f, bottom);
                DrawSpan(context, primaryRect, gradientRect, &m_PrimaryGradient, m_PrimaryColor, m_PrimaryAlpha);
            }
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
