/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BarElement.h"
#include "Direct2DHelper.h"
#include "Logging.h"

BarElement::BarElement(const std::wstring& id, int x, int y, int w, int h, float value, BarOrientation orientation)
    : Element(ELEMENT_BAR, id, x, y, w, h), m_Value(value), m_Orientation(orientation)
{
}

bool BarElement::HitTest(int x, int y)
{
    // Transform the point into local (unrotated) space if needed.
    GfxRect bounds = GetBackgroundBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    D2D1::Matrix3x2F matrix;
    bool needsTransform = (m_HasTransformMatrix || m_Rotate != 0.0f);

    if (needsTransform) {
        if (m_HasTransformMatrix) {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]
            );
        } else {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (!matrix.Invert()) return false;
    }

    D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
    if (needsTransform) {
        p = matrix.TransformPoint(p);
    }

    if (p.x < bounds.X || p.x >= bounds.X + bounds.Width ||
        p.y < bounds.Y || p.y >= bounds.Y + bounds.Height) {
        return false;
    }

    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return false;

    auto fillContainsPoint = [&](const D2D1_RECT_F& rect, float radius) -> bool {
        if (rect.right <= rect.left || rect.bottom <= rect.top) return false;
        BOOL hit = FALSE;

        if (radius > 0.0f) {
            Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> geometry;
            D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, radius, radius);
            if (FAILED(factory->CreateRoundedRectangleGeometry(rounded, geometry.GetAddressOf()))) return false;
            if (SUCCEEDED(geometry->FillContainsPoint(p, nullptr, &hit)) && hit) return true;
        } else {
            Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> geometry;
            if (FAILED(factory->CreateRectangleGeometry(rect, geometry.GetAddressOf()))) return false;
            if (SUCCEEDED(geometry->FillContainsPoint(p, nullptr, &hit)) && hit) return true;
        }

        return false;
    };

    // Background hit-test
    if (m_HasSolidColor && (m_SolidAlpha > 0 || m_SolidGradient.type != GRADIENT_NONE)) {
        D2D1_RECT_F bgRect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + GetWidth()), (float)(m_Y + GetHeight()));
        if (fillContainsPoint(bgRect, (float)m_CornerRadius)) return true;
    }

    // Bar hit-test
    if (m_HasBarColor || m_BarGradient.type != GRADIENT_NONE) {
        float val = (m_Value < 0.0f) ? 0.0f : (m_Value > 1.0f) ? 1.0f : m_Value;
        if (m_BarAlpha > 0 || m_BarGradient.type != GRADIENT_NONE) {
            D2D1_RECT_F barRect;
            int w = GetWidth();
            int h = GetHeight();
            if (m_Orientation == BAR_HORIZONTAL) {
                barRect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + w * val), (float)(m_Y + h));
            } else {
                float barH = h * val;
                barRect = D2D1::RectF((float)m_X, (float)(m_Y + h - barH), (float)(m_X + w), (float)(m_Y + h));
            }

            if (fillContainsPoint(barRect, (float)m_BarCornerRadius)) return true;
        }
    }

    return false;
}

void BarElement::Render(ID2D1DeviceContext* context) {
    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
    
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    // Draw background first
    RenderBackground(context);

    if (m_HasBarColor || m_BarGradient.type != GRADIENT_NONE) {
        float val = (m_Value < 0.0f) ? 0.0f : (m_Value > 1.0f) ? 1.0f : m_Value;
        
        D2D1_RECT_F barRect;
        if (m_Orientation == BAR_HORIZONTAL) {
            barRect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + w * val), (float)(m_Y + h));
        } else {
            float barH = h * val;
            barRect = D2D1::RectF((float)m_X, (float)(m_Y + h - barH), (float)(m_X + w), (float)(m_Y + h));
        }

        if (barRect.right > barRect.left && barRect.bottom > barRect.top) {
            Microsoft::WRL::ComPtr<ID2D1Brush> barBrush;
            Direct2D::CreateBrushFromGradientOrColor(
                context,
                barRect,
                &m_BarGradient,
                m_BarColor,
                m_BarAlpha / 255.0f,
                barBrush.GetAddressOf()
            );

            if (barBrush) {
                if (m_BarCornerRadius > 0) {
                    D2D1_ROUNDED_RECT roundedBar = D2D1::RoundedRect(barRect, (float)m_BarCornerRadius, (float)m_BarCornerRadius);
                    context->FillRoundedRectangle(roundedBar, barBrush.Get());
                } else {
                    context->FillRectangle(barRect, barBrush.Get());
                }
            }
        }
    }

    // Draw bevel last
    RenderBevel(context);

    // Restore transform
    RestoreRenderTransform(context, originalTransform);
}
