/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "EllipseShape.h"
#include "Direct2DHelper.h"
#include <cmath>

EllipseShape::EllipseShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

EllipseShape::~EllipseShape()
{
}

bool EllipseShape::HitTestLocal(const D2D1_POINT_2F& point)
{
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return false;

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry)) return false;

    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0) {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit) return true;
    }

    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0) {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit) {
            return true;
        }
    }

    return false;
}

bool EllipseShape::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    if (!factory) return false;

    D2D1_ELLIPSE ellipse;
    ellipse.point = D2D1::Point2F(m_X + m_Width / 2.0f, m_Y + m_Height / 2.0f);

    float rx = (m_Width > 0) ? m_Width / 2.0f : m_RadiusX;
    float ry = (m_Height > 0) ? m_Height / 2.0f : m_RadiusY;

    ellipse.radiusX = rx;
    ellipse.radiusY = ry;

    Microsoft::WRL::ComPtr<ID2D1EllipseGeometry> ellipseGeometry;
    if (FAILED(factory->CreateEllipseGeometry(ellipse, ellipseGeometry.GetAddressOf()))) return false;
    geometry = ellipseGeometry;
    return true;
}

int EllipseShape::GetAutoWidth()
{
    if (m_RadiusX > 0.0f) return (int)ceilf(m_RadiusX * 2.0f);
    if (m_RadiusY > 0.0f) return (int)ceilf(m_RadiusY * 2.0f);
    return 0;
}

int EllipseShape::GetAutoHeight()
{
    if (m_RadiusY > 0.0f) return (int)ceilf(m_RadiusY * 2.0f);
    if (m_RadiusX > 0.0f) return (int)ceilf(m_RadiusX * 2.0f);
    return 0;
}

void EllipseShape::Render(ID2D1DeviceContext* context)
{
    if (IsConsumed()) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> pStrokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> pFillBrush;
    TryCreateStrokeBrush(context, pStrokeBrush);
    TryCreateFillBrush(context, pFillBrush);

    D2D1_ELLIPSE ellipse;
    ellipse.point = D2D1::Point2F(m_X + m_Width/2.0f, m_Y + m_Height/2.0f);
    
    float rx = (m_Width > 0) ? m_Width / 2.0f : m_RadiusX;
    float ry = (m_Height > 0) ? m_Height / 2.0f : m_RadiusY;

    ellipse.radiusX = rx;
    ellipse.radiusY = ry;

    if (pFillBrush) {
        context->FillEllipse(ellipse, pFillBrush.Get());
    }
    if (pStrokeBrush) {
        UpdateStrokeStyle(context);
        context->DrawEllipse(ellipse, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
    }
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
