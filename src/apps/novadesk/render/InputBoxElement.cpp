/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "InputBoxElement.h"
#include "../shared/Logging.h"
#include "Direct2DHelper.h"
#include "FontManager.h"
#include "ColorUtil.h"
#include <algorithm>
#include <cwctype>

namespace
{
    constexpr DWORD kCaretBlinkMs = 530;

    // Apply the subset of TextElement alignment options the input box supports
    // (horizontal alignment + vertical centering by default).
    void ApplyInputTextAlignment(IDWriteTextFormat *format, TextAlignment align)
    {
        if (!format)
            return;

        switch (align)
        {
        case TEXT_ALIGN_CENTER_TOP:
        case TEXT_ALIGN_CENTER_CENTER:
        case TEXT_ALIGN_CENTER_BOTTOM:
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            break;
        case TEXT_ALIGN_RIGHT_TOP:
        case TEXT_ALIGN_RIGHT_CENTER:
        case TEXT_ALIGN_RIGHT_BOTTOM:
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            break;
        default:
            format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            break;
        }

        switch (align)
        {
        case TEXT_ALIGN_LEFT_TOP:
        case TEXT_ALIGN_CENTER_TOP:
        case TEXT_ALIGN_RIGHT_TOP:
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            break;
        case TEXT_ALIGN_LEFT_BOTTOM:
        case TEXT_ALIGN_CENTER_BOTTOM:
        case TEXT_ALIGN_RIGHT_BOTTOM:
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
            break;
        default:
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            break;
        }
    }
}

InputBoxElement::InputBoxElement(const std::wstring &id, int x, int y, int width, int height)
    : Element(ELEMENT_INPUT_BOX, id, x, y, width, height)
{
    // Input fields typically want a solid background and a small corner radius.
    m_MouseEventCursor = true;
    m_MouseEventCursorName = L"text";
}

GfxRect InputBoxElement::GetBounds()
{
    // Input fields use their declared box directly (no font metrics overflow).
    return GfxRect(m_X, m_Y, GetWidth(), GetHeight());
}

int InputBoxElement::GetAutoWidth()
{
    // Return the raw stored width (no padding, no recursion).
    // Element::GetWidth() adds padding on top of this value.
    return m_Width;
}

int InputBoxElement::GetAutoHeight()
{
    // Return the raw stored height (no padding, no recursion).
    // Element::GetHeight() adds padding on top of this value.
    return m_Height;
}

bool InputBoxElement::HitTest(int x, int y)
{
    if (!m_Show)
        return false;
    GfxRect b = GetBounds();
    return (x >= b.X && x < b.X + b.Width && y >= b.Y && y < b.Y + b.Height);
}

D2D1_RECT_F InputBoxElement::GetContentRect() const
{
    GfxRect b = const_cast<InputBoxElement *>(this)->GetBounds();
    float left = (float)b.X + m_BorderWidth + m_PaddingLeft;
    float top = (float)b.Y + m_BorderWidth + m_PaddingTop;
    float right = (float)b.X + b.Width - m_BorderWidth - m_PaddingRight;
    float bottom = (float)b.Y + b.Height - m_BorderWidth - m_PaddingBottom;
    if (right < left)
        right = left;
    if (bottom < top)
        bottom = top;
    return D2D1::RectF(left, top, right, bottom);
}

void InputBoxElement::UpdateBlink()
{
    DWORD now = GetTickCount();
    if (now - m_LastBlinkTick >= kCaretBlinkMs)
    {
        m_LastBlinkTick = now;
        m_CaretVisible = !m_CaretVisible;
    }
}

