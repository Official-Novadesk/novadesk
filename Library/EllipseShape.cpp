/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "EllipseShape.h"

EllipseShape::EllipseShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

EllipseShape::~EllipseShape()
{
}

void EllipseShape::Render(ID2D1DeviceContext* context)
{
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
        context->DrawEllipse(ellipse, pStrokeBrush.Get(), m_StrokeWidth);
    }
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
