/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "LineShape.h"
#include "Direct2DHelper.h"
#include <cmath>

LineShape::LineShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

LineShape::~LineShape()
{
}

GfxRect LineShape::GetBounds()
{
    float minX = (m_StartX < m_EndX) ? m_StartX : m_EndX;
    float minY = (m_StartY < m_EndY) ? m_StartY : m_EndY;
    float maxX = (m_StartX > m_EndX) ? m_StartX : m_EndX;
    float maxY = (m_StartY > m_EndY) ? m_StartY : m_EndY;

    float pad = (m_StrokeWidth > 0.0f) ? (m_StrokeWidth / 2.0f) : 0.0f;

    int x = (int)floorf(minX - pad);
    int y = (int)floorf(minY - pad);
    int w = (int)ceilf((maxX - minX) + pad * 2.0f);
    int h = (int)ceilf((maxY - minY) + pad * 2.0f);

    return GfxRect(x, y, w, h);
}

bool LineShape::HitTestLocal(const D2D1_POINT_2F& point)
{
    if (!m_HasStroke || m_StrokeWidth <= 0.0f || m_StrokeAlpha <= 0) return false;

    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return false;

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry)) return false;

    EnsureStrokeStyle();
    BOOL hit = FALSE;
    if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit) {
        return true;
    }

    return false;
}

bool LineShape::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    if (!factory) return false;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(path.GetAddressOf()))) return false;

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(sink.GetAddressOf()))) return false;

    D2D1_POINT_2F start = D2D1::Point2F(m_StartX, m_StartY);
    D2D1_POINT_2F end = D2D1::Point2F(m_EndX, m_EndY);
    sink->BeginFigure(start, D2D1_FIGURE_BEGIN_HOLLOW);
    sink->AddLine(end);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();

    geometry = path;
    return true;
}

void LineShape::Render(ID2D1DeviceContext* context)
{
    if (IsConsumed()) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> pStrokeBrush;
    TryCreateStrokeBrush(context, pStrokeBrush);

    D2D1_POINT_2F start = D2D1::Point2F(m_StartX, m_StartY);
    D2D1_POINT_2F end = D2D1::Point2F(m_EndX, m_EndY);
    
    if (pStrokeBrush) {
        UpdateStrokeStyle(context);
        context->DrawLine(start, end, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
    }
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