void InputBoxElement::SetFocus(bool focused)
{
    if (m_Focused == focused)
        return;
    m_Focused = focused;
    // Reset blink phase so the caret appears immediately on focus.
    m_CaretVisible = focused;
    m_LastBlinkTick = GetTickCount();
    if (focused)
        EnsureCaretVisible();
    else
        m_ScrollOffset = 0.0f;
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> InputBoxElement::CreateTextLayout(
    ID2D1DeviceContext *context, const std::wstring &text, float layoutW, float layoutH) const
{
    (void)context;
    if (!Direct2D::GetWriteFactory())
        return nullptr;

    std::wstring fontFace = m_FontFace.empty() ? L"Segoe UI" : m_FontFace;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
    if (!m_FontPath.empty())
    {
        pCollection = FontManager::GetFontCollection(m_FontPath);
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        fontFace.c_str(),
        pCollection.Get(),
        (DWRITE_FONT_WEIGHT)m_FontWeight,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        (float)m_FontSize,
        L"",
        pFormat.GetAddressOf());
    if (FAILED(hr) || !pFormat)
        return nullptr;

    TextAlignment align = m_TextAlign;
    if (m_Multiline)
    {
        if (align == TEXT_ALIGN_LEFT_CENTER)
            align = TEXT_ALIGN_LEFT_TOP;
        else if (align == TEXT_ALIGN_CENTER_CENTER)
            align = TEXT_ALIGN_CENTER_TOP;
        else if (align == TEXT_ALIGN_RIGHT_CENTER)
            align = TEXT_ALIGN_RIGHT_TOP;
    }
    ApplyInputTextAlignment(pFormat.Get(), align);
    pFormat->SetWordWrapping(m_Multiline ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);

    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        text.c_str(), (UINT32)text.length(), pFormat.Get(),
        layoutW, layoutH, pLayout.GetAddressOf());
    if (FAILED(hr))
        return nullptr;

    return pLayout;
}

float InputBoxElement::CaretIndexToX(UINT32 index) const
{
    D2D1_RECT_F content = GetContentRect();
    float w = content.right - content.left;
    float h = content.bottom - content.top;
    if (w < 1.0f)
        w = 1.0f;
    if (h < 1.0f)
        h = 1.0f;

    std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
    if (index > text.size())
        index = (UINT32)text.size();

    auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, w, h);
    if (!layout)
        return content.left;

    DWRITE_HIT_TEST_METRICS metrics{};
    float x = 0, y = 0;
    layout->HitTestTextPosition(index, FALSE, &x, &y, &metrics);
    return content.left + x;
}

void InputBoxElement::CaretIndexToXY(UINT32 index, float &outX, float &outY, float &outH) const
{
    D2D1_RECT_F content = GetContentRect();
    float w = content.right - content.left;
    float h = content.bottom - content.top;
    if (w < 1.0f)
        w = 1.0f;
    if (h < 1.0f)
        h = 1.0f;

    std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
    if (index > text.size())
        index = (UINT32)text.size();

    auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, w, h);
    if (!layout)
    {
        outX = content.left;
        outY = content.top;
        outH = (float)m_FontSize;
        return;
    }

    DWRITE_HIT_TEST_METRICS metrics{};
    float x = 0, y = 0;
    layout->HitTestTextPosition(index, FALSE, &x, &y, &metrics);
    outX = content.left + x;
    outY = content.top + y;
    outH = metrics.height;
}

float InputBoxElement::MeasureTextWidth() const
{
    D2D1_RECT_F content = GetContentRect();
    float w = content.right - content.left;
    float h = content.bottom - content.top;
    if (w < 1.0f)
        w = 1.0f;
    if (h < 1.0f)
        h = 1.0f;

    std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
    if (text.empty())
        return 0.0f;

    auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, w, h);
    if (!layout)
        return 0.0f;

    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    return metrics.widthIncludingTrailingWhitespace;
}

