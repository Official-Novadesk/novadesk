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
#include <vector>

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

enum ElementType
{
    ELEMENT_IMAGE,
    ELEMENT_TEXT,
    ELEMENT_BAR
};

class Element
{
public:
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height);
    virtual ~Element() {}

    virtual void Render(Gdiplus::Graphics& graphics) = 0;

    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    
    int GetWidth();
    int GetHeight();

    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { 
        m_Width = w; 
        m_Height = h; 
        m_WDefined = (w > 0);
        m_HDefined = (h > 0);
    }

    virtual int GetAutoWidth() { return 0; }
    virtual int GetAutoHeight() { return 0; }

    virtual bool HitTest(int x, int y);

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

    void SetAntiAlias(bool enable) { m_AntiAlias = enable; }
    
    void SetPadding(int left, int top, int right, int bottom);

    void SetRotate(float angle) { m_Rotate = angle; }
    float GetRotate() const { return m_Rotate; }

    bool HasSolidColor() const { return m_HasSolidColor; }
    COLORREF GetSolidColor() const { return m_SolidColor; }
    BYTE GetSolidAlpha() const { return m_SolidAlpha; }
    int GetCornerRadius() const { return m_CornerRadius; }

    bool HasGradient() const { return m_HasGradient; }
    COLORREF GetSolidColor2() const { return m_SolidColor2; }
    BYTE GetSolidAlpha2() const { return m_SolidAlpha2; }
    float GetGradientAngle() const { return m_GradientAngle; }

    int GetBevelType() const { return m_BevelType; }
    int GetBevelWidth() const { return m_BevelWidth; }
    COLORREF GetBevelColor1() const { return m_BevelColor1; }
    BYTE GetBevelAlpha1() const { return m_BevelAlpha1; }
    COLORREF GetBevelColor2() const { return m_BevelColor2; }
    BYTE GetBevelAlpha2() const { return m_BevelAlpha2; }

    int GetPaddingLeft() const { return m_PaddingLeft; }
    int GetPaddingTop() const { return m_PaddingTop; }
    int GetPaddingRight() const { return m_PaddingRight; }
    int GetPaddingBottom() const { return m_PaddingBottom; }

    bool GetAntiAlias() const { return m_AntiAlias; }

    virtual bool IsTransparentHit() const { return false; }

    bool HasAction(UINT message, WPARAM wParam) const;
    bool HasMouseAction() const;

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
    int m_BevelType = 0;
    int m_BevelWidth = 0;
    COLORREF m_BevelColor1 = RGB(255, 255, 255);
    BYTE m_BevelAlpha1 = 200;
    COLORREF m_BevelColor2 = RGB(0, 0, 0);
    BYTE m_BevelAlpha2 = 150;

    // Rendering properties
    bool m_AntiAlias = true;
    
    // Padding properties
    int m_PaddingLeft = 0;
    int m_PaddingTop = 0;
    int m_PaddingRight = 0;
    int m_PaddingBottom = 0;

    // Transformation properties
    float m_Rotate = 0.0f;

    void RenderBackground(Gdiplus::Graphics& graphics);
    void RenderBevel(Gdiplus::Graphics& graphics);
};

#endif
