/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Element.h"
#include "Logging.h"
#include "Direct2DHelper.h"
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
    int w = m_WDefined ? m_Width : GetAutoWidth();
    return w + m_PaddingLeft + m_PaddingRight;
}

/*
** Get the height of the element.
*/
int Element::GetHeight() { 
    int h = m_HDefined ? m_Height : GetAutoHeight();
    return h + m_PaddingTop + m_PaddingBottom;
}

/*
** Get the bounding box of the element.
*/
GfxRect Element::GetBounds() {
    return GfxRect(m_X, m_Y, GetWidth(), GetHeight());
}

/*
** Check if a point is within the element's bounds.
*/
bool Element::HitTest(int x, int y) {
    if (m_Rotate == 0.0f) {
        GfxRect bounds = GetBounds();
        return (x >= bounds.X && x < bounds.X + bounds.Width &&
                y >= bounds.Y && y < bounds.Y + bounds.Height);
    }

    // Transform point (x, y) by the inverse of the rotation transform
    GfxRect bounds = GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    D2D1::Matrix3x2F rotation = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
    D2D1::Matrix3x2F inverse;
    
    // If inversion fails (degenerate matrix), fallback to standard bounds
    if (!rotation.Invert()) {
        return (x >= bounds.X && x < bounds.X + bounds.Width &&
                y >= bounds.Y && y < bounds.Y + bounds.Height);
    }

    D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
    D2D1_POINT_2F transformed = rotation.TransformPoint(p); // Actually transforms by the matrix itself

    // Wait, D2D1::Matrix3x2F::TransformPoint applies the matrix. 
    // rotation.Invert() MODIFIES the matrix to be the inverse.
    // So 'rotation' IS now the inverse matrix.
    
    return (transformed.x >= bounds.X && transformed.x < bounds.X + bounds.Width &&
            transformed.y >= bounds.Y && transformed.y < bounds.Y + bounds.Height);
}

/*
** Check if the element has an action associated with it.
*/
bool Element::HasAction(UINT message, WPARAM wParam) const {
    switch (message)
    {
    case WM_LBUTTONUP:     return m_OnLeftMouseUpCallbackId != -1;
    case WM_LBUTTONDOWN:   return m_OnLeftMouseDownCallbackId != -1;
    case WM_LBUTTONDBLCLK: return m_OnLeftDoubleClickCallbackId != -1;
    case WM_RBUTTONUP:     return m_OnRightMouseUpCallbackId != -1;
    case WM_RBUTTONDOWN:   return m_OnRightMouseDownCallbackId != -1;
    case WM_RBUTTONDBLCLK: return m_OnRightDoubleClickCallbackId != -1;
    case WM_MBUTTONUP:     return m_OnMiddleMouseUpCallbackId != -1;
    case WM_MBUTTONDOWN:   return m_OnMiddleMouseDownCallbackId != -1;
    case WM_MBUTTONDBLCLK: return m_OnMiddleDoubleClickCallbackId != -1;
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1MouseUpCallbackId != -1;
        else return m_OnX2MouseUpCallbackId != -1;
    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1MouseDownCallbackId != -1;
        else return m_OnX2MouseDownCallbackId != -1;
    case WM_XBUTTONDBLCLK:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1DoubleClickCallbackId != -1;
        else return m_OnX2DoubleClickCallbackId != -1;
    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return m_OnScrollUpCallbackId != -1;
        else return m_OnScrollDownCallbackId != -1;
    case WM_MOUSEHWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return m_OnScrollRightCallbackId != -1;
        else return m_OnScrollLeftCallbackId != -1;
    case WM_MOUSEMOVE:
        return m_OnMouseOverCallbackId != -1 || m_OnMouseLeaveCallbackId != -1;
    }
    return false;
}