void InputBoxElement::EnsureCaretVisible()
{
    D2D1_RECT_F content = GetContentRect();
    float contentW = content.right - content.left;
    float contentH = content.bottom - content.top;
    if (contentW < 1.0f)
        contentW = 1.0f;
    if (contentH < 1.0f)
        contentH = 1.0f;

    if (m_Multiline)
    {
        std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
        auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, contentW, contentH);
        if (!layout)
        {
            m_ScrollOffset = 0.0f;
            return;
        }

        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        float textH = metrics.height;

        // If the text fits vertically, no scrolling needed.
        if (textH <= contentH)
        {
            m_ScrollOffset = 0.0f;
            return;
        }

        float caretX = 0.0f, caretY = 0.0f, caretH = 0.0f;
        CaretIndexToXY(m_CaretPos, caretX, caretY, caretH);
        float localCaretY = caretY - content.top;

        if (localCaretY < m_ScrollOffset)
        {
            m_ScrollOffset = localCaretY;
        }
        else if (localCaretY + caretH > m_ScrollOffset + contentH)
        {
            m_ScrollOffset = localCaretY + caretH - contentH;
        }

        float maxOffset = textH - contentH;
        if (m_ScrollOffset > maxOffset)
            m_ScrollOffset = maxOffset;
        if (m_ScrollOffset < 0.0f)
            m_ScrollOffset = 0.0f;
    }
    else
    {
        float textW = MeasureTextWidth();

        // A small margin (in DIPs) to ensure the caret is not clipped at the right edge.
        constexpr float kCaretMargin = 2.0f;

        // If the text fits entirely (including the caret margin), no scrolling needed.
        if (textW + kCaretMargin <= contentW)
        {
            m_ScrollOffset = 0.0f;
            return;
        }

        // Caret x position relative to the text origin (unscrolled).
        float caretX = CaretIndexToX(m_CaretPos) - content.left;

        // Visible window of the text in text-local coordinates:
        //   [m_ScrollOffset, m_ScrollOffset + contentW)
        float caretScreenX = caretX - m_ScrollOffset;

        if (caretScreenX < 0.0f)
        {
            // Caret scrolled off the left edge: snap so caret is at the left.
            m_ScrollOffset = caretX;
        }
        else if (caretScreenX > contentW - kCaretMargin)
        {
            // Caret scrolled off the right edge: snap so caret is at the right edge minus margin.
            m_ScrollOffset = caretX - (contentW - kCaretMargin);
        }

        // Clamp so we don't scroll past the end of the text.
        float maxOffset = textW - (contentW - kCaretMargin);
        if (m_ScrollOffset > maxOffset)
            m_ScrollOffset = maxOffset;
        if (m_ScrollOffset < 0.0f)
            m_ScrollOffset = 0.0f;
    }
}

UINT32 InputBoxElement::PointToCaretIndex(int x, int y) const
{
    D2D1_RECT_F content = GetContentRect();
    float w = content.right - content.left;
    float h = content.bottom - content.top;
    if (w < 1.0f)
        w = 1.0f;
    if (h < 1.0f)
        h = 1.0f;

    std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
    auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, w, h);
    if (!layout)
        return (UINT32)m_Text.size();

    float relX = (float)x - content.left + (m_Multiline ? 0.0f : m_ScrollOffset);
    float relY = (float)y - content.top + (m_Multiline ? m_ScrollOffset : 0.0f);

    BOOL isTrailing = FALSE;
    BOOL isInside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics{};
    layout->HitTestPoint(relX, relY, &isTrailing, &isInside, &metrics);

    UINT32 pos = metrics.textPosition;
    if (isTrailing)
        pos += metrics.length;
    if (pos > (UINT32)m_Text.size())
        pos = (UINT32)m_Text.size();
    return pos;
}

void InputBoxElement::NormalizeSelection(UINT32 &outStart, UINT32 &outEnd) const
{
    if (m_SelectionStart <= m_SelectionEnd)
    {
        outStart = m_SelectionStart;
        outEnd = m_SelectionEnd;
    }
    else
    {
        outStart = m_SelectionEnd;
        outEnd = m_SelectionStart;
    }
}

std::wstring InputBoxElement::GetSelectedText() const
{
    UINT32 s, e;
    NormalizeSelection(s, e);
    if (s >= e)
        return L"";
    return m_Text.substr(s, e - s);
}

void InputBoxElement::DeleteSelection()
{
    UINT32 s, e;
    NormalizeSelection(s, e);
    if (s >= e)
        return;
    SaveUndoState();
    m_Text.erase(s, e - s);
    m_CaretPos = s;
    m_SelectionStart = m_SelectionEnd = s;
    m_SelectionAnchor = s;
    EnsureCaretVisible();
}

void InputBoxElement::ReplaceSelection(const std::wstring &text)
{
    if (!HasSelection())
    {
        SaveUndoState();
    }
    else
    {
        DeleteSelection();
    }

    if (m_MaxLength > 0 && (int)(m_Text.size() + text.size()) > m_MaxLength)
    {
        int remaining = m_MaxLength - (int)m_Text.size();
        if (remaining <= 0)
            return;
        m_Text.insert(m_CaretPos, text, 0, (size_t)remaining);
        m_CaretPos += (UINT32)remaining;
    }
    else
    {
        m_Text.insert(m_CaretPos, text);
        m_CaretPos += (UINT32)text.size();
    }
    m_SelectionStart = m_SelectionEnd = m_CaretPos;
    m_SelectionAnchor = m_CaretPos;
    EnsureCaretVisible();
}

