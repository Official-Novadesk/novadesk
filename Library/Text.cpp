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
     bool bold, bool italic, TextAlign align, VerticalAlign vAlign, float lineHeight)
    : Element(ELEMENT_TEXT, id, x, y, w, h),
      m_Text(text), m_FontFamily(fontFamily), m_FontSize(fontSize),
      m_Color(color), m_Alpha(alpha), m_Bold(bold), m_Italic(italic),
      m_Align(align), m_VerticalAlign(vAlign), m_LineHeight(lineHeight)
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
    case ALIGN_LEFT:   format.SetAlignment(StringAlignmentNear); break;
    case ALIGN_CENTER: format.SetAlignment(StringAlignmentCenter); break;
    case ALIGN_RIGHT:  format.SetAlignment(StringAlignmentFar); break;
    }
    
    switch (m_VerticalAlign)
    {
    case VALIGN_TOP:    format.SetLineAlignment(StringAlignmentNear); break;
    case VALIGN_MIDDLE: format.SetLineAlignment(StringAlignmentCenter); break;
    case VALIGN_BOTTOM: format.SetLineAlignment(StringAlignmentFar); break;
    }
    
    // Draw text
    RectF layoutRect((REAL)m_X, (REAL)m_Y, (REAL)m_Width, (REAL)m_Height);
    graphics.DrawString(m_Text.c_str(), -1, &font, layoutRect, &format, &brush);
}
