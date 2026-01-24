/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_TEXT_ELEMENT_H__
#define __NOVADESK_TEXT_ELEMENT_H__

#include "Element.h"
#include <string>
#include <windows.h>

enum TextAlignment
{
    TEXT_ALIGN_LEFT_TOP,
    TEXT_ALIGN_CENTER_TOP,
    TEXT_ALIGN_RIGHT_TOP,
    TEXT_ALIGN_LEFT_CENTER,
    TEXT_ALIGN_CENTER_CENTER,
    TEXT_ALIGN_RIGHT_CENTER,
    TEXT_ALIGN_LEFT_BOTTOM,
    TEXT_ALIGN_CENTER_BOTTOM,
    TEXT_ALIGN_RIGHT_BOTTOM
};

enum TextClipString
{
    TEXT_CLIP_NONE = 0,
    TEXT_CLIP_ON = 1,
    TEXT_CLIP_ELLIPSIS = 2,
    TEXT_CLIP_WRAP = 3
};

class TextElement : public Element
{
public:
    TextElement(const std::wstring& id, int x, int y, int w, int h,
          const std::wstring& text, const std::wstring& fontFace,
          int fontSize, COLORREF fontColor, BYTE alpha,
          bool bold, bool italic, TextAlignment textAlign,
          TextClipString clip = TEXT_CLIP_NONE);

    virtual ~TextElement() {}

    virtual void Render(ID2D1DeviceContext* context) override;

    void SetText(const std::wstring& text) { m_Text = text; }
    void SetFontFace(const std::wstring& font) { m_FontFace = font; }
    void SetFontSize(int size) { m_FontSize = size; }
    void SetFontColor(COLORREF color, BYTE alpha) { m_FontColor = color; m_Alpha = alpha; }
    void SetBold(bool bold) { m_Bold = bold; }
    void SetItalic(bool italic) { m_Italic = italic; }
    void SetTextAlign(TextAlignment align) { m_TextAlign = align; }
    void SetClip(TextClipString clip) { m_ClipString = clip; }

    const std::wstring& GetText() const { return m_Text; }
    const std::wstring& GetFontFace() const { return m_FontFace; }
    int GetFontSize() const { return m_FontSize; }
    COLORREF GetFontColor() const { return m_FontColor; }
    BYTE GetFontAlpha() const { return m_Alpha; }
    bool IsBold() const { return m_Bold; }
    bool IsItalic() const { return m_Italic; }
    TextAlignment GetTextAlign() const { return m_TextAlign; }
    TextClipString GetClipString() const { return m_ClipString; }

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual GfxRect GetBounds() override; // Keeping ROI as GfxRect for now as it's used for layout, but internally use D2D
    virtual bool HitTest(int x, int y) override;

private:
    std::wstring m_Text;
    std::wstring m_FontFace;
    int m_FontSize;
    COLORREF m_FontColor;
    BYTE m_Alpha;
    bool m_Bold;
    bool m_Italic;
    TextAlignment m_TextAlign;
    TextClipString m_ClipString;
};

#endif
