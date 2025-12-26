/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_TEXT_H__
#define __NOVADESK_TEXT_H__

#include "Element.h"
#include <string>
#include <windows.h>
#include <gdiplus.h>

enum Alignment
{
    ALIGN_LEFT_TOP,
    ALIGN_CENTER_TOP,
    ALIGN_RIGHT_TOP,
    ALIGN_LEFT_CENTER,
    ALIGN_CENTER_CENTER,
    ALIGN_RIGHT_CENTER,
    ALIGN_LEFT_BOTTOM,
    ALIGN_CENTER_BOTTOM,
    ALIGN_RIGHT_BOTTOM
};

enum ClipString
{
    CLIP_NONE = 0,
    CLIP_ON = 1,
    CLIP_ELLIPSIS = 2
};

class Text : public Element
{
public:
    Text(const std::wstring& id, int x, int y, int w, int h,
         const std::wstring& text, const std::wstring& fontFamily,
         int fontSize, COLORREF color, BYTE alpha,
         bool bold, bool italic, Alignment align,
         ClipString clip = CLIP_NONE, int clipW = -1, int clipH = -1);

    virtual ~Text() {}

    virtual void Render(Gdiplus::Graphics& graphics) override;

    void SetText(const std::wstring& text) { m_Text = text; }
    void SetFontFace(const std::wstring& font) { m_FontFace = font; }
    void SetFontSize(int size) { m_FontSize = size; }
    void SetFontColor(COLORREF color, BYTE alpha) { m_FontColor = color; m_Alpha = alpha; }

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual bool HitTest(int x, int y) override;

private:
    std::wstring m_Text;
    std::wstring m_FontFace;
    int m_FontSize;
    COLORREF m_FontColor;
    BYTE m_Alpha;
    bool m_Bold;
    bool m_Italic;
    Alignment m_TextAlign;
    ClipString m_ClipString;
    int m_ClipStringW;
    int m_ClipStringH;
};

#endif