void InputBoxElement::SelectAll()
{
    m_SelectionStart = 0;
    m_SelectionEnd = (UINT32)m_Text.size();
    m_SelectionAnchor = 0;
    m_CaretPos = (UINT32)m_Text.size();
    EnsureCaretVisible();
}

void InputBoxElement::ClearSelection()
{
    m_SelectionStart = m_SelectionEnd = m_CaretPos;
    m_SelectionAnchor = m_CaretPos;
}

void InputBoxElement::SetText(const std::wstring &text)
{
    m_Text = text;
    if (m_CaretPos > m_Text.size())
        m_CaretPos = (UINT32)m_Text.size();
    m_SelectionStart = m_SelectionEnd = m_CaretPos;
    m_SelectionAnchor = m_CaretPos;
    // Programmatic text change: reset scroll to the start.
    m_ScrollOffset = 0.0f;

    m_UndoStack.clear();
    m_RedoStack.clear();
}

InputBoxElement::HandleCharResult InputBoxElement::HandleChar(wchar_t ch)
{
    if (!m_Focused)
        return HandleCharResult::Ignored;

    // Ignore control characters (handled by HandleKeyDown).
    if (ch < 32)
        return HandleCharResult::Ignored;

    // Ctrl-key shortcuts arrive as WM_CHAR (e.g. 0x03 for Ctrl+C); skip them.
    if (ch == 3 /*Ctrl+C*/ || ch == 22 /*Ctrl+V*/ || ch == 1 /*Ctrl+A*/ || ch == 24 /*Ctrl+X*/)
        return HandleCharResult::Ignored;

    // Input type filtering
    switch (m_InputType)
    {
    case InputType::Integer:
        // Allow digits and a leading '-' only at position 0
        if (!((ch >= L'0' && ch <= L'9') || (ch == L'-' && m_CaretPos == 0 && m_Text.find(L'-') == std::wstring::npos)))
            return HandleCharResult::Rejected;
        break;
    case InputType::Float:
        // Allow digits, leading '-' at position 0, and a single '.'
        if (!((ch >= L'0' && ch <= L'9') ||
              (ch == L'-' && m_CaretPos == 0 && m_Text.find(L'-') == std::wstring::npos) ||
              (ch == L'.' && m_Text.find(L'.') == std::wstring::npos)))
            return HandleCharResult::Rejected;
        break;
    case InputType::Letters:
        if (!iswalpha(ch))
            return HandleCharResult::Rejected;
        break;
    case InputType::Alphanumeric:
        if (!iswalnum(ch))
            return HandleCharResult::Rejected;
        break;
    case InputType::Hex:
    {
        wchar_t u = (wchar_t)towupper(ch);
        if (!((u >= L'0' && u <= L'9') || (u >= L'A' && u <= L'F')))
            return HandleCharResult::Rejected;
        break;
    }
    case InputType::Email:
        if (!(iswalnum(ch) || ch == L'@' || ch == L'.' || ch == L'-' || ch == L'_' || ch == L'+'))
            return HandleCharResult::Rejected;
        break;
    case InputType::Custom:
        // Only allow characters that are in the user-supplied allowedChars set.
        if (m_AllowedChars.empty() || m_AllowedChars.find(ch) == std::wstring::npos)
            return HandleCharResult::Rejected;
        break;
    case InputType::Any:
    default:
        break;
    }

    if (HasSelection())
    {
        DeleteSelection();
    }
    else if (m_MaxLength > 0 && (int)m_Text.size() >= m_MaxLength)
    {
        return HandleCharResult::Ignored;
    }
    else
    {
        SaveUndoState();
    }

    m_Text.insert(m_CaretPos, 1, ch);
    m_CaretPos += 1;
    m_SelectionStart = m_SelectionEnd = m_CaretPos;
    m_SelectionAnchor = m_CaretPos;
    m_CaretVisible = true;
    m_LastBlinkTick = GetTickCount();

    EnsureCaretVisible();
    return HandleCharResult::Changed;
}


