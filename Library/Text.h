/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_TEXT_H__
#define __NOVADESK_TEXT_H__

#include "Element.h"

enum TextAlign
{
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

enum VerticalAlign
{
    VALIGN_TOP,
    VALIGN_MIDDLE,
    VALIGN_BOTTOM
};

class Text : public Element
{
public:
    Text(const std::wstring& id, int x, int y, int w, int h,
         const std::wstring& text, const std::wstring& fontFamily,
         int fontSize, COLORREF color, BYTE alpha,
         bool bold, bool italic, TextAlign align, VerticalAlign vAlign, float lineHeight);

    virtual ~Text() {}

    virtual void Render(Gdiplus::Graphics& graphics) override;

    void SetText(const std::wstring& text) { m_Text = text; }
    void SetFontFamily(const std::wstring& font) { m_FontFamily = font; }
    void SetFontSize(int size) { m_FontSize = size; }
    void SetColor(COLORREF color, BYTE alpha) { m_Color = color; m_Alpha = alpha; }

private:
    std::wstring m_Text;
    std::wstring m_FontFamily;
    int m_FontSize;
    COLORREF m_Color;
    BYTE m_Alpha;
    bool m_Bold;
    bool m_Italic;
    TextAlign m_Align;
    VerticalAlign m_VerticalAlign;
    float m_LineHeight;
};

#endif
