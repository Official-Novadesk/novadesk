/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "AreaGraphElement.h"
#include "Direct2DHelper.h"
#include <algorithm>
#include <cmath>

namespace
{
    float DistancePointToSegment(const D2D1_POINT_2F &p, const D2D1_POINT_2F &a, const D2D1_POINT_2F &b)
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
        if (c2 <= c1)
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

    bool PointInPolygon(const D2D1_POINT_2F &p, const std::vector<D2D1_POINT_2F> &poly)
    {
        if (poly.size() < 3)
            return false;

        bool inside = false;
        size_t j = poly.size() - 1;
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const D2D1_POINT_2F &pi = poly[i];
            const D2D1_POINT_2F &pj = poly[j];
            const bool intersect = ((pi.y > p.y) != (pj.y > p.y)) &&
                                   (p.x < (pj.x - pi.x) * (p.y - pi.y) / ((pj.y - pi.y) == 0.0f ? 1.0f : (pj.y - pi.y)) + pi.x);
            if (intersect)
                inside = !inside;
            j = i;
        }
        return inside;
    }
}

AreaGraphElement::AreaGraphElement(const std::wstring &id, int x, int y, int w, int h)
    : Element(ELEMENT_AREA_GRAPH, id, x, y, w, h)
{
}

void AreaGraphElement::SetData(const std::vector<float> &data)
{
    if (m_MaxPoints > 0 && data.size() > (size_t)m_MaxPoints)
    {
        m_Data.assign(data.end() - m_MaxPoints, data.end());
    }
    else
    {
        m_Data = data;
    }
}

void AreaGraphElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show || !context)
        return;

    const float width = (float)GetWidth();
    const float height = (float)GetHeight();
    if (width <= 1.0f || height <= 1.0f)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    const float left = (float)m_X + m_PaddingLeft;
    const float top = (float)m_Y + m_PaddingTop;
    const float right = (float)(m_X + GetWidth()) - 1.0f - m_PaddingRight;
    const float bottom = (float)(m_Y + GetHeight()) - 1.0f - m_PaddingBottom;
    const D2D1_RECT_F elementRect = D2D1::RectF(left, top, right + 1.0f, bottom + 1.0f);

    // 1. Draw Grid
    if (m_GridVisible && m_GridAlpha > 0)
    {
        Microsoft::WRL::ComPtr<ID2D1Brush> gridBrush;
        Direct2D::CreateBrushFromGradientOrColor(
            context,
            elementRect,
            &m_GridGradient,
            m_GridColor,
            m_GridAlpha / 255.0f,
            gridBrush.GetAddressOf());
        if (gridBrush)
        {
            // Vertical lines
            if (m_GridXSpacing > 0)
            {
                for (float x = left + m_GridXSpacing; x < right; x += m_GridXSpacing)
                {
                    context->DrawLine(D2D1::Point2F(x, top), D2D1::Point2F(x, bottom), gridBrush.Get(), 1.0f);
                }
            }
            // Horizontal lines
            if (m_GridYSpacing > 0)
            {
                for (float y = top + m_GridYSpacing; y < bottom; y += m_GridYSpacing)
                {
                    context->DrawLine(D2D1::Point2F(left, y), D2D1::Point2F(right, y), gridBrush.Get(), 1.0f);
                }
            }
        }
    }

    // 2. Plot Data
    if (!m_Data.empty())
    {
        float minV = 0.0f;
        float maxV = 1.0f;
        BuildAutoRange(minV, maxV);
        const float range = maxV - minV;

        const int numPoints = (int)m_Data.size();
        std::vector<D2D1_POINT_2F> points;
        points.reserve(numPoints);

        const float drawWidth  = right - left;
        const float drawHeight = bottom - top;

        for (int i = 0; i < numPoints; ++i)
        {
            float val = m_Data[i];
            float norm = (range <= 0.000001f) ? 0.0f : (val - minV) / range;
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;

            int capacity = (m_MaxPoints > numPoints) ? m_MaxPoints : numPoints;
            float dx = (capacity > 1) ? drawWidth / (float)(capacity - 1) : 0.0f;

            float px = m_GraphStartLeft ? (left + (float)(numPoints - 1 - i) * dx) : (right - (float)(numPoints - 1 - i) * dx);
            float py = !m_Flip ? (bottom - norm * drawHeight) : (top + norm * drawHeight);

            points.push_back(D2D1::Point2F(px, py));
        }

        if (points.size() >= 2)
        {
            Microsoft::WRL::ComPtr<ID2D1Factory> factory;
            context->GetFactory(factory.GetAddressOf());
            if (factory)
            {
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
                if (SUCCEEDED(factory->CreatePathGeometry(geometry.GetAddressOf())))
                {
                    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                    if (SUCCEEDED(geometry->Open(sink.GetAddressOf())))
                    {
                        // Area Fill Path
                        sink->BeginFigure(D2D1::Point2F(points[0].x, bottom), D2D1_FIGURE_BEGIN_FILLED);
                        for (const auto &p : points)
                        {
                            sink->AddLine(p);
                        }
                        sink->AddLine(D2D1::Point2F(points.back().x, bottom));
                        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                        sink->Close();

                        // Draw Fill
                        Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
                        Direct2D::CreateBrushFromGradientOrColor(
                            context,
                            elementRect,
                            &m_FillGradient,
                            m_FillColor,
                            m_FillAlpha / 255.0f,
                            fillBrush.GetAddressOf());
                        if (fillBrush)
                        {
                            context->FillGeometry(geometry.Get(), fillBrush.Get());
                        }
                    }
                }

                // Top Line Path (separately to ensure clean stroke)
                {
                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> lineGeom;
                    if (SUCCEEDED(factory->CreatePathGeometry(lineGeom.GetAddressOf())))
                    {
                        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                        if (SUCCEEDED(lineGeom->Open(sink.GetAddressOf())))
                        {
                            sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_HOLLOW);
                            for (size_t i = 1; i < points.size(); ++i)
                            {
                                sink->AddLine(points[i]);
                            }
                            sink->EndFigure(D2D1_FIGURE_END_OPEN);
                            sink->Close();

                            Microsoft::WRL::ComPtr<ID2D1Brush> lineBrush;
                            Direct2D::CreateBrushFromGradientOrColor(
                                context,
                                elementRect,
                                &m_LineGradient,
                                m_LineColor,
                                1.0f,
                                lineBrush.GetAddressOf());
                            if (lineBrush)
                            {
                                context->DrawGeometry(lineGeom.Get(), lineBrush.Get(), m_LineWidth);
                            }
                        }
                    }
                }
            }
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

bool AreaGraphElement::HitTest(int x, int y)
{
    if (!Element::HitTest(x, y))
        return false;

    if (!GetPixelHitTest())
        return true;

    if ((m_HasSolidColor && m_SolidAlpha > 0) || (m_SolidGradient.type != GRADIENT_NONE))
        return true;

    float targetX = static_cast<float>(x);
    float targetY = static_cast<float>(y);
    if (m_HasTransformMatrix || m_Rotate != 0.0f)
    {
        GfxRect bounds = GetBounds();
        float centerX = bounds.X + bounds.Width / 2.0f;
        float centerY = bounds.Y + bounds.Height / 2.0f;

        D2D1::Matrix3x2F matrix;
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

        if (matrix.Invert())
        {
            const D2D1_POINT_2F transformed = matrix.TransformPoint(D2D1::Point2F(targetX, targetY));
            targetX = transformed.x;
            targetY = transformed.y;
        }
    }

    const float width = static_cast<float>(GetWidth());
    const float height = static_cast<float>(GetHeight());
    if (width <= 1.0f || height <= 1.0f)
        return false;

    const float left = static_cast<float>(m_X + m_PaddingLeft);
    const float top = static_cast<float>(m_Y + m_PaddingTop);
    const float right = left + width - 1.0f;
    const float bottom = top + height - 1.0f;
    if (targetX < left || targetX > right || targetY < top || targetY > bottom)
        return false;

    if (m_Data.empty())
        return false;

    float minV = 0.0f;
    float maxV = 1.0f;
    BuildAutoRange(minV, maxV);
    const float range = maxV - minV;

    const int numPoints = static_cast<int>(m_Data.size());
    std::vector<D2D1_POINT_2F> points;
    points.reserve(static_cast<size_t>(numPoints));

    for (int i = 0; i < numPoints; ++i)
    {
        float val = m_Data[static_cast<size_t>(i)];
        float norm = (range <= 0.000001f) ? 0.0f : (val - minV) / range;
        if (norm < 0.0f)
            norm = 0.0f;
        if (norm > 1.0f)
            norm = 1.0f;

        int capacity = (m_MaxPoints > numPoints) ? m_MaxPoints : numPoints;
        float dx = (capacity > 1) ? (width - 1.0f) / static_cast<float>(capacity - 1) : 0.0f;

        float px = m_GraphStartLeft ? (left + static_cast<float>(numPoints - 1 - i) * dx) : (right - static_cast<float>(numPoints - 1 - i) * dx);
        float py = !m_Flip ? (bottom - norm * (height - 1.0f)) : (top + norm * (height - 1.0f));
        points.push_back(D2D1::Point2F(px, py));
    }

    const D2D1_POINT_2F p = D2D1::Point2F(targetX, targetY);
    const bool fillVisible = (m_FillAlpha > 0) || (m_FillGradient.type != GRADIENT_NONE);
    if (fillVisible && points.size() >= 2)
    {
        std::vector<D2D1_POINT_2F> poly;
        poly.reserve(points.size() + 2);
        poly.push_back(D2D1::Point2F(points[0].x, bottom));
        for (const auto &pt : points)
            poly.push_back(pt);
        poly.push_back(D2D1::Point2F(points.back().x, bottom));

        if (PointInPolygon(p, poly))
            return true;
    }

    if (m_LineWidth > 0.0f && points.size() >= 2)
    {
        const float tolerance = (std::max)(1.0f, (m_LineWidth * 0.5f) + 1.0f);
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            if (DistancePointToSegment(p, points[i], points[i + 1]) <= tolerance)
                return true;
        }
    }

    return false;
}

bool AreaGraphElement::BuildAutoRange(float &outMin, float &outMax) const
{
    if (!m_AutoRange)
    {
        outMin = m_MinValue;
        outMax = m_MaxValue;
        return true;
    }

    if (m_Data.empty())
    {
        outMin = 0.0f;
        outMax = 1.0f;
        return false;
    }

    float minV = m_Data[0];
    float maxV = m_Data[0];
    for (float v : m_Data)
    {
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
    }

    if (fabsf(maxV - minV) < 0.000001f)
    {
        outMin = minV - 0.5f;
        outMax = maxV + 0.5f;
    }
    else
    {
        outMin = minV;
        outMax = maxV;
    }
    return true;
}
