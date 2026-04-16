/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "LineElement.h"

#include "Direct2DHelper.h"

#include <algorithm>
#include <cmath>

LineElement::LineElement(const std::wstring& id, int x, int y, int w, int h)
    : Element(ELEMENT_LINE, id, x, y, w, h)
{
    EnsureStorage();
}

void LineElement::SetLineCount(int count)
{
    m_LineCount = (count < 1) ? 1 : count;
    EnsureStorage();
}

void LineElement::SetDataSets(const std::vector<std::vector<float>>& dataSets)
{
    m_DataSets = dataSets;
    if (m_MaxPoints > 0)
    {
        for (auto& series : m_DataSets)
        {
            if (series.size() > (size_t)m_MaxPoints)
            {
                series.erase(series.begin(), series.end() - m_MaxPoints);
            }
        }
    }
    EnsureStorage();
}

void LineElement::SetLineColors(const std::vector<COLORREF>& colors, const std::vector<BYTE>& alphas)
{
    m_LineColors = colors;
    m_LineAlphas = alphas;
    EnsureStorage();
}

void LineElement::SetLineGradients(const std::vector<GradientInfo>& gradients)
{
    m_LineGradients = gradients;
    EnsureStorage();
}

void LineElement::SetScaleValues(const std::vector<float>& scaleValues)
{
    m_ScaleValues = scaleValues;
    EnsureStorage();
}

void LineElement::SetLineWidth(float width)
{
    m_LineWidth = (width < 1.0f) ? 1.0f : width;
}

void LineElement::SetMaxPoints(int maxPoints)
{
    m_MaxPoints = (maxPoints < 0) ? 0 : maxPoints;
    if (m_MaxPoints > 0)
    {
        for (auto& series : m_DataSets)
        {
            if (series.size() > (size_t)m_MaxPoints)
            {
                series.erase(series.begin(), series.end() - m_MaxPoints);
            }
        }
    }
}

void LineElement::SetScaleRange(float minValue, float maxValue)
{
    if (maxValue < minValue)
    {
        std::swap(minValue, maxValue);
    }
    if (fabsf(maxValue - minValue) < 0.000001f)
    {
        maxValue = minValue + 1.0f;
    }
    m_ScaleMin = minValue;
    m_ScaleMax = maxValue;
}

void LineElement::EnsureStorage()
{
    if (m_LineCount < 1)
    {
        m_LineCount = 1;
    }

    if ((int)m_DataSets.size() < m_LineCount)
    {
        m_DataSets.resize((size_t)m_LineCount);
    }
    else if ((int)m_DataSets.size() > m_LineCount)
    {
        m_DataSets.resize((size_t)m_LineCount);
    }

    if ((int)m_LineColors.size() < m_LineCount)
    {
        m_LineColors.resize((size_t)m_LineCount, RGB(255, 255, 255));
    }
    else if ((int)m_LineColors.size() > m_LineCount)
    {
        m_LineColors.resize((size_t)m_LineCount);
    }

    if ((int)m_LineAlphas.size() < m_LineCount)
    {
        m_LineAlphas.resize((size_t)m_LineCount, 255);
    }
    else if ((int)m_LineAlphas.size() > m_LineCount)
    {
        m_LineAlphas.resize((size_t)m_LineCount);
    }

    if ((int)m_LineGradients.size() < m_LineCount)
    {
        m_LineGradients.resize((size_t)m_LineCount);
    }
    else if ((int)m_LineGradients.size() > m_LineCount)
    {
        m_LineGradients.resize((size_t)m_LineCount);
    }

    if ((int)m_ScaleValues.size() < m_LineCount)
    {
        m_ScaleValues.resize((size_t)m_LineCount, 1.0f);
    }
    else if ((int)m_ScaleValues.size() > m_LineCount)
    {
        m_ScaleValues.resize((size_t)m_LineCount);
    }

    for (float& scale : m_ScaleValues)
    {
        if (!std::isfinite(scale))
        {
            scale = 1.0f;
        }
    }
}