bool InputBoxElement::HandleKeyDown(WPARAM vk, bool shift, bool control)
{
    if (!m_Focused)
        return false;

    bool changed = false;

    if (control && (vk == 'A'))
    {
        SelectAll();
        EnsureCaretVisible();
        return false; // no content change, but redraw needed
    }
    if (control && (vk == 'C'))
    {
        // Copy handled by Widget (clipboard). No content change.
        return false;
    }
    if (control && (vk == 'V'))
    {
        // Paste handled by Widget (clipboard) -> calls ReplaceSelection.
        return false;
    }
    if (control && (vk == 'X'))
    {
        // Cut: Widget reads selection into clipboard then we delete.
        return false;
    }
    if (control && (vk == 'Z'))
    {
        if (shift)
            changed = Redo();
        else
            changed = Undo();
        return changed;
    }
    if (control && (vk == 'Y'))
    {
        changed = Redo();
        return changed;
    }

    switch (vk)
    {
    case VK_TAB:
        ReplaceSelection(L"    ");
        changed = true;
        break;

    case VK_BACK:
        if (HasSelection())
        {
            DeleteSelection();
            changed = true;
        }
        else if (m_CaretPos > 0)
        {
            SaveUndoState();
            // Delete previous code unit. For surrogate pairs this is simplified.
            m_Text.erase(m_CaretPos - 1, 1);
            m_CaretPos -= 1;
            m_SelectionStart = m_SelectionEnd = m_CaretPos;
            m_SelectionAnchor = m_CaretPos;
            changed = true;
        }
        break;

    case VK_DELETE:
        if (HasSelection())
        {
            DeleteSelection();
            changed = true;
        }
        else if (m_CaretPos < m_Text.size())
        {
            SaveUndoState();
            m_Text.erase(m_CaretPos, 1);
            changed = true;
        }
        break;

    case VK_LEFT:
    {
        UINT32 newPos = (m_CaretPos > 0) ? m_CaretPos - 1 : 0;
        if (shift)
        {
            if (!HasSelection())
                m_SelectionAnchor = m_CaretPos;
            m_CaretPos = newPos;
            m_SelectionEnd = newPos;
            m_SelectionStart = m_SelectionAnchor;
        }
        else
        {
            if (HasSelection())
            {
                UINT32 s, e;
                NormalizeSelection(s, e);
                newPos = s;
            }
            ClearSelection();
            m_CaretPos = newPos;
        }
        break;
    }

    case VK_RIGHT:
    {
        UINT32 newPos = (m_CaretPos < m_Text.size()) ? m_CaretPos + 1 : (UINT32)m_Text.size();
        if (shift)
        {
            if (!HasSelection())
                m_SelectionAnchor = m_CaretPos;
            m_CaretPos = newPos;
            m_SelectionEnd = newPos;
            m_SelectionStart = m_SelectionAnchor;
        }
        else
        {
            if (HasSelection())
            {
                UINT32 s, e;
                NormalizeSelection(s, e);
                newPos = e;
            }
            ClearSelection();
            m_CaretPos = newPos;
        }
        break;
    }

    case VK_HOME:
    {
        UINT32 newPos = 0;
        if (shift)
        {
            if (!HasSelection())
                m_SelectionAnchor = m_CaretPos;
            m_CaretPos = newPos;
            m_SelectionEnd = newPos;
            m_SelectionStart = m_SelectionAnchor;
        }
        else
        {
            ClearSelection();
            m_CaretPos = newPos;
        }
        break;
    }

    case VK_END:
    {
        UINT32 newPos = (UINT32)m_Text.size();
        if (shift)
        {
            if (!HasSelection())
                m_SelectionAnchor = m_CaretPos;
            m_CaretPos = newPos;
            m_SelectionEnd = newPos;
            m_SelectionStart = m_SelectionAnchor;
        }
        else
        {
            ClearSelection();
            m_CaretPos = newPos;
        }
        break;
    }

    case VK_UP:
    case VK_DOWN:
    {
        if (m_Multiline)
        {
            float caretX = 0.0f, caretY = 0.0f, caretH = 0.0f;
            CaretIndexToXY(m_CaretPos, caretX, caretY, caretH);

            D2D1_RECT_F content = GetContentRect();
            float w = content.right - content.left;
            float h = content.bottom - content.top;
            if (w < 1.0f) w = 1.0f;
            if (h < 1.0f) h = 1.0f;

            std::wstring text = m_Password ? std::wstring(m_Text.size(), L'\x2022') : m_Text;
            auto layout = const_cast<InputBoxElement *>(this)->CreateTextLayout(nullptr, text, w, h);
            if (layout)
            {
                // Move vertically by caret height (approx 1 line height)
                float targetY = (vk == VK_UP) ? (caretY - content.top - caretH / 2.0f) : (caretY - content.top + caretH * 1.5f);
                float targetX = caretX - content.left;

                BOOL isTrailing = FALSE;
                BOOL isInside = FALSE;
                DWRITE_HIT_TEST_METRICS metrics{};
                layout->HitTestPoint(targetX, targetY, &isTrailing, &isInside, &metrics);

                UINT32 newPos = metrics.textPosition;
                if (isTrailing)
                    newPos += metrics.length;
                if (newPos > (UINT32)m_Text.size())
                    newPos = (UINT32)m_Text.size();

                if (shift)
                {
                    if (!HasSelection())
                        m_SelectionAnchor = m_CaretPos;
                    m_CaretPos = newPos;
                    m_SelectionEnd = newPos;
                    m_SelectionStart = m_SelectionAnchor;
                }
                else
                {
                    ClearSelection();
                    m_CaretPos = newPos;
                }
            }
        }
        break;
    }

    default:
        break;
    }

    m_CaretVisible = true;
    m_LastBlinkTick = GetTickCount();
    EnsureCaretVisible();
    return changed;
}

