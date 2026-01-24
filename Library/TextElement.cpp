/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "TextElement.h"
#include "Logging.h"
#include "Direct2DHelper.h"

TextElement::TextElement(const std::wstring& id, int x, int y, int w, int h,
     const std::wstring& text, const std::wstring& fontFace,
     int fontSize, COLORREF fontColor, BYTE alpha,
     bool bold, bool italic, TextAlignment textAlign,
     TextClipString clip)
    : Element(ELEMENT_TEXT, id, x, y, w, h),
      m_Text(text), m_FontFace(fontFace), m_FontSize(fontSize),
      m_FontColor(fontColor), m_Alpha(alpha), m_Bold(bold), m_Italic(italic),
      m_TextAlign(textAlign), m_ClipString(clip)
{
}

void TextElement::Render(ID2D1DeviceContext* context)
{
    // Draw background first
    RenderBackground(context);

    // Draw bevel second
    RenderBevel(context);

    if (m_Text.empty()) return;

    // Create text format
    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        m_FontFace.c_str(),
        nullptr,
        m_Bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        (float)m_FontSize,
        L"", // locale
        pTextFormat.GetAddressOf()
    );

    if (FAILED(hr)) return;

    // Set alignment
    switch (m_TextAlign)
    {
    case TEXT_ALIGN_LEFT_TOP:
    case TEXT_ALIGN_LEFT_CENTER:
    case TEXT_ALIGN_LEFT_BOTTOM:
        pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        break;
    case TEXT_ALIGN_CENTER_TOP:
    case TEXT_ALIGN_CENTER_CENTER:
    case TEXT_ALIGN_CENTER_BOTTOM:
        pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    case TEXT_ALIGN_RIGHT_TOP:
    case TEXT_ALIGN_RIGHT_CENTER:
    case TEXT_ALIGN_RIGHT_BOTTOM:
        pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        break;
    }

    switch (m_TextAlign)
    {
    case TEXT_ALIGN_LEFT_TOP:
    case TEXT_ALIGN_CENTER_TOP:
    case TEXT_ALIGN_RIGHT_TOP:
        pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        break;
    case TEXT_ALIGN_LEFT_CENTER:
    case TEXT_ALIGN_CENTER_CENTER:
    case TEXT_ALIGN_RIGHT_CENTER:
        pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        break;
    case TEXT_ALIGN_LEFT_BOTTOM:
    case TEXT_ALIGN_CENTER_BOTTOM:
    case TEXT_ALIGN_RIGHT_BOTTOM:
        pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        break;
    }

    // Set trimming/clipping and wrapping behavior berdasarkan standar:
    // "none" / "wrap": Enable wrapping if width provided
    // "clip" / "ellipsis": Single line only
    
    bool allowWrap = (m_ClipString == TEXT_CLIP_NONE || m_ClipString == TEXT_CLIP_WRAP);
    
    if (allowWrap)
    {
        pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    }
    else
    {
        pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        
        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        Microsoft::WRL::ComPtr<IDWriteInlineObject> pEllipsis;
        
        if (m_ClipString == TEXT_CLIP_ELLIPSIS)
        {
            Direct2D::GetWriteFactory()->CreateEllipsisTrimmingSign(pTextFormat.Get(), pEllipsis.GetAddressOf());
            pTextFormat->SetTrimming(&trimming, pEllipsis.Get());
        }
        else if (m_ClipString == TEXT_CLIP_ON)
        {
            pTextFormat->SetTrimming(&trimming, nullptr);
        }
    }
    
    // Apply rotation if specified
    D2D1_MATRIX_3X2_F originalTransform;
    context->GetTransform(&originalTransform);
    if (m_Rotate != 0.0f)
    {
        GfxRect bounds = GetBounds();
        D2D1_POINT_2F center = D2D1::Point2F(bounds.X + bounds.Width / 2.0f, bounds.Y + bounds.Height / 2.0f);
        context->SetTransform(D2D1::Matrix3x2F::Rotation(m_Rotate, center) * originalTransform);
    }
    
    // Apply padding to layout rectangle
    GfxRect bounds = GetBounds();
    float layoutX = (float)bounds.X + m_PaddingLeft;
    float layoutY = (float)bounds.Y + m_PaddingTop;
    float layoutW = (float)bounds.Width - m_PaddingLeft - m_PaddingRight;
    float layoutH = (float)bounds.Height - m_PaddingTop - m_PaddingBottom;
    
    if (layoutW < 0) layoutW = 0;
    if (layoutH < 0) layoutH = 0;
    
    D2D1_RECT_F layoutRect = D2D1::RectF(layoutX, layoutY, layoutX + layoutW, layoutY + layoutH);

    // Create brush
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrush;
    Direct2D::CreateSolidBrush(context, m_FontColor, m_Alpha / 255.0f, pBrush.GetAddressOf());

    if (pBrush)
    {
        context->SetTextAntialiasMode(m_AntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
        context->DrawText(m_Text.c_str(), (UINT32)m_Text.length(), pTextFormat.Get(), layoutRect, pBrush.Get());
    }
    
    // Reset transform
    context->SetTransform(originalTransform);
}