bool LineElement::BuildAutoRange(float& outMin, float& outMax) const
{
    if (!m_AutoRange)
    {
        outMin = m_ScaleMin;
        outMax = m_ScaleMax;
        if (fabsf(outMax - outMin) < 0.000001f)
        {
            outMax = outMin + 1.0f;
        }
        return true;
    }

    bool hasValue = false;
    float minV = 0.0f;
    float maxV = 0.0f;

    for (size_t seriesIndex = 0; seriesIndex < m_DataSets.size(); ++seriesIndex)
    {
        const auto& series = m_DataSets[seriesIndex];
        float lineScale = 1.0f;
        if (seriesIndex < m_ScaleValues.size())
        {
            lineScale = m_ScaleValues[seriesIndex];
        }

        for (float v : series)
        {
            v *= lineScale;
            if (!hasValue)
            {
                minV = maxV = v;
                hasValue = true;
            }
            else
            {
                minV = std::min(minV, v);
                maxV = std::max(maxV, v);
            }
        }
    }

    if (!hasValue)
    {
        outMin = 0.0f;
        outMax = 1.0f;
        return false;
    }

    if (fabsf(maxV - minV) < 0.000001f)
    {
        outMin = minV - 0.5f;
        outMax = maxV + 0.5f;
        return true;
    }

    outMin = minV;
    outMax = maxV;
    return true;
}

bool LineElement::MapPoint(int dataIndex, int pointIndex, int totalPoints, int capacityPoints, float minValue, float maxValue, D2D1_POINT_2F& outPoint)
{
    if (dataIndex < 0 || dataIndex >= (int)m_DataSets.size() || totalPoints <= 0 || pointIndex < 0 || pointIndex >= totalPoints)
    {
        return false;
    }

    const auto& series = m_DataSets[(size_t)dataIndex];
    if (series.empty())
    {
        return false;
    }

    float w = (float)GetWidth();
    float h = (float)GetHeight();
    if (w <= 1.0f || h <= 1.0f)
    {
        return false;
    }

    const float left = (float)m_X;
    const float top = (float)m_Y;
    const float valueRange = maxValue - minValue;

    if (capacityPoints < totalPoints)
    {
        capacityPoints = totalPoints;
    }

    int valueIndex = pointIndex;
    if (valueIndex >= (int)series.size())
    {
        valueIndex = (int)series.size() - 1;
    }

    float value = series[(size_t)valueIndex];
    if ((size_t)dataIndex < m_ScaleValues.size())
    {
        value *= m_ScaleValues[(size_t)dataIndex];
    }
    float normalized = (valueRange <= 0.000001f) ? 0.5f : ((value - minValue) / valueRange);
    normalized = std::max(0.0f, std::min(1.0f, normalized));

    if (!m_GraphHorizontalOrientation)
    {
        const float dx = (capacityPoints > 1) ? ((w - 1.0f) / (float)(capacityPoints - 1)) : 0.0f;
        const int axisIndex = m_GraphStartLeft ? (totalPoints - 1 - pointIndex) : ((capacityPoints - totalPoints) + pointIndex);
        float x = left + (float)axisIndex * dx;
        float y = !m_Flip ? (top + (h - 1.0f) - normalized * (h - 1.0f)) : (top + normalized * (h - 1.0f));
        outPoint = D2D1::Point2F(x, y);
        return true;
    }

    const float dy = (capacityPoints > 1) ? ((h - 1.0f) / (float)(capacityPoints - 1)) : 0.0f;
    const int axisIndex = !m_Flip ? ((capacityPoints - totalPoints) + pointIndex) : (totalPoints - 1 - pointIndex);
    float y = top + (float)axisIndex * dy;
    float x = m_GraphStartLeft ? (left + normalized * (w - 1.0f)) : (left + (w - 1.0f) - normalized * (w - 1.0f));
    outPoint = D2D1::Point2F(x, y);
    return true;
}

