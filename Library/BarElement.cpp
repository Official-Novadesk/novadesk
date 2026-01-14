/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BarElement.h"

BarElement::BarElement(const std::wstring& id, int x, int y, int w, int h, float value, BarOrientation orientation)
    : Element(ELEMENT_BAR, id, x, y, w, h), m_Value(value), m_Orientation(orientation)
{
}

void BarElement::Render(Gdiplus::Graphics& graphics) {
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;

    // 1. Render Background (Base Element)
    RenderBackground(graphics);

    // 2. Calculate Bar Area
    if (m_HasBarColor || m_HasBarGradient) {
        float val = (m_Value < 0.0f) ? 0.0f : (m_Value > 1.0f) ? 1.0f : m_Value;
        
        Gdiplus::RectF barRect;
        if (m_Orientation == BAR_HORIZONTAL) {
            barRect = Gdiplus::RectF((Gdiplus::REAL)m_X, (Gdiplus::REAL)m_Y, (Gdiplus::REAL)(w * val), (Gdiplus::REAL)h);
        } else {
            float barH = h * val;
            barRect = Gdiplus::RectF((Gdiplus::REAL)m_X, (Gdiplus::REAL)(m_Y + h - barH), (Gdiplus::REAL)w, (Gdiplus::REAL)barH);
        }

        if (barRect.Width > 0 && barRect.Height > 0) {
            Gdiplus::Brush* barBrush = nullptr;
            
            if (m_HasBarGradient) {
                Gdiplus::Color color1(m_BarAlpha, GetRValue(m_BarColor), GetGValue(m_BarColor), GetBValue(m_BarColor));
                Gdiplus::Color color2(m_BarAlpha2, GetRValue(m_BarColor2), GetGValue(m_BarColor2), GetBValue(m_BarColor2));
                
                barBrush = new Gdiplus::LinearGradientBrush(barRect, color1, color2, m_BarGradientAngle);
            } else {
                Gdiplus::Color barColor(m_BarAlpha, GetRValue(m_BarColor), GetGValue(m_BarColor), GetBValue(m_BarColor));
                barBrush = new Gdiplus::SolidBrush(barColor);
            }

            if (m_BarCornerRadius > 0) {
                Gdiplus::GraphicsPath* path = new Gdiplus::GraphicsPath();
                int d = m_BarCornerRadius * 2;
                if (d > (int)barRect.Width) d = (int)barRect.Width;
                if (d > (int)barRect.Height) d = (int)barRect.Height;

                if (d > 0) {
                    path->AddArc(barRect.X, barRect.Y, (Gdiplus::REAL)d, (Gdiplus::REAL)d, 180, 90);
                    path->AddArc(barRect.X + barRect.Width - d, barRect.Y, (Gdiplus::REAL)d, (Gdiplus::REAL)d, 270, 90);
                    path->AddArc(barRect.X + barRect.Width - d, barRect.Y + barRect.Height - d, (Gdiplus::REAL)d, (Gdiplus::REAL)d, 0, 90);
                    path->AddArc(barRect.X, barRect.Y + barRect.Height - d, (Gdiplus::REAL)d, (Gdiplus::REAL)d, 90, 90);
                    path->CloseFigure();
                    graphics.FillPath(barBrush, path);
                } else {
                    graphics.FillRectangle(barBrush, barRect);
                }
                delete path;
            } else {
                graphics.FillRectangle(barBrush, barRect);
            }

            delete barBrush;
        }
    }

    // 3. Render Bevel (Base Element)
    RenderBevel(graphics);
}
