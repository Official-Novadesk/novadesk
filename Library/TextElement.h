/* Copyright (C) 2026 OfficialNovadesk 
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
#include <vector>
#include <optional>

struct TextSegmentStyle
{
    std::optional<int> fontWeight;
    std::optional<bool> italic;
    std::optional<bool> underline;
    std::optional<bool> strikethrough;
    std::optional<COLORREF> color;
    std::optional<BYTE> alpha;
    std::optional<int> fontSize;
    std::optional<std::wstring> fontFace;
    std::optional<GradientInfo> gradient;
};

struct TextSegment
{
    std::wstring text;
    TextSegmentStyle style;
    UINT32 startPos;
    UINT32 length;
};

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
          int fontWeight, bool italic, TextAlignment textAlign,
          TextClipString clip = TEXT_CLIP_NONE, const std::wstring& fontPath = L"");

    virtual ~TextElement() {}

    virtual void Render(ID2D1DeviceContext* context) override;

    void SetText(const std::wstring& text) { m_Text = text; ParseInlineStyles(); }
    void SetFontFace(const std::wstring& font) { m_FontFace = font; ParseInlineStyles(); }
    void SetFontSize(int size) { m_FontSize = size; ParseInlineStyles(); }
    void SetFontColor(COLORREF color, BYTE alpha) { m_FontColor = color; m_Alpha = alpha; ParseInlineStyles(); }
    void SetFontWeight(int weight) { m_FontWeight = weight; ParseInlineStyles(); }
    void SetItalic(bool italic) { m_Italic = italic; ParseInlineStyles(); }
    void SetTextAlign(TextAlignment align) { m_TextAlign = align; }
    void SetClip(TextClipString clip) { m_ClipString = clip; }
    void SetFontPath(const std::wstring& path) { m_FontPath = path; }
    void SetShadows(const std::vector<TextShadow>& shadows) { m_Shadows = shadows; }
    void SetFontGradient(const GradientInfo& gradient) { m_FontGradient = gradient; }
    void SetLetterSpacing(float spacing) { m_LetterSpacing = spacing; }
    void SetUnderline(bool underline) { m_UnderLine = underline; }
    void SetStrikethrough(bool strikethrough) { m_StrikeThrough = strikethrough; }
    void SetTextCase(TextCase textCase) { m_TextCase = textCase; }

    const std::wstring& GetText() const { return m_Text; }
    const std::wstring& GetCleanText() const { return m_CleanText; }
    const std::wstring& GetFontFace() const { return m_FontFace; }
    int GetFontSize() const { return m_FontSize; }
    COLORREF GetFontColor() const { return m_FontColor; }
    BYTE GetFontAlpha() const { return m_Alpha; }
    int GetFontWeight() const { return m_FontWeight; }
    bool IsItalic() const { return m_Italic; }
    TextAlignment GetTextAlign() const { return m_TextAlign; }
    TextClipString GetClipString() const { return m_ClipString; }
    const std::wstring& GetFontPath() const { return m_FontPath; }
    const std::vector<TextShadow>& GetShadows() const { return m_Shadows; }
    const GradientInfo& GetFontGradient() const { return m_FontGradient; }
    float GetLetterSpacing() const { return m_LetterSpacing; }
    bool GetUnderline() const { return m_UnderLine; }
    bool GetStrikethrough() const { return m_StrikeThrough; }
    TextCase GetTextCase() const { return m_TextCase; }

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual GfxRect GetBounds() override; // Keeping ROI as GfxRect for now as it's used for layout, but internally use D2D
    virtual bool HitTest(int x, int y) override;

    std::wstring GetProcessedText() const;

private:
    void ParseInlineStyles();

    std::wstring m_Text;
    std::wstring m_CleanText;
    std::wstring m_FontFace;
    int m_FontSize;
    COLORREF m_FontColor;
    BYTE m_Alpha;
    int m_FontWeight;
    bool m_Italic;
    TextAlignment m_TextAlign;
    TextClipString m_ClipString;
    std::wstring m_FontPath;
    std::vector<TextShadow> m_Shadows;
    GradientInfo m_FontGradient;
    float m_LetterSpacing = 0.0f;
    bool m_UnderLine = false;
    bool m_StrikeThrough = false;
    TextCase m_TextCase = TEXT_CASE_NORMAL;

    std::vector<TextSegment> m_Segments;
};

#endif