float LineElement::DistancePointToSegment(const D2D1_POINT_2F& p, const D2D1_POINT_2F& a, const D2D1_POINT_2F& b)
{
    const float vx = b.x - a.x;
    const float vy = b.y - a.y;
    const float wx = p.x - a.x;
    const float wy = p.y - a.y;
    const float c1 = vx * wx + vy * wy;

    if (c1 <= 0.0f)
    {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }

    const float c2 = vx * vx + vy * vy;
    if (c2 <= 0.000001f)
    {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }

    if (c1 >= c2)
    {
        const float dx = p.x - b.x;
        const float dy = p.y - b.y;
        return sqrtf(dx * dx + dy * dy);
    }

    const float t = c1 / c2;
    const float px = a.x + t * vx;
    const float py = a.y + t * vy;
    const float dx = p.x - px;
    const float dy = p.y - py;
    return sqrtf(dx * dx + dy * dy);
}

bool LineElement::HitTest(int x, int y)
{
    GfxRect bounds = GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    D2D1::Matrix3x2F matrix;
    bool needsTransform = (m_HasTransformMatrix || m_Rotate != 0.0f);

    if (needsTransform)
    {
        if (m_HasTransformMatrix)
        {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]);
        }
        else
        {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (!matrix.Invert())
            return false;
    }

    D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
    if (needsTransform)
    {
        p = matrix.TransformPoint(p);
    }

    if (p.x < bounds.X || p.x >= bounds.X + bounds.Width || p.y < bounds.Y || p.y >= bounds.Y + bounds.Height)
    {
        return false;
    }

    float minV = 0.0f;
    float maxV = 1.0f;
    BuildAutoRange(minV, maxV);

    const float tolerance = std::max(2.0f, (m_LineWidth * 0.5f) + 1.0f);
    for (int i = 0; i < m_LineCount && i < (int)m_DataSets.size(); ++i)
    {
        const auto& series = m_DataSets[(size_t)i];
        if (series.size() < 2)
            continue;
        const int totalPoints = (int)series.size();
        const int capacityPoints = (m_MaxPoints > totalPoints) ? m_MaxPoints : totalPoints;

        D2D1_POINT_2F a{};
        if (!MapPoint(i, 0, totalPoints, capacityPoints, minV, maxV, a))
            continue;

        D2D1_POINT_2F base{};
        if (!m_GraphHorizontalOrientation)
        {
            const float baseY = !m_Flip ? ((float)m_Y + (float)GetHeight() - 1.0f) : (float)m_Y;
            base = D2D1::Point2F(a.x, baseY);
        }
        else
        {
            const float baseX = m_GraphStartLeft ? (float)m_X : ((float)m_X + (float)GetWidth() - 1.0f);
            base = D2D1::Point2F(baseX, a.y);
        }
        if (DistancePointToSegment(p, base, a) <= tolerance)
        {
            return true;
        }

        for (int j = 1; j < totalPoints; ++j)
        {
            D2D1_POINT_2F b{};
            if (!MapPoint(i, j, totalPoints, capacityPoints, minV, maxV, b))
                continue;

            if (DistancePointToSegment(p, a, b) <= tolerance)
            {
                return true;
            }
            a = b;
        }
    }

    if (m_HorizontalLines)
    {
        const float left = (float)m_X;
        const float top = (float)m_Y;
        const float right = left + (float)GetWidth() - 1.0f;
        const float h = (float)GetHeight();
        const int markerCount = 4;
        for (int i = 1; i <= markerCount; ++i)
        {
            float yLine = top + ((float)i * (h / (float)(markerCount + 1)));
            D2D1_POINT_2F a = D2D1::Point2F(left, yLine);
            D2D1_POINT_2F b = D2D1::Point2F(right, yLine);
            if (DistancePointToSegment(p, a, b) <= tolerance)
            {
                return true;
            }
        }
    }

    return false;
}