void InputBoxElement::HandleMouseDown(int x, int y, bool shift)
{
    UINT32 pos = PointToCaretIndex(x, y);
    if (shift && m_Focused)
    {
        if (!HasSelection())
            m_SelectionAnchor = m_CaretPos;
        m_CaretPos = pos;
        m_SelectionStart = m_SelectionAnchor;
        m_SelectionEnd = pos;
    }
    else
    {
        m_CaretPos = pos;
        m_SelectionAnchor = pos;
        m_SelectionStart = m_SelectionEnd = pos;
    }
    m_IsDragging = true;
    m_CaretVisible = true;
    m_LastBlinkTick = GetTickCount();
    EnsureCaretVisible();
}

void InputBoxElement::HandleMouseMove(int x, int y)
{
    if (!m_IsDragging)
        return;
    UINT32 pos = PointToCaretIndex(x, y);
    m_CaretPos = pos;
    m_SelectionStart = m_SelectionAnchor;
    m_SelectionEnd = pos;
    m_CaretVisible = true;
    m_LastBlinkTick = GetTickCount();
    EnsureCaretVisible();
}

void InputBoxElement::HandleMouseUp()
{
    m_IsDragging = false;
}

void InputBoxElement::Render(ID2D1DeviceContext *context)
{
    if (!context)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    // Background (uses m_SolidColor / m_CornerRadius set via SetSolidColor/SetCornerRadius)
    RenderBackground(context);
    RenderBevel(context);

    // Draw the custom Fill Color if defined (or Fill Gradient)
    if ((m_HasFillColor && m_FillAlpha > 0) || m_FillGradient.type != GRADIENT_NONE)
    {
        D2D1_RECT_F fillRect = D2D1::RectF((float)m_X, (float)m_Y,
                                            (float)(m_X + GetWidth()), (float)(m_Y + GetHeight()));

        Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
        if (Direct2D::CreateBrushFromGradientOrColor(context, fillRect, &m_FillGradient, m_FillColor, m_FillAlpha / 255.0f, fillBrush.GetAddressOf()) && fillBrush)
        {
            if (m_BorderRadius > 0.0f)
            {
                D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(fillRect, m_BorderRadius, m_BorderRadius);
                context->FillRoundedRectangle(roundedRect, fillBrush.Get());
            }
            else
            {
                context->FillRectangle(fillRect, fillBrush.Get());
            }
        }
    }

    // Border (solid or gradient)
    COLORREF activeColor = m_BorderColor;
    BYTE activeAlpha = m_BorderAlpha;
    const GradientInfo *activeGradient = &m_BorderGradient;
    if (m_Focused && (m_HasBorderFocusColor || m_BorderFocusGradient.type != GRADIENT_NONE))
    {
        if (m_HasBorderFocusColor)
        {
            activeColor = m_BorderFocusColor;
            activeAlpha = m_BorderFocusAlpha;
        }
        activeGradient = &m_BorderFocusGradient;
    }

    if (m_BorderWidth > 0.0f && (activeAlpha > 0 || activeGradient->type != GRADIENT_NONE))
    {
        D2D1_RECT_F borderRectF = D2D1::RectF((float)m_X, (float)m_Y,
                                       (float)(m_X + GetWidth()), (float)(m_Y + GetHeight()));

        Microsoft::WRL::ComPtr<ID2D1Brush> borderBrush;
        if (Direct2D::CreateBrushFromGradientOrColor(context, borderRectF, activeGradient, activeColor, activeAlpha / 255.0f, borderBrush.GetAddressOf()) && borderBrush)
        {
            if (m_BorderRadius > 0.0f)
            {
                D2D1_ROUNDED_RECT borderRect = D2D1::RoundedRect(borderRectF, m_BorderRadius, m_BorderRadius);
                context->DrawRoundedRectangle(borderRect, borderBrush.Get(), m_BorderWidth, nullptr);
            }
            else
            {
                context->DrawRectangle(borderRectF, borderBrush.Get(), m_BorderWidth, nullptr);
            }
        }
    }

    D2D1_RECT_F content = GetContentRect();
    float layoutW = content.right - content.left;
    float layoutH = content.bottom - content.top;
    if (layoutW < 1.0f)
        layoutW = 1.0f;
    if (layoutH < 1.0f)
        layoutH = 1.0f;

    // Clip drawing to the content rect so text/caret never overflow the box.
    Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> clipGeom;
    if (Direct2D::GetFactory())
    {
        Direct2D::GetFactory()->CreateRectangleGeometry(content, clipGeom.GetAddressOf());
    }

    bool pushedClip = false;
    if (clipGeom)
    {
        context->PushLayer(D2D1::LayerParameters(content, clipGeom.Get()), nullptr);
        pushedClip = true;
    }

    bool showingPlaceholder = m_Text.empty() && !m_Placeholder.empty();
    const std::wstring &drawText = m_Text.empty() ? m_Placeholder : m_Text;
    std::wstring maskedText;
    if (m_Password && !m_Text.empty())
    {
        maskedText.assign(m_Text.size(), L'\x2022');
    }
    const std::wstring &renderText = m_Password ? (m_Text.empty() ? m_Placeholder : maskedText)
                                                : drawText;

    if (!renderText.empty())
    {
        auto layout = CreateTextLayout(context, renderText, layoutW, layoutH);
        if (layout)
        {
            // Selection highlight (behind text)
            if (m_Focused && HasSelection() && !showingPlaceholder)
            {
                UINT32 s, e;
                NormalizeSelection(s, e);
                if (s < e && e <= (UINT32)renderText.size())
                {
                    UINT32 actualCount = 0;
                    layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, nullptr, 0, &actualCount);
                    if (actualCount > 0)
                    {
                        std::vector<DWRITE_HIT_TEST_METRICS> metrics(actualCount);
                        layout->HitTestTextRange(s, e - s, 0.0f, 0.0f, metrics.data(), actualCount, &actualCount);

                        Microsoft::WRL::ComPtr<ID2D1Brush> selBrush;
                        if (Direct2D::CreateBrushFromGradientOrColor(context, content, &m_SelectionGradient, m_SelectionColor, m_SelectionAlpha / 255.0f, selBrush.GetAddressOf()) && selBrush)
                        {
                            for (UINT32 i = 0; i < actualCount; ++i)
                            {
                                D2D1_RECT_F r = D2D1::RectF(
                                    content.left + metrics[i].left - (m_Multiline ? 0.0f : m_ScrollOffset),
                                    content.top + metrics[i].top - (m_Multiline ? m_ScrollOffset : 0.0f),
                                    content.left + metrics[i].left + metrics[i].width - (m_Multiline ? 0.0f : m_ScrollOffset),
                                    content.top + metrics[i].top + metrics[i].height - (m_Multiline ? m_ScrollOffset : 0.0f));
                                context->FillRectangle(r, selBrush.Get());
                            }
                        }
                    }
                }
            }

            // Text brush (placeholder color when empty)
            COLORREF color = showingPlaceholder ? m_PlaceholderColor : m_FontColor;
            BYTE alpha = showingPlaceholder ? m_PlaceholderAlpha : m_FontAlpha;
            const GradientInfo *textGrad = showingPlaceholder ? &m_PlaceholderGradient : &m_FontGradient;

            Microsoft::WRL::ComPtr<ID2D1Brush> textBrush;
            if (Direct2D::CreateBrushFromGradientOrColor(context, content, textGrad, color, alpha / 255.0f, textBrush.GetAddressOf()) && textBrush)
            {
                context->SetTextAntialiasMode(m_AntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
                context->DrawTextLayout(D2D1::Point2F(content.left - (m_Multiline ? 0.0f : m_ScrollOffset), content.top - (m_Multiline ? m_ScrollOffset : 0.0f)),
                                        layout.Get(), textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
            }
        }
    }

    // Caret (only when focused and blinking on)
    if (m_Focused && m_CaretVisible)
    {
        float caretX = 0.0f;
        float caretY = 0.0f;
        float caretH = 0.0f;

        if (showingPlaceholder)
        {
            caretX = content.left;
            caretY = content.top;
            caretH = (float)m_FontSize;
            if (caretH > layoutH)
                caretH = layoutH;
            if (!m_Multiline)
                caretY = content.top + (layoutH - caretH) / 2.0f;
        }
        else
        {
            CaretIndexToXY(m_CaretPos, caretX, caretY, caretH);
            if (m_Multiline)
            {
                caretY = caretY - m_ScrollOffset;
            }
            else
            {
                caretX = caretX - m_ScrollOffset;
                float caretTargetH = (float)m_FontSize;
                if (caretTargetH > layoutH)
                    caretTargetH = layoutH;
                caretY = content.top + (layoutH - caretTargetH) / 2.0f;
                caretH = caretTargetH;
            }
        }
        float caretW = 1.5f;

        D2D1_RECT_F caretRect = D2D1::RectF(caretX, caretY, caretX + caretW, caretY + caretH);
        Microsoft::WRL::ComPtr<ID2D1Brush> caretBrush;
        if (Direct2D::CreateBrushFromGradientOrColor(context, caretRect, &m_CaretGradient, m_CaretColor, m_CaretAlpha / 255.0f, caretBrush.GetAddressOf()) && caretBrush)
        {
            context->FillRectangle(caretRect, caretBrush.Get());
        }
    }

    if (pushedClip)
        context->PopLayer();

    RestoreRenderTransform(context, originalTransform);
}

void InputBoxElement::SaveUndoState()
{
    if (!m_UndoStack.empty())
    {
        const auto &top = m_UndoStack.back();
        if (top.text == m_Text &&
            top.caretPos == m_CaretPos &&
            top.selectionStart == m_SelectionStart &&
            top.selectionEnd == m_SelectionEnd)
        {
            return;
        }
    }

    if (m_UndoStack.size() >= 100)
    {
        m_UndoStack.erase(m_UndoStack.begin());
    }

    UndoState state;
    state.text = m_Text;
    state.caretPos = m_CaretPos;
    state.selectionStart = m_SelectionStart;
    state.selectionEnd = m_SelectionEnd;
    m_UndoStack.push_back(state);

    m_RedoStack.clear();
}

bool InputBoxElement::Undo()
{
    if (m_UndoStack.empty())
        return false;

    UndoState currentState;
    currentState.text = m_Text;
    currentState.caretPos = m_CaretPos;
    currentState.selectionStart = m_SelectionStart;
    currentState.selectionEnd = m_SelectionEnd;
    m_RedoStack.push_back(currentState);

    UndoState prevState = m_UndoStack.back();
    m_UndoStack.pop_back();

    m_Text = prevState.text;
    m_CaretPos = prevState.caretPos;
    m_SelectionStart = prevState.selectionStart;
    m_SelectionEnd = prevState.selectionEnd;
    m_SelectionAnchor = m_CaretPos;

    EnsureCaretVisible();
    return true;
}

bool InputBoxElement::Redo()
{
    if (m_RedoStack.empty())
        return false;

    UndoState currentState;
    currentState.text = m_Text;
    currentState.caretPos = m_CaretPos;
    currentState.selectionStart = m_SelectionStart;
    currentState.selectionEnd = m_SelectionEnd;
    m_UndoStack.push_back(currentState);

    UndoState nextState = m_RedoStack.back();
    m_RedoStack.pop_back();

    m_Text = nextState.text;
    m_CaretPos = nextState.caretPos;
    m_SelectionStart = nextState.selectionStart;
    m_SelectionEnd = nextState.selectionEnd;
    m_SelectionAnchor = m_CaretPos;

    EnsureCaretVisible();
    return true;
}
