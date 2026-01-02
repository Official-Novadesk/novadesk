/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Element.h"
#include "Logging.h"
#include <algorithm>

Element::Element(ElementType type, const std::wstring& id, int x, int y, int width, int height)
    : m_Type(type), m_Id(id), m_X(x), m_Y(y)
{
    m_Width = (width > 0) ? width : 0;
    m_Height = (height > 0) ? height : 0;
    m_WDefined = (width > 0);
    m_HDefined = (height > 0);
}

/*
** Get the width of the element.
*/
int Element::GetWidth() { 
    if (m_WDefined) return m_Width;
    return GetAutoWidth();
}

/*
** Get the height of the element.
*/
int Element::GetHeight() { 
    if (m_HDefined) return m_Height;
    return GetAutoHeight();
}

/*
** Get the bounding box of the element.
*/
Gdiplus::Rect Element::GetBounds() {
    return Gdiplus::Rect(m_X, m_Y, GetWidth(), GetHeight());
}

/*
** Check if a point is within the element's bounds.
*/
bool Element::HitTest(int x, int y) {
    Gdiplus::Rect bounds = GetBounds();
    return (x >= bounds.X && x < bounds.X + bounds.Width &&
            y >= bounds.Y && y < bounds.Y + bounds.Height);
}

/*
** Check if the element has an action associated with it.
*/
bool Element::HasAction(UINT message, WPARAM wParam) const {
    switch (message)
    {
    case WM_LBUTTONUP:     return !m_OnLeftMouseUp.empty();
    case WM_LBUTTONDOWN:   return !m_OnLeftMouseDown.empty();
    case WM_LBUTTONDBLCLK: return !m_OnLeftDoubleClick.empty();
    case WM_RBUTTONUP:     return !m_OnRightMouseUp.empty();
    case WM_RBUTTONDOWN:   return !m_OnRightMouseDown.empty();
    case WM_RBUTTONDBLCLK: return !m_OnRightDoubleClick.empty();
    case WM_MBUTTONUP:     return !m_OnMiddleMouseUp.empty();
    case WM_MBUTTONDOWN:   return !m_OnMiddleMouseDown.empty();
    case WM_MBUTTONDBLCLK: return !m_OnMiddleDoubleClick.empty();
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1MouseUp.empty();
        else return !m_OnX2MouseUp.empty();
    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1MouseDown.empty();
        else return !m_OnX2MouseDown.empty();
    case WM_XBUTTONDBLCLK:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1DoubleClick.empty();
        else return !m_OnX2DoubleClick.empty();
    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return !m_OnScrollUp.empty();
        else return !m_OnScrollDown.empty();
    case WM_MOUSEHWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return !m_OnScrollRight.empty();
        else return !m_OnScrollLeft.empty();
    case WM_MOUSEMOVE:
        return !m_OnMouseOver.empty() || !m_OnMouseLeave.empty();
    }
    return false;
}

/*
** Check if the element has any interactive mouse action.
*/
bool Element::HasMouseAction() const {
    return !m_OnLeftMouseUp.empty() || !m_OnLeftMouseDown.empty() || !m_OnLeftDoubleClick.empty() ||
           !m_OnRightMouseUp.empty() || !m_OnRightMouseDown.empty() || !m_OnRightDoubleClick.empty() ||
           !m_OnMiddleMouseUp.empty() || !m_OnMiddleMouseDown.empty() || !m_OnMiddleDoubleClick.empty() ||
           !m_OnX1MouseUp.empty() || !m_OnX1MouseDown.empty() || !m_OnX1DoubleClick.empty() ||
           !m_OnX2MouseUp.empty() || !m_OnX2MouseDown.empty() || !m_OnX2DoubleClick.empty() ||
           !m_OnScrollUp.empty() || !m_OnScrollDown.empty() || !m_OnScrollLeft.empty() || !m_OnScrollRight.empty() ||
           !m_OnMouseOver.empty() || !m_OnMouseLeave.empty();
}

/*
** Set the padding for the element.
*/
void Element::SetPadding(int left, int top, int right, int bottom) {
    Logging::Log(LogLevel::Debug, L"Element SetPadding: [%d, %d, %d, %d]", left, top, right, bottom);
    m_PaddingLeft = left;
    m_PaddingTop = top;
    m_PaddingRight = right;
    m_PaddingBottom = bottom;
}