void LineElement::Render(ID2D1DeviceContext* context)
{
    if (!context)
        return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    const float width = (float)GetWidth();
    const float height = (float)GetHeight();
    if (width <= 1.0f || height <= 1.0f)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    float minV = 0.0f;
    float maxV = 1.0f;
    BuildAutoRange(minV, maxV);

    Microsoft::WRL::ComPtr<ID2D1StrokeStyle> strokeStyle;
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (factory)
    {
#if defined(D2D1_STROKE_STYLE_PROPERTIES1)
        D2D1_STROKE_STYLE_PROPERTIES1 strokeProps1 = D2D1::StrokeStyleProperties1();
        strokeProps1.transformType = m_StrokeType;
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> strokeStyle1;
        if (SUCCEEDED(factory->CreateStrokeStyle(strokeProps1, nullptr, 0, strokeStyle1.GetAddressOf())))
        {
            strokeStyle = strokeStyle1;
        }
        else
#endif
        {
            D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties();
            factory->CreateStrokeStyle(strokeProps, nullptr, 0, strokeStyle.GetAddressOf());
        }
    }

    if (m_HorizontalLines)
    {
        Microsoft::WRL::ComPtr<ID2D1Brush> markerBrush;
        const D2D1_RECT_F elementRect = D2D1::RectF((float)m_X, (float)m_Y, (float)m_X + width, (float)m_Y + height);
        Direct2D::CreateBrushFromGradientOrColor(
            context,
            elementRect,
            &m_HorizontalLineGradient,
            m_HorizontalLineColor,
            m_HorizontalLineAlpha / 255.0f,
            markerBrush.GetAddressOf());
        if (markerBrush)
        {
            const float left = (float)m_X;
            const float right = left + width - 1.0f;
            const float top = (float)m_Y;
            const int markerCount = 4;
            for (int i = 1; i <= markerCount; ++i)
            {
                float y = top + ((float)i * (height / (float)(markerCount + 1)));
                context->DrawLine(
                    D2D1::Point2F(left, y),
                    D2D1::Point2F(right, y),
                    markerBrush.Get(),
                    1.0f,
                    strokeStyle.Get());
            }
        }
    }

    EnsureStorage();
    const float strokeWidth = (m_LineWidth < 1.0f) ? 1.0f : m_LineWidth;

    for (int i = 0; i < m_LineCount && i < (int)m_DataSets.size(); ++i)
    {
        const auto& series = m_DataSets[(size_t)i];
        if (series.size() < 2)
            continue;
        const int totalPoints = (int)series.size();
        const int capacityPoints = (m_MaxPoints > totalPoints) ? m_MaxPoints : totalPoints;

        Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
        if (!factory || FAILED(factory->CreatePathGeometry(geometry.GetAddressOf())))
            continue;

        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geometry->Open(sink.GetAddressOf())))
            continue;

        D2D1_POINT_2F first{};
        if (!MapPoint(i, 0, totalPoints, capacityPoints, minV, maxV, first))
        {
            sink->Close();
            continue;
        }
        D2D1_POINT_2F base{};
        if (!m_GraphHorizontalOrientation)
        {
            const float baseY = !m_Flip ? ((float)m_Y + height - 1.0f) : (float)m_Y;
            base = D2D1::Point2F(first.x, baseY);
        }
        else
        {
            const float baseX = m_GraphStartLeft ? (float)m_X : ((float)m_X + width - 1.0f);
            base = D2D1::Point2F(baseX, first.y);
        }

        sink->BeginFigure(base, D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(first);
        for (int j = 1; j < totalPoints; ++j)
        {
            D2D1_POINT_2F p{};
            if (MapPoint(i, j, totalPoints, capacityPoints, minV, maxV, p))
            {
                sink->AddLine(p);
            }
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        if (FAILED(sink->Close()))
        {
            continue;
        }

        Microsoft::WRL::ComPtr<ID2D1Brush> brush;
        const D2D1_RECT_F elementRect = D2D1::RectF((float)m_X, (float)m_Y, (float)m_X + width, (float)m_Y + height);
        const GradientInfo* lineGradient = nullptr;
        if ((size_t)i < m_LineGradients.size())
        {
            lineGradient = &m_LineGradients[(size_t)i];
        }
        Direct2D::CreateBrushFromGradientOrColor(
            context,
            elementRect,
            lineGradient,
            m_LineColors[(size_t)i],
            m_LineAlphas[(size_t)i] / 255.0f,
            brush.GetAddressOf());

        if (brush)
        {
            context->DrawGeometry(geometry.Get(), brush.Get(), strokeWidth, strokeStyle.Get());
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
