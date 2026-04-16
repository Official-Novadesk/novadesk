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
    const float right = left + width - 1.0f;
    const float bottom = top + height - 1.0f;
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

        for (int i = 0; i < numPoints; ++i)
        {
            float val = m_Data[i];
            float norm = (range <= 0.000001f) ? 0.0f : (val - minV) / range;
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;

            int capacity = (m_MaxPoints > numPoints) ? m_MaxPoints : numPoints;
            float dx = (capacity > 1) ? (width - 1.0f) / (float)(capacity - 1) : 0.0f;

            float px = m_GraphStartLeft ? (left + (float)(numPoints - 1 - i) * dx) : (right - (float)(numPoints - 1 - i) * dx);
            float py = !m_Flip ? (bottom - norm * (height - 1.0f)) : (top + norm * (height - 1.0f));

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
