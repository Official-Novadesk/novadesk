/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Text.h"

using namespace Gdiplus;

Text::Text(const std::wstring& id, int x, int y, int w, int h,
     const std::wstring& text, const std::wstring& fontFamily,
     int fontSize, COLORREF color, BYTE alpha,
     bool bold, bool italic, Alignment align,
     ClipString clip, int clipW, int clipH)
    : Element(ELEMENT_TEXT, id, x, y, w, h),
      m_Text(text), m_FontFamily(fontFamily), m_FontSize(fontSize),
      m_Color(color), m_Alpha(alpha), m_Bold(bold), m_Italic(italic),
      m_Align(align), m_ClipString(clip), m_ClipStringW(clipW), m_ClipStringH(clipH)
{
}

void Text::Render(Graphics& graphics)
{
    // Create font
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    
    Font font(m_FontFamily.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);
    
    // Create brush with color and alpha
    Color textColor(m_Alpha, GetRValue(m_Color), GetGValue(m_Color), GetBValue(m_Color));
    SolidBrush brush(textColor);
    
    // Set up string format for alignment
    StringFormat format;
    switch (m_Align)
    {
    case ALIGN_LEFT_TOP:
    case ALIGN_LEFT_CENTER:
    case ALIGN_LEFT_BOTTOM:
        format.SetAlignment(StringAlignmentNear);
        break;
    case ALIGN_CENTER_TOP:
    case ALIGN_CENTER_CENTER:
    case ALIGN_CENTER_BOTTOM:
        format.SetAlignment(StringAlignmentCenter);
        break;
    case ALIGN_RIGHT_TOP:
    case ALIGN_RIGHT_CENTER:
    case ALIGN_RIGHT_BOTTOM:
        format.SetAlignment(StringAlignmentFar);
        break;
    }

    switch (m_Align)
    {
    case ALIGN_LEFT_TOP:
    case ALIGN_CENTER_TOP:
    case ALIGN_RIGHT_TOP:
        format.SetLineAlignment(StringAlignmentNear);
        break;
    case ALIGN_LEFT_CENTER:
    case ALIGN_CENTER_CENTER:
    case ALIGN_RIGHT_CENTER:
        format.SetLineAlignment(StringAlignmentCenter);
        break;
    case ALIGN_LEFT_BOTTOM:
    case ALIGN_CENTER_BOTTOM:
    case ALIGN_RIGHT_BOTTOM:
        format.SetLineAlignment(StringAlignmentFar);
        break;
    }

    // Set trimming/clipping
    if (m_ClipString == CLIP_ELLIPSIS)
    {
        format.SetTrimming(StringTrimmingEllipsisCharacter);
    }
    else if (m_ClipString == CLIP_ON)
    {
        format.SetTrimming(StringTrimmingCharacter);
    }
    
    // Draw text
    RectF layoutRect((REAL)m_X, (REAL)m_Y, (REAL)GetWidth(), (REAL)GetHeight());
    graphics.DrawString(m_Text.c_str(), -1, &font, layoutRect, &format, &brush);
}

int Text::GetAutoWidth()
{
    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFamily.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, PointF(0, 0), &boundingBox);
    
    ReleaseDC(NULL, hdc);
    
    int width = (int)ceil(boundingBox.Width);
    if (!m_WDefined && m_ClipString != CLIP_NONE && m_ClipStringW != -1)
    {
        if (width > m_ClipStringW) return m_ClipStringW;
    }
    return width;
}

int Text::GetAutoHeight()
{
    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFamily.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, PointF(0, 0), &boundingBox);
    
    ReleaseDC(NULL, hdc);
    
    int height = (int)ceil(boundingBox.Height);
    if (!m_HDefined && m_ClipString != CLIP_NONE && m_ClipStringH != -1)
    {
        if (height > m_ClipStringH) return m_ClipStringH;
    }
    return height;
}

bool Text::HitTest(int x, int y)
{
    // Bounding box check first (Element's layout rect)
    if (!Element::HitTest(x, y)) return false;

    // Redundant but keeping it for now if we want tighter bounds
    // Better: use the bounding box from MeasureString
    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFamily.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    StringFormat format;
    // (Same alignment logic as in Render)
    switch (m_Align) {
    case ALIGN_LEFT_TOP: case ALIGN_LEFT_CENTER: case ALIGN_LEFT_BOTTOM:
        format.SetAlignment(StringAlignmentNear); break;
    case ALIGN_CENTER_TOP: case ALIGN_CENTER_CENTER: case ALIGN_CENTER_BOTTOM:
        format.SetAlignment(StringAlignmentCenter); break;
    case ALIGN_RIGHT_TOP: case ALIGN_RIGHT_CENTER: case ALIGN_RIGHT_BOTTOM:
        format.SetAlignment(StringAlignmentFar); break;
    }
    switch (m_Align) {
    case ALIGN_LEFT_TOP: case ALIGN_CENTER_TOP: case ALIGN_RIGHT_TOP:
        format.SetLineAlignment(StringAlignmentNear); break;
    case ALIGN_LEFT_CENTER: case ALIGN_CENTER_CENTER: case ALIGN_RIGHT_CENTER:
        format.SetLineAlignment(StringAlignmentCenter); break;
    case ALIGN_LEFT_BOTTOM: case ALIGN_CENTER_BOTTOM: case ALIGN_RIGHT_BOTTOM:
        format.SetLineAlignment(StringAlignmentFar); break;
    }

    RectF layoutRect(0, 0, (REAL)GetWidth(), (REAL)GetHeight());
    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, layoutRect, &format, &boundingBox);
    
    ReleaseDC(NULL, hdc);

    boundingBox.X += m_X;
    boundingBox.Y += m_Y;

    return (x >= boundingBox.X && x < boundingBox.X + boundingBox.Width &&
            y >= boundingBox.Y && y < boundingBox.Y + boundingBox.Height);
}
