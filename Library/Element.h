/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <cmath>

// Helper macros for color extraction from COLORREF (0x00BBGGRR)
#ifndef GetRValue
#define GetRValue(rgb)      (LOBYTE(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb)      (LOBYTE((WORD)(rgb) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb)      (LOBYTE((rgb) >> 16))
#endif

/*
** Content type enumeration.
*/
enum ElementType
{
    ELEMENT_IMAGE,
    ELEMENT_TEXT
};

/*
** Base class for all widget elements (Text, Image, etc.)
*/
class Element
{
public:
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height)
        : m_Type(type), m_Id(id), m_X(x), m_Y(y)
    {
        m_Width = (width > 0) ? width : 0;
        m_Height = (height > 0) ? height : 0;
        m_WDefined = (width > 0);
        m_HDefined = (height > 0);
    }

    virtual ~Element() {}

    /*
    ** Render the element to the GDI+ Graphics context.
    */
    virtual void Render(Gdiplus::Graphics& graphics) = 0;

    /*
    ** Getters
    */
    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    
    int GetWidth() { 
        if (m_WDefined) return m_Width;
        return GetAutoWidth();
    }
    
    int GetHeight() { 
        if (m_HDefined) return m_Height;
        return GetAutoHeight();
    }

    /*
    ** Setters
    */
    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { 
        m_Width = w; 
        m_Height = h; 
        m_WDefined = (w > 0);
        m_HDefined = (h > 0);
    }

    /*
    ** Auto-size calculation (to be overriden by subclasses)
    */
    virtual int GetAutoWidth() { return 0; }
    virtual int GetAutoHeight() { return 0; }

    /*
    ** Check if a point is within the element's bounds.
    */
    virtual bool HitTest(int x, int y) {
        int w = GetWidth();
        int h = GetHeight();
        return (x >= m_X && x < m_X + w && y >= m_Y && y < m_Y + h);
    }

    void SetSolidColor(COLORREF color, BYTE alpha) { 
        m_SolidColor = color; 
        m_SolidAlpha = alpha; 
        m_HasSolidColor = true; 
    }

    void SetGradient(COLORREF color2, BYTE alpha2, float angle) {
        m_SolidColor2 = color2;
        m_SolidAlpha2 = alpha2;
        m_GradientAngle = angle;
        m_HasGradient = true;
    }

    void SetCornerRadius(int radius) { 
        m_CornerRadius = radius; 
    }

    void SetBevel(int type, int width, COLORREF color1, BYTE alpha1, COLORREF color2, BYTE alpha2) {
        m_BevelType = type;
        m_BevelWidth = width;
        m_BevelColor1 = color1;
        m_BevelAlpha1 = alpha1;
        m_BevelColor2 = color2;
        m_BevelAlpha2 = alpha2;
    }

    /*
    ** Check if this element should be hit even if it's transparent.
    ** (e.g. for SolidColor in Rainmeter)
    */
    virtual bool IsTransparentHit() const { return false; }

    /*
    ** Check if the element has an action associated with a specific mouse message.
    */
    bool HasAction(UINT message, WPARAM wParam) const {
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

    // Mouse Actions
    std::wstring m_OnLeftMouseUp;
    std::wstring m_OnLeftMouseDown;
    std::wstring m_OnLeftDoubleClick;
    std::wstring m_OnRightMouseUp;
    std::wstring m_OnRightMouseDown;
    std::wstring m_OnRightDoubleClick;
    std::wstring m_OnMiddleMouseUp;
    std::wstring m_OnMiddleMouseDown;
    std::wstring m_OnMiddleDoubleClick;
    std::wstring m_OnX1MouseUp;
    std::wstring m_OnX1MouseDown;
    std::wstring m_OnX1DoubleClick;
    std::wstring m_OnX2MouseUp;
    std::wstring m_OnX2MouseDown;
    std::wstring m_OnX2DoubleClick;
    std::wstring m_OnScrollUp;
    std::wstring m_OnScrollDown;
    std::wstring m_OnScrollLeft;
    std::wstring m_OnScrollRight;
    std::wstring m_OnMouseOver;
    std::wstring m_OnMouseLeave;

    bool m_IsMouseOver = false;

protected:
    ElementType m_Type;
    std::wstring m_Id;
    int m_X, m_Y;
    int m_Width, m_Height;
    bool m_WDefined, m_HDefined;
    
    // Background properties
    bool m_HasSolidColor = false;
    COLORREF m_SolidColor = 0;
    BYTE m_SolidAlpha = 0;
    int m_CornerRadius = 0;

    // Gradient properties
    bool m_HasGradient = false;
    COLORREF m_SolidColor2 = 0;
    BYTE m_SolidAlpha2 = 0;
    float m_GradientAngle = 0.0f;

    // Bevel properties
    int m_BevelType = 0;  // 0=None, 1=Raised, 2=Sunken, 3=Emboss, 4=Pillow
    int m_BevelWidth = 0;
    COLORREF m_BevelColor1 = RGB(255, 255, 255);  // Highlight
    BYTE m_BevelAlpha1 = 200;
    COLORREF m_BevelColor2 = RGB(0, 0, 0);  // Shadow
    BYTE m_BevelAlpha2 = 150;

    void RenderBackground(Gdiplus::Graphics& graphics) {
        if (!m_HasSolidColor) return;

        int w = GetWidth();
        int h = GetHeight();
        
        Gdiplus::Brush* brush = nullptr;
        Gdiplus::GraphicsPath* path = nullptr;

        // Create gradient or solid brush
        if (m_HasGradient) {
            // Calculate gradient direction from angle
            float radians = m_GradientAngle * 3.14159265f / 180.0f;
            float dx = std::cos(radians) * w;
            float dy = std::sin(radians) * h;
            
            Gdiplus::PointF p1((Gdiplus::REAL)m_X, (Gdiplus::REAL)m_Y);
            Gdiplus::PointF p2((Gdiplus::REAL)(m_X + dx), (Gdiplus::REAL)(m_Y + dy));
            
            Gdiplus::Color color1(m_SolidAlpha, GetRValue(m_SolidColor), GetGValue(m_SolidColor), GetBValue(m_SolidColor));
            Gdiplus::Color color2(m_SolidAlpha2, GetRValue(m_SolidColor2), GetGValue(m_SolidColor2), GetBValue(m_SolidColor2));
            
            brush = new Gdiplus::LinearGradientBrush(p1, p2, color1, color2);
        } else {
            Gdiplus::Color backColor(m_SolidAlpha, GetRValue(m_SolidColor), GetGValue(m_SolidColor), GetBValue(m_SolidColor));
            brush = new Gdiplus::SolidBrush(backColor);
        }
        
        if (m_CornerRadius > 0) {
            // Draw rounded rectangle
            path = new Gdiplus::GraphicsPath();
            int d = m_CornerRadius * 2;
            
            // Clamp radius to half width/height
            if (d > w) d = w;
            if (d > h) d = h;

            Gdiplus::Rect r(m_X, m_Y, w, h);
            path->AddArc(r.X, r.Y, d, d, 180, 90);
            path->AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
            path->AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
            path->AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
            path->CloseFigure();
            
            graphics.FillPath(brush, path);
            delete path;
        } else {
            graphics.FillRectangle(brush, m_X, m_Y, w, h);
        }
        
        delete brush;
    }

    void RenderBevel(Gdiplus::Graphics& graphics) {
        if (m_BevelType == 0 || m_BevelWidth <= 0) return;

        int w = GetWidth();
        int h = GetHeight();
        
        Gdiplus::Color highlight(m_BevelAlpha1, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
        Gdiplus::Color shadow(m_BevelAlpha2, GetRValue(m_BevelColor2), GetGValue(m_BevelColor2), GetBValue(m_BevelColor2));
        
        Gdiplus::Pen highlightPen(highlight, (Gdiplus::REAL)m_BevelWidth);
        Gdiplus::Pen shadowPen(shadow, (Gdiplus::REAL)m_BevelWidth);
        
        int offset = m_BevelWidth / 2;
        
        switch (m_BevelType) {
        case 1: // Raised
            // Light on top-left, dark on bottom-right
            graphics.DrawLine(&highlightPen, m_X + offset, m_Y + offset, m_X + w - offset, m_Y + offset); // Top
            graphics.DrawLine(&highlightPen, m_X + offset, m_Y + offset, m_X + offset, m_Y + h - offset); // Left
            graphics.DrawLine(&shadowPen, m_X + w - offset, m_Y + offset, m_X + w - offset, m_Y + h - offset); // Right
            graphics.DrawLine(&shadowPen, m_X + offset, m_Y + h - offset, m_X + w - offset, m_Y + h - offset); // Bottom
            break;
            
        case 2: // Sunken
            // Dark on top-left, light on bottom-right
            graphics.DrawLine(&shadowPen, m_X + offset, m_Y + offset, m_X + w - offset, m_Y + offset); // Top
            graphics.DrawLine(&shadowPen, m_X + offset, m_Y + offset, m_X + offset, m_Y + h - offset); // Left
            graphics.DrawLine(&highlightPen, m_X + w - offset, m_Y + offset, m_X + w - offset, m_Y + h - offset); // Right
            graphics.DrawLine(&highlightPen, m_X + offset, m_Y + h - offset, m_X + w - offset, m_Y + h - offset); // Bottom
            break;
            
        case 3: // Emboss
            // Subtle 3D effect with both highlights
            {
                Gdiplus::Color midHighlight(m_BevelAlpha1 / 2, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
                Gdiplus::Pen midPen(midHighlight, (Gdiplus::REAL)m_BevelWidth);
                graphics.DrawLine(&highlightPen, m_X + offset, m_Y + offset, m_X + w - offset, m_Y + offset);
                graphics.DrawLine(&midPen, m_X + offset, m_Y + offset, m_X + offset, m_Y + h - offset);
            }
            break;
            
        case 4: // Pillow (rounded cushion effect)
            // Draw gradient from edges to center
            for (int i = 0; i < m_BevelWidth; i++) {
                int alpha = (int)(m_BevelAlpha1 * (1.0f - (float)i / m_BevelWidth));
                Gdiplus::Color fadeColor(alpha, GetRValue(m_BevelColor1), GetGValue(m_BevelColor1), GetBValue(m_BevelColor1));
                Gdiplus::Pen fadePen(fadeColor, 1.0f);
                graphics.DrawRectangle(&fadePen, m_X + i, m_Y + i, w - i * 2, h - i * 2);
            }
            break;
        }
    }
};

#endif