/*
** Render the background of the element.
*/
void Element::RenderBackground(Gdiplus::Graphics& graphics) {
    if (!m_HasSolidColor) return;

    Gdiplus::Rect bounds = GetBounds();
    int w = bounds.Width;
    int h = bounds.Height;
    if (w <= 0 || h <= 0) return;
    
    Gdiplus::Brush* brush = nullptr;
    Gdiplus::GraphicsPath* path = nullptr;

    // Create gradient or solid brush
    if (m_HasGradient) {
        auto FindEdgePoint = [&](float angle, float left, float top, float width, float height) -> Gdiplus::PointF {
            float base_angle = angle;
            while (base_angle < 0.0f) base_angle += 360.0f;
            base_angle = fmodf(base_angle, 360.0f);

            const float M_PI_F = 3.14159265f;
            const float base_radians = base_angle * (M_PI_F / 180.0f);
            const float rectangle_tangent = atan2f(height, width);
            const int quadrant = (int)fmodf(base_angle / 90.0f, 4.0f) + 1;

            const float axis_angle = [&]() -> float {
                switch (quadrant) {
                    default:
                    case 1: return base_radians - M_PI_F * 0.0f;
                    case 2: return M_PI_F * 1.0f - base_radians;
                    case 3: return base_radians - M_PI_F * 1.0f;
                    case 4: return M_PI_F * 2.0f - base_radians;
                }
            }();

            const float half_area = sqrtf(powf(width, 2.0f) + powf(height, 2.0f)) / 2.0f;
            const float cos_axis = cosf(fabsf(axis_angle - rectangle_tangent));

            return Gdiplus::PointF(
                left + (width / 2.0f) + (half_area * cos_axis * cosf(base_radians)),
                top + (height / 2.0f) + (half_area * cos_axis * sinf(base_radians))
            );
        };

       
        Gdiplus::PointF p1 = FindEdgePoint(m_GradientAngle + 180.0f, (float)bounds.X, (float)bounds.Y, (float)w, (float)h);
        Gdiplus::PointF p2 = FindEdgePoint(m_GradientAngle, (float)bounds.X, (float)bounds.Y, (float)w, (float)h);
        
        Gdiplus::Color color1(m_SolidAlpha, GetRValue(m_SolidColor), GetGValue(m_SolidColor), GetBValue(m_SolidColor));
        Gdiplus::Color color2(m_SolidAlpha2, GetRValue(m_SolidColor2), GetGValue(m_SolidColor2), GetBValue(m_SolidColor2));
        
        Gdiplus::LinearGradientBrush* lgBrush = new Gdiplus::LinearGradientBrush(p1, p2, color1, color2);
        lgBrush->SetWrapMode(Gdiplus::WrapModeClamp);
        brush = lgBrush;
    } else {
        Gdiplus::Color backColor(m_SolidAlpha, GetRValue(m_SolidColor), GetGValue(m_SolidColor), GetBValue(m_SolidColor));
        brush = new Gdiplus::SolidBrush(backColor);
    }
    
    if (m_CornerRadius > 0) {
        path = new Gdiplus::GraphicsPath();
        int d = m_CornerRadius * 2;
        if (d > w) d = w;
        if (d > h) d = h;

        path->AddArc(bounds.X, bounds.Y, d, d, 180, 90);
        path->AddArc(bounds.X + bounds.Width - d, bounds.Y, d, d, 270, 90);
        path->AddArc(bounds.X + bounds.Width - d, bounds.Y + bounds.Height - d, d, d, 0, 90);
        path->AddArc(bounds.X, bounds.Y + bounds.Height - d, d, d, 90, 90);
        path->CloseFigure();
        
        graphics.FillPath(brush, path);
        delete path;
    } else {
        graphics.FillRectangle(brush, (Gdiplus::REAL)bounds.X, (Gdiplus::REAL)bounds.Y, (Gdiplus::REAL)w, (Gdiplus::REAL)h);
    }
    
    delete brush;
}

/*
** Render the bevel of the element.
*/
void Element::RenderBevel(Gdiplus::Graphics& graphics) {
    if (m_BevelType == 0 || m_BevelWidth <= 0) return;

    Gdiplus::Rect bounds = GetBounds();
    int w = bounds.Width;
    int h = bounds.Height;
    
    Gdiplus::Color highlight(m_BevelAlpha1, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
    Gdiplus::Color shadow(m_BevelAlpha2, GetRValue(m_BevelColor2), GetGValue(m_BevelColor2), GetBValue(m_BevelColor2));
    
    Gdiplus::Pen highlightPen(highlight, (Gdiplus::REAL)m_BevelWidth);
    Gdiplus::Pen shadowPen(shadow, (Gdiplus::REAL)m_BevelWidth);
    
    int offset = m_BevelWidth / 2;
    
    switch (m_BevelType) {
    case 1: // Raised
        graphics.DrawLine(&highlightPen, bounds.X + offset, bounds.Y + offset, bounds.X + w - offset, bounds.Y + offset);
        graphics.DrawLine(&highlightPen, bounds.X + offset, bounds.Y + offset, bounds.X + offset, bounds.Y + h - offset);
        graphics.DrawLine(&shadowPen, bounds.X + w - offset, bounds.Y + offset, bounds.X + w - offset, bounds.Y + h - offset);
        graphics.DrawLine(&shadowPen, bounds.X + offset, bounds.Y + h - offset, bounds.X + w - offset, bounds.Y + h - offset);
        break;
        
    case 2: // Sunken
        graphics.DrawLine(&shadowPen, bounds.X + offset, bounds.Y + offset, bounds.X + w - offset, bounds.Y + offset);
        graphics.DrawLine(&shadowPen, bounds.X + offset, bounds.Y + offset, bounds.X + offset, bounds.Y + h - offset);
        graphics.DrawLine(&highlightPen, bounds.X + w - offset, bounds.Y + offset, bounds.X + w - offset, bounds.Y + h - offset);
        graphics.DrawLine(&highlightPen, bounds.X + offset, bounds.Y + h - offset, bounds.X + w - offset, bounds.Y + h - offset);
        break;
        
    case 3: // Emboss
        {
            Gdiplus::Color midHighlight(m_BevelAlpha1 / 2, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
            Gdiplus::Pen midPen(midHighlight, (Gdiplus::REAL)m_BevelWidth);
            graphics.DrawLine(&highlightPen, bounds.X + offset, bounds.Y + offset, bounds.X + w - offset, bounds.Y + offset);
            graphics.DrawLine(&midPen, bounds.X + offset, bounds.Y + offset, bounds.X + offset, bounds.Y + h - offset);
        }
        break;
        
    case 4: // Pillow
        for (int i = 0; i < m_BevelWidth; i++) {
            int alpha = (int)(m_BevelAlpha1 * (1.0f - (float)i / m_BevelWidth));
            Gdiplus::Color fadeColor(alpha, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
            Gdiplus::Pen fadePen(fadeColor, 1.0f);
            graphics.DrawRectangle(&fadePen, bounds.X + i, bounds.Y + i, w - i * 2, h - i * 2);
        }
        break;
    }
}
