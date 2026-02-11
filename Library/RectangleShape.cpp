/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "RectangleShape.h"
#include "Direct2DHelper.h"

RectangleShape::RectangleShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

RectangleShape::~RectangleShape()
{
}

bool RectangleShape::HitTestLocal(const D2D1_POINT_2F& point)
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

bool RectangleShape::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    if (!factory) return false;
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded;
    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;

    if (FAILED(factory->CreateRoundedRectangleGeometry(rect, rounded.GetAddressOf()))) return false;
    geometry = rounded;
    return true;
}

void RectangleShape::Render(ID2D1DeviceContext* context)
{
    if (IsConsumed()) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> pStrokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> pFillBrush;
    TryCreateStrokeBrush(context, pStrokeBrush);
    TryCreateFillBrush(context, pFillBrush);

    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY; 

    if (pFillBrush) {
        context->FillRoundedRectangle(rect, pFillBrush.Get());
    }
    if (pStrokeBrush) {
        UpdateStrokeStyle(context);
        context->DrawRoundedRectangle(rect, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
    }
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
