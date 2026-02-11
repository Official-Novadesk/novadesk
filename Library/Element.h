/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <d2d1_1.h>
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
    ELEMENT_BAR,
    ELEMENT_ROUNDLINE,
    ELEMENT_SHAPE
};

struct GfxRect {
    int X, Y, Width, Height;
    GfxRect() : X(0), Y(0), Width(0), Height(0) {}
    GfxRect(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
};

struct TextShadow {
    float offsetX = 0;
    float offsetY = 0;
    float blur = 0;
    COLORREF color = 0;
    BYTE alpha = 255;
};

struct GradientStop {
    COLORREF color;
    BYTE alpha;
    float position;
};

enum GradientType {
    GRADIENT_NONE,
    GRADIENT_LINEAR,
    GRADIENT_RADIAL
};

struct GradientInfo {
    GradientType type = GRADIENT_NONE;
    std::vector<GradientStop> stops;
    float angle = 0.0f; // For linear
    std::wstring shape = L"circle"; // For radial
};

enum TextCase {
    TEXT_CASE_NORMAL,
    TEXT_CASE_UPPER,
    TEXT_CASE_LOWER,
    TEXT_CASE_CAPITALIZE,
    TEXT_CASE_SENTENCE
};

class Element
{
public:
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height);
    virtual ~Element() {}

    virtual void Render(ID2D1DeviceContext* context) = 0;

    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    
    int GetWidth();
    int GetHeight();

    bool IsWDefined() const { return m_WDefined; }
    bool IsHDefined() const { return m_HDefined; }

    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { 
        m_Width = w; 
        m_Height = h; 
        m_WDefined = (w > 0);
        m_HDefined = (h > 0);
    }

    virtual int GetAutoWidth() { return 0; }
    virtual int GetAutoHeight() { return 0; }

    virtual GfxRect GetBounds();
    virtual GfxRect GetBackgroundBounds();

    virtual bool HitTest(int x, int y);

    void SetSolidColor(COLORREF color, BYTE alpha) { 
        m_SolidColor = color; 
        m_SolidAlpha = alpha; 
        m_HasSolidColor = true; 
    }

    void SetSolidGradient(const GradientInfo& gradient) {
        m_SolidGradient = gradient;
        m_HasSolidColor = true;
    }

    void SetCornerRadius(int radius) { 
        m_CornerRadius = radius; 
    }

    void SetBevel(int type, int width, COLORREF color, BYTE alpha, COLORREF color2, BYTE alpha2) {
        m_BevelType = type;
        m_BevelWidth = width;
        m_BevelColor = color;
        m_BevelAlpha = alpha;
        m_BevelColor2 = color2;
        m_BevelAlpha2 = alpha2;
    }

    void SetAntiAlias(bool enable) { m_AntiAlias = enable; }
    
    void SetPadding(int left, int top, int right, int bottom);

    void SetRotate(float angle) { m_Rotate = angle; }
    float GetRotate() const { return m_Rotate; }

    void SetTransformMatrix(const float* matrix) {
        if (matrix) {
            memcpy(m_TransformMatrix, matrix, sizeof(float) * 6);
            m_HasTransformMatrix = true;
        } else {
            m_HasTransformMatrix = false;
        }
    }
    bool HasTransformMatrix() const { return m_HasTransformMatrix; }
    const float* GetTransformMatrix() const { return m_TransformMatrix; }

    bool HasSolidColor() const { return m_HasSolidColor; }
    COLORREF GetSolidColor() const { return m_SolidColor; }
    BYTE GetSolidAlpha() const { return m_SolidAlpha; }
    int GetCornerRadius() const { return m_CornerRadius; }

    const GradientInfo& GetSolidGradient() const { return m_SolidGradient; }

    int GetBevelType() const { return m_BevelType; }
    int GetBevelWidth() const { return m_BevelWidth; }
    COLORREF GetBevelColor() const { return m_BevelColor; }
    BYTE GetBevelAlpha() const { return m_BevelAlpha; }
    COLORREF GetBevelColor2() const { return m_BevelColor2; }
    BYTE GetBevelAlpha2() const { return m_BevelAlpha2; }

    int GetPaddingLeft() const { return m_PaddingLeft; }
    int GetPaddingTop() const { return m_PaddingTop; }
    int GetPaddingRight() const { return m_PaddingRight; }
    int GetPaddingBottom() const { return m_PaddingBottom; }

    bool GetAntiAlias() const { return m_AntiAlias; }

    void SetShow(bool show) { m_Show = show; }
    bool IsVisible() const { return m_Show; }