int TextElement::GetAutoWidth()
{
    if (m_Text.empty()) return 0;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        m_FontFace.c_str(), nullptr,
        m_Bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, (float)m_FontSize, L"",
        pTextFormat.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    // For AutoWidth, we never wrap
    pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        m_Text.c_str(), (UINT32)m_Text.length(), pTextFormat.Get(),
        10000.0f, 10000.0f, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    DWRITE_TEXT_METRICS metrics;
    pLayout->GetMetrics(&metrics);

    int width = (int)ceil(metrics.widthIncludingTrailingWhitespace);
    
    // Add element padding
    width += m_PaddingLeft + m_PaddingRight;

    if (!m_WDefined && (m_ClipString == TEXT_CLIP_ON || m_ClipString == TEXT_CLIP_ELLIPSIS) && m_Width > 0)
    {
        if (width > m_Width) return m_Width;
    }
    return width;
}

int TextElement::GetAutoHeight()
{
    if (m_Text.empty()) return 0;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        m_FontFace.c_str(), nullptr,
        m_Bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, (float)m_FontSize, L"",
        pTextFormat.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    float maxWidth = 10000.0f;
    bool wrap = (m_ClipString == TEXT_CLIP_NONE || m_ClipString == TEXT_CLIP_WRAP);

    if (wrap)
    {
        int elementW = GetWidth();
        if (elementW > 0)
        {
            int contentW = elementW - m_PaddingLeft - m_PaddingRight;
            maxWidth = (float)(contentW > 0 ? contentW : 1);
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        }
        else
        {
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
    }
    else
    {
        pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }

    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        m_Text.c_str(), (UINT32)m_Text.length(), pTextFormat.Get(),
        maxWidth, 10000.0f, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    DWRITE_TEXT_METRICS metrics;
    pLayout->GetMetrics(&metrics);

    int height = (int)ceil(metrics.height);
    
    // Add element padding
    height += m_PaddingTop + m_PaddingBottom;

    if (!m_HDefined && (m_ClipString == TEXT_CLIP_ON || m_ClipString == TEXT_CLIP_ELLIPSIS) && m_Height > 0)
    {
        if (height > m_Height) return m_Height;
    }
    return height;
}

GfxRect TextElement::GetBounds() {
    int w = GetWidth();
    int h = GetHeight();
    int x = m_X;
    int y = m_Y;

    // Horizontal Alignment Offset
    if (m_TextAlign == TEXT_ALIGN_CENTER_TOP || m_TextAlign == TEXT_ALIGN_CENTER_CENTER || m_TextAlign == TEXT_ALIGN_CENTER_BOTTOM) {
        x -= w / 2;
    }
    else if (m_TextAlign == TEXT_ALIGN_RIGHT_TOP || m_TextAlign == TEXT_ALIGN_RIGHT_CENTER || m_TextAlign == TEXT_ALIGN_RIGHT_BOTTOM) {
        x -= w;
    }

    // Vertical Alignment Offset
    if (m_TextAlign == TEXT_ALIGN_LEFT_CENTER || m_TextAlign == TEXT_ALIGN_CENTER_CENTER || m_TextAlign == TEXT_ALIGN_RIGHT_CENTER) {
        y -= h / 2;
    }
    else if (m_TextAlign == TEXT_ALIGN_LEFT_BOTTOM || m_TextAlign == TEXT_ALIGN_CENTER_BOTTOM || m_TextAlign == TEXT_ALIGN_RIGHT_BOTTOM) {
        y -= h;
    }

    return GfxRect(x, y, w, h);
}

bool TextElement::HitTest(int x, int y)
{
    // Bounding box check first (Element's bounds)
    if (!Element::HitTest(x, y)) return false;

    // If we have a background or gradient, the entire element is clickable
    if ((m_HasSolidColor && m_SolidAlpha > 0) || (m_HasGradient)) return true;

    // Use DirectWrite for precise hit testing
    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        m_FontFace.c_str(), nullptr,
        m_Bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, (float)m_FontSize, L"",
        pTextFormat.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    GfxRect bounds = GetBounds();
    float layoutW = (float)bounds.Width - m_PaddingLeft - m_PaddingRight;
    float layoutH = (float)bounds.Height - m_PaddingTop - m_PaddingBottom;
    if (layoutW < 0) layoutW = 1;
    if (layoutH < 0) layoutH = 1;

    bool allowWrap = (m_ClipString == TEXT_CLIP_NONE || m_ClipString == TEXT_CLIP_WRAP);
    pTextFormat->SetWordWrapping(allowWrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);

    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        m_Text.c_str(), (UINT32)m_Text.length(), pTextFormat.Get(),
        layoutW, layoutH, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // Get point relative to layout area
    float relX = (float)x - (bounds.X + m_PaddingLeft);
    float relY = (float)y - (bounds.Y + m_PaddingTop);

    BOOL isTrailingHit;
    BOOL isInside;
    DWRITE_HIT_TEST_METRICS hitMetrics;
    pLayout->HitTestPoint(relX, relY, &isTrailingHit, &isInside, &hitMetrics);

    return isInside;
}