/*
** Check if the element has any interactive mouse action.
*/
bool Element::HasMouseAction() const {
    return m_OnLeftMouseUpCallbackId != -1 ||
           m_OnLeftMouseDownCallbackId != -1 ||
           m_OnLeftDoubleClickCallbackId != -1 ||
           m_OnRightMouseUpCallbackId != -1 ||
           m_OnRightMouseDownCallbackId != -1 ||
           m_OnRightDoubleClickCallbackId != -1 ||
           m_OnMiddleMouseUpCallbackId != -1 ||
           m_OnMiddleMouseDownCallbackId != -1 ||
           m_OnMiddleDoubleClickCallbackId != -1 ||
           m_OnX1MouseUpCallbackId != -1 ||
           m_OnX1MouseDownCallbackId != -1 ||
           m_OnX1DoubleClickCallbackId != -1 ||
           m_OnX2MouseUpCallbackId != -1 ||
           m_OnX2MouseDownCallbackId != -1 ||
           m_OnX2DoubleClickCallbackId != -1 ||
           m_OnScrollUpCallbackId != -1 ||
           m_OnScrollDownCallbackId != -1 ||
           m_OnScrollLeftCallbackId != -1 ||
           m_OnScrollRightCallbackId != -1 ||
           m_OnMouseOverCallbackId != -1 ||
           m_OnMouseLeaveCallbackId != -1;
}

/*
** Set the padding for the element.
*/
void Element::SetPadding(int left, int top, int right, int bottom) {
    // Logging::Log(LogLevel::Debug, L"Element SetPadding: [%d, %d, %d, %d]", left, top, right, bottom);
    m_PaddingLeft = left;
    m_PaddingTop = top;
    m_PaddingRight = right;
    m_PaddingBottom = bottom;
}

/*
** Render the background of the element.
*/
void Element::RenderBackground(ID2D1DeviceContext* context) {
    if (!m_HasSolidColor) return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    GfxRect bounds = GetBounds();
    D2D1_RECT_F rect = D2D1::RectF((float)bounds.X, (float)bounds.Y, (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height));
    
    if (rect.right <= rect.left || rect.bottom <= rect.top) return;

    Microsoft::WRL::ComPtr<ID2D1Brush> brush;
    if (m_HasGradient) {
        D2D1_POINT_2F p1 = Direct2D::FindEdgePoint(m_GradientAngle + 180.0f, rect);
        D2D1_POINT_2F p2 = Direct2D::FindEdgePoint(m_GradientAngle, rect);

        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> lgBrush;
        Direct2D::CreateLinearGradientBrush(context, p1, p2, m_SolidColor, m_SolidAlpha / 255.0f, m_SolidColor2, m_SolidAlpha2 / 255.0f, lgBrush.GetAddressOf());
        brush = lgBrush;
    } else {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> sBrush;
        Direct2D::CreateSolidBrush(context, m_SolidColor, m_SolidAlpha / 255.0f, sBrush.GetAddressOf());
        brush = sBrush;
    }

    if (brush) {
        if (m_CornerRadius > 0) {
            D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect, (float)m_CornerRadius, (float)m_CornerRadius);
            context->FillRoundedRectangle(roundedRect, brush.Get());
        } else {
            context->FillRectangle(rect, brush.Get());
        }
    }
}

/*
** Render the bevel of the element.
*/
void Element::RenderBevel(ID2D1DeviceContext* context) {
    if (m_BevelType == 0 || m_BevelWidth <= 0) return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    GfxRect bounds = GetBounds();
    D2D1_RECT_F rect = D2D1::RectF((float)bounds.X, (float)bounds.Y, (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height));
    
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> highlightBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> shadowBrush;
    Direct2D::CreateSolidBrush(context, m_BevelColor, m_BevelAlpha / 255.0f, highlightBrush.GetAddressOf());
    Direct2D::CreateSolidBrush(context, m_BevelColor2, m_BevelAlpha2 / 255.0f, shadowBrush.GetAddressOf());

    float offset = m_BevelWidth / 2.0f;
    
    switch (m_BevelType) {
    case 1: // Raised
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        break;
        
    case 2: // Sunken
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        break;
        
    case 3: // Emboss
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> midBrush;
            Direct2D::CreateSolidBrush(context, m_BevelColor, (m_BevelAlpha / 2.0f) / 255.0f, midBrush.GetAddressOf());
            context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), highlightBrush.Get(), (float)m_BevelWidth);
            context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), midBrush.Get(), (float)m_BevelWidth);
        }
        break;
        
    case 4: // Pillow
        for (int i = 0; i < m_BevelWidth; i++) {
            float alpha = (m_BevelAlpha / 255.0f) * (1.0f - (float)i / m_BevelWidth);
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fadeBrush;
            Direct2D::CreateSolidBrush(context, m_BevelColor, alpha, fadeBrush.GetAddressOf());
            D2D1_RECT_F r = D2D1::RectF(rect.left + i, rect.top + i, rect.right - i, rect.bottom - i);
            context->DrawRectangle(r, fadeBrush.Get(), 1.0f);
        }
        break;
    }
}