    void SetContainerId(const std::wstring& id) { m_ContainerId = id; }
    const std::wstring& GetContainerId() const { return m_ContainerId; }
    void SetContainer(Element* container) { m_ContainerElement = container; }
    Element* GetContainer() const { return m_ContainerElement; }
    bool IsContained() const { return m_ContainerElement != nullptr; }

    void AddContainerItem(Element* item) { m_ContainerItems.push_back(item); }
    void RemoveContainerItem(Element* item);
    void ClearContainerItems();
    const std::vector<Element*>& GetContainerItems() const { return m_ContainerItems; }
    bool IsContainer() const { return !m_ContainerItems.empty(); }

    virtual bool IsTransparentHit() const { return false; }

    bool HasAction(UINT message, WPARAM wParam) const;
    bool HasMouseAction() const;

    // Tooltip properties
    void SetToolTip(const std::wstring& text, const std::wstring& title = L"", const std::wstring& icon = L"", int maxWidth = 0, int maxHeight = 0, bool balloon = false) {
        m_ToolTipText = text;
        m_ToolTipTitle = title;
        m_ToolTipIcon = icon;
        m_ToolTipMaxWidth = maxWidth;
        m_ToolTipMaxHeight = maxHeight;
        m_ToolTipBalloon = balloon;
    }

    const std::wstring& GetToolTipText() const { return m_ToolTipText; }
    const std::wstring& GetToolTipTitle() const { return m_ToolTipTitle; }
    const std::wstring& GetToolTipIcon() const { return m_ToolTipIcon; }
    int GetToolTipMaxWidth() const { return m_ToolTipMaxWidth; }
    int GetToolTipMaxHeight() const { return m_ToolTipMaxHeight; }
    bool GetToolTipBalloon() const { return m_ToolTipBalloon; }

    bool HasToolTip() const { return !m_ToolTipText.empty(); }

    // Mouse Actions
    
    // Callback IDs (initialized to -1)
    int m_OnLeftMouseUpCallbackId = -1;
    int m_OnLeftMouseDownCallbackId = -1;
    int m_OnLeftDoubleClickCallbackId = -1;
    int m_OnRightMouseUpCallbackId = -1;
    int m_OnRightMouseDownCallbackId = -1;
    int m_OnRightDoubleClickCallbackId = -1;
    int m_OnMiddleMouseUpCallbackId = -1;
    int m_OnMiddleMouseDownCallbackId = -1;
    int m_OnMiddleDoubleClickCallbackId = -1;
    int m_OnX1MouseUpCallbackId = -1;
    int m_OnX1MouseDownCallbackId = -1;
    int m_OnX1DoubleClickCallbackId = -1;
    int m_OnX2MouseUpCallbackId = -1;
    int m_OnX2MouseDownCallbackId = -1;
    int m_OnX2DoubleClickCallbackId = -1;
    int m_OnScrollUpCallbackId = -1;
    int m_OnScrollDownCallbackId = -1;
    int m_OnScrollLeftCallbackId = -1;
    int m_OnScrollRightCallbackId = -1;
    int m_OnMouseOverCallbackId = -1;
    int m_OnMouseLeaveCallbackId = -1;

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
    GradientInfo m_SolidGradient;

    // Bevel properties
    int m_BevelType = 0;
    int m_BevelWidth = 0;
    COLORREF m_BevelColor = RGB(255, 255, 255);
    BYTE m_BevelAlpha = 200;
    COLORREF m_BevelColor2 = RGB(0, 0, 0);
    BYTE m_BevelAlpha2 = 150;

    // Rendering properties
    bool m_AntiAlias = true;
    bool m_Show = true;

    std::wstring m_ContainerId;
    Element* m_ContainerElement = nullptr;
    std::vector<Element*> m_ContainerItems;
    
    // Padding properties
    int m_PaddingLeft = 0;
    int m_PaddingTop = 0;
    int m_PaddingRight = 0;
    int m_PaddingBottom = 0;

    // Transformation properties
    float m_Rotate = 0.0f;
    bool m_HasTransformMatrix = false;
    float m_TransformMatrix[6];

    // Tooltip properties
    std::wstring m_ToolTipText;
    std::wstring m_ToolTipTitle;
    std::wstring m_ToolTipIcon;
    int m_ToolTipMaxWidth = 0;
    int m_ToolTipMaxHeight = 0;
    bool m_ToolTipBalloon = false;

    void RenderBackground(ID2D1DeviceContext* context);
    void RenderBevel(ID2D1DeviceContext* context);
    void ApplyRenderTransform(ID2D1DeviceContext* context, D2D1_MATRIX_3X2_F& originalTransform);
    void RestoreRenderTransform(ID2D1DeviceContext* context, const D2D1_MATRIX_3X2_F& originalTransform);
};

#endif
