/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BarElement.h"
#include "Direct2DHelper.h"

BarElement::BarElement(const std::wstring& id, int x, int y, int w, int h, float value, BarOrientation orientation)
    : Element(ELEMENT_BAR, id, x, y, w, h), m_Value(value), m_Orientation(orientation)
{
}

void BarElement::Render(ID2D1DeviceContext* context) {
    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
    
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;

    D2D1_MATRIX_3X2_F originalTransform;
    context->GetTransform(&originalTransform);

    // Apply rotation around center
    if (m_Rotate != 0.0f) {
        GfxRect bounds = GetBounds();
        float centerX = bounds.X + bounds.Width / 2.0f;
        float centerY = bounds.Y + bounds.Height / 2.0f;
        context->SetTransform(D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY)) * originalTransform);
    }

    // Draw background first
    RenderBackground(context);

    if (m_HasBarColor || m_HasBarGradient) {
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
            
            if (m_HasBarGradient) {
                D2D1_POINT_2F start = Direct2D::FindEdgePoint(m_BarGradientAngle + 180.0f, barRect);
                D2D1_POINT_2F end = Direct2D::FindEdgePoint(m_BarGradientAngle, barRect);
                
                Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> lgBrush;
                Direct2D::CreateLinearGradientBrush(context, start, end, m_BarColor, m_BarAlpha / 255.0f, m_BarColor2, m_BarAlpha2 / 255.0f, lgBrush.GetAddressOf());
                barBrush = lgBrush;
            } else {
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> sBrush;
                Direct2D::CreateSolidBrush(context, m_BarColor, m_BarAlpha / 255.0f, sBrush.GetAddressOf());
                barBrush = sBrush;
            }

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
    context->SetTransform(originalTransform);
}
