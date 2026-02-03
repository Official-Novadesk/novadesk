/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "TextElement.h"
#include "Logging.h"
#include "Direct2DHelper.h"
#include "Logging.h"
#include <d2d1effects.h>
#include <cwctype>
#include <algorithm>
#include "FontManager.h"
#include "ColorUtil.h"
#include "Utils.h"
#include "PropertyParser.h"

TextElement::TextElement(const std::wstring& id, int x, int y, int w, int h,
     const std::wstring& text, const std::wstring& fontFace,
     int fontSize, COLORREF fontColor, BYTE alpha,
     int fontWeight, bool italic, TextAlignment textAlign,
     TextClipString clip, const std::wstring& fontPath)
    : Element(ELEMENT_TEXT, id, x, y, w, h),
      m_Text(text), m_FontFace(fontFace), m_FontSize(fontSize),
      m_FontColor(fontColor), m_Alpha(alpha), m_FontWeight(fontWeight), m_Italic(italic),
      m_TextAlign(textAlign), m_ClipString(clip), m_FontPath(fontPath)
{
    ParseInlineStyles();
}

void TextElement::Render(ID2D1DeviceContext* context)
{
    D2D1_MATRIX_3X2_F originalTransform;
    context->GetTransform(&originalTransform);

    // Apply rotation around the center of the element's total bounds
    GfxRect bounds = GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    if (m_HasTransformMatrix)
    {
        D2D1::Matrix3x2F matrix = D2D1::Matrix3x2F(
            m_TransformMatrix[0], m_TransformMatrix[1],
            m_TransformMatrix[2], m_TransformMatrix[3],
            m_TransformMatrix[4], m_TransformMatrix[5]
        );
        Logging::Log(LogLevel::Debug, L"TextElement(%s) Applying Transform: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f]", 
            m_Id.c_str(), m_TransformMatrix[0], m_TransformMatrix[1], m_TransformMatrix[2], m_TransformMatrix[3], m_TransformMatrix[4], m_TransformMatrix[5]);
        context->SetTransform(matrix * originalTransform);
    }
    else if (m_Rotate != 0.0f)
    {
        context->SetTransform(D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY)) * originalTransform);
    }

    // Draw background and bevel inside the transform
    RenderBackground(context);
    RenderBevel(context);

    if (m_Text.empty())
    {
        context->SetTransform(originalTransform);
        return;
    }

    // Create text format
    std::wstring fontFace = m_FontFace.empty() ? L"Arial" : m_FontFace;
    
    Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
    if (!m_FontPath.empty()) {
        pCollection = FontManager::GetFontCollection(m_FontPath);
        if (pCollection) {
            UINT32 index;
            BOOL exists;
            if (FAILED(pCollection->FindFamilyName(fontFace.c_str(), &index, &exists)) || !exists) {
                Logging::Log(LogLevel::Warn, L"TextElement(%s): Font family '%s' not found in custom collection, falling back to system fonts.", m_Id.c_str(), fontFace.c_str());
                pCollection = nullptr;
            } else {
                Logging::Log(LogLevel::Debug, L"TextElement(%s): Using custom font collection from '%s' (Family found)", m_Id.c_str(), m_FontPath.c_str());
            }
        } else {
            Logging::Log(LogLevel::Warn, L"TextElement(%s): Failed to load custom collection, falling back to system fonts.", m_Id.c_str());
        }
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        fontFace.c_str(),
        pCollection.Get(),
        (DWRITE_FONT_WEIGHT)m_FontWeight,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        (float)m_FontSize,
        L"", // locale
        pTextFormat.GetAddressOf()
    );

    if (FAILED(hr))
    {
        Logging::Log(LogLevel::Error, L"TextElement(%s): Failed to create text format (Font: '%s', Path: '%s') (0x%08X)", 
            m_Id.c_str(), fontFace.c_str(), m_FontPath.c_str(), hr);
        context->SetTransform(originalTransform);
        return;
    }

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

    float layoutX = (float)bounds.X + m_PaddingLeft;
    float layoutY = (float)bounds.Y + m_PaddingTop;
    float layoutW = (float)bounds.Width - m_PaddingLeft - m_PaddingRight;
    float layoutH = (float)bounds.Height - m_PaddingTop - m_PaddingBottom;
    
    if (layoutW < 0) layoutW = 0;
    if (layoutH < 0) layoutH = 0;
    
    D2D1_RECT_F layoutRect = D2D1::RectF(layoutX, layoutY, layoutX + layoutW, layoutY + layoutH);

    // Create brush
    Microsoft::WRL::ComPtr<ID2D1Brush> pBrush;
    if (m_FontGradient.type != GRADIENT_NONE)
    {
        Direct2D::CreateGradientBrush(context, layoutRect, m_FontGradient, pBrush.GetAddressOf());
    }
    else
    {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pSolidBrush;
        Direct2D::CreateSolidBrush(context, m_FontColor, m_Alpha / 255.0f, pSolidBrush.GetAddressOf());
        pBrush = pSolidBrush;
    }

    if (pBrush)
    {
        context->SetTextAntialiasMode(m_AntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

    // Always create a text layout for rendering if we have letter spacing or shadows
    std::wstring processedText = GetProcessedText();
    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        processedText.c_str(), (UINT32)processedText.length(), pTextFormat.Get(),
        layoutW, layoutH, pLayout.GetAddressOf()
    );

    if (SUCCEEDED(hr)) {
        for (const auto& segment : m_Segments) {
            DWRITE_TEXT_RANGE range = { segment.startPos, segment.length };
            
            if (segment.style.fontWeight.has_value()) {
                pLayout->SetFontWeight((DWRITE_FONT_WEIGHT)segment.style.fontWeight.value(), range);
            }
            if (segment.style.italic.has_value()) {
                pLayout->SetFontStyle(segment.style.italic.value() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
            }
            if (segment.style.underline.has_value()) {
                pLayout->SetUnderline(segment.style.underline.value(), range);
            }
            if (segment.style.strikethrough.has_value()) {
                pLayout->SetStrikethrough(segment.style.strikethrough.value(), range);
            }
            if (segment.style.fontSize.has_value()) {
                pLayout->SetFontSize((float)segment.style.fontSize.value(), range);
            }
            if (segment.style.fontFace.has_value()) {
                pLayout->SetFontFamilyName(segment.style.fontFace.value().c_str(), range);
            }
            if (segment.style.gradient.has_value() && segment.style.gradient->type != GRADIENT_NONE) {
                Microsoft::WRL::ComPtr<ID2D1Brush> pSegmentGradientBrush;
                if (Direct2D::CreateGradientBrush(context, layoutRect, segment.style.gradient.value(), &pSegmentGradientBrush)) {
                    pLayout->SetDrawingEffect(pSegmentGradientBrush.Get(), range);
                }
            }
            else if (segment.style.color.has_value()) {
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pSegmentBrush;
                BYTE a = segment.style.alpha.value_or(255);
                if (Direct2D::CreateSolidBrush(context, segment.style.color.value(), a / 255.0f, &pSegmentBrush)) {
                    pLayout->SetDrawingEffect(pSegmentBrush.Get(), range);
                }
            }
        }

        if (m_LetterSpacing != 0.0f) {
                Microsoft::WRL::ComPtr<IDWriteTextLayout1> pLayout1;
                if (SUCCEEDED(pLayout.As(&pLayout1))) {
                    DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
                    pLayout1->SetCharacterSpacing(m_LetterSpacing, 0.0f, 0.0f, range);
                }
            }

            if (m_UnderLine) {
                DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
                pLayout->SetUnderline(TRUE, range);
            }

            if (m_StrikeThrough) {
                DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
                pLayout->SetStrikethrough(TRUE, range);
            }

            if (!m_Shadows.empty())
            {
                // Create a Command List to capture the text with letter spacing for blurred shadows
                Microsoft::WRL::ComPtr<ID2D1CommandList> pCommandList;
                if (SUCCEEDED(context->CreateCommandList(&pCommandList)))
                {
                    Microsoft::WRL::ComPtr<ID2D1Device> pDevice;
                    context->GetDevice(&pDevice);
                    
                    if (pDevice)
                    {
                        Microsoft::WRL::ComPtr<ID2D1DeviceContext> pRecordingContext;
                        if (SUCCEEDED(pDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pRecordingContext)))
                        {
                            pRecordingContext->SetTarget(pCommandList.Get());
                            pRecordingContext->BeginDraw();
                            // For the command list, draw at (0,0)
                            pRecordingContext->DrawTextLayout(D2D1::Point2F(0, 0), pLayout.Get(), pBrush.Get());
                            pRecordingContext->EndDraw();
                            pCommandList->Close();
                        }
                    }

                    for (const auto& shadow : m_Shadows)
                    {
                        if (shadow.blur <= 0.01f)
                        {
                            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pShadowBrush;
                            if (Direct2D::CreateSolidBrush(context, shadow.color, shadow.alpha / 255.0f, &pShadowBrush))
                            {
                                context->DrawTextLayout(D2D1::Point2F(layoutX + shadow.offsetX, layoutY + shadow.offsetY), pLayout.Get(), pShadowBrush.Get());
                            }
                        }
                        else
                        {
                            Microsoft::WRL::ComPtr<ID2D1Effect> pShadowEffect;
                            Microsoft::WRL::ComPtr<ID2D1Effect> pColorMatrixEffect;

                            if (SUCCEEDED(context->CreateEffect(CLSID_D2D1Shadow, &pShadowEffect)) &&
                                SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, &pColorMatrixEffect)))
                            {
                                pShadowEffect->SetInput(0, pCommandList.Get());
                                pShadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, shadow.blur / 2.0f);
                                
                                pColorMatrixEffect->SetInputEffect(0, pShadowEffect.Get());
                                D2D1_COLOR_F c = Direct2D::ColorToD2D(shadow.color, shadow.alpha / 255.0f);
                                D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
                                    0, 0, 0, 0,
                                    0, 0, 0, 0,
                                    0, 0, 0, 0,
                                    c.r * c.a, c.g * c.a, c.b * c.a, c.a,
                                    0, 0, 0, 0
                                );
                                pColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);

                                D2D1_POINT_2F offset = D2D1::Point2F(layoutRect.left + shadow.offsetX, layoutRect.top + shadow.offsetY);
                                context->DrawImage(pColorMatrixEffect.Get(), offset);
                            }
                        }
                    }
                }
            }

            // Draw main text
            context->DrawTextLayout(D2D1::Point2F(layoutX, layoutY), pLayout.Get(), pBrush.Get());
        }
    }
    
    // Restore transform
    context->SetTransform(originalTransform);
}

int TextElement::GetAutoWidth()
{
    if (m_Text.empty()) return 0;

    std::wstring fontFace = m_FontFace.empty() ? L"Arial" : m_FontFace;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
    if (!m_FontPath.empty()) {
        pCollection = FontManager::GetFontCollection(m_FontPath);
        if (pCollection) {
            UINT32 index;
            BOOL exists;
            if (FAILED(pCollection->FindFamilyName(fontFace.c_str(), &index, &exists)) || !exists) {
                pCollection = nullptr;
            }
        }
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        fontFace.c_str(), pCollection.Get(),
        (DWRITE_FONT_WEIGHT)m_FontWeight,
        m_Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, (float)m_FontSize, L"",
        pTextFormat.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    // For AutoWidth, we never wrap
    pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    std::wstring processedText = GetProcessedText();
    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        processedText.c_str(), (UINT32)processedText.length(), pTextFormat.Get(),
        10000.0f, 10000.0f, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    for (const auto& segment : m_Segments) {
        DWRITE_TEXT_RANGE range = { segment.startPos, segment.length };
        if (segment.style.fontWeight.has_value()) pLayout->SetFontWeight((DWRITE_FONT_WEIGHT)segment.style.fontWeight.value(), range);
        if (segment.style.italic.has_value()) pLayout->SetFontStyle(segment.style.italic.value() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
        if (segment.style.underline.has_value()) pLayout->SetUnderline(segment.style.underline.value(), range);
        if (segment.style.strikethrough.has_value()) pLayout->SetStrikethrough(segment.style.strikethrough.value(), range);
        if (segment.style.fontSize.has_value()) pLayout->SetFontSize((float)segment.style.fontSize.value(), range);
        if (segment.style.fontFace.has_value()) pLayout->SetFontFamilyName(segment.style.fontFace.value().c_str(), range);
    }

    if (m_LetterSpacing != 0.0f) {
        Microsoft::WRL::ComPtr<IDWriteTextLayout1> pLayout1;
        if (SUCCEEDED(pLayout.As(&pLayout1))) {
            DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
            pLayout1->SetCharacterSpacing(m_LetterSpacing, 0.0f, 0.0f, range);
        }
    }

    if (m_UnderLine) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetUnderline(TRUE, range);
    }

    if (m_StrikeThrough) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetStrikethrough(TRUE, range);
    }

    DWRITE_TEXT_METRICS metrics;
    pLayout->GetMetrics(&metrics);

    int contentW = (int)ceil(metrics.widthIncludingTrailingWhitespace);
    
    if (!m_WDefined && (m_ClipString == TEXT_CLIP_ON || m_ClipString == TEXT_CLIP_ELLIPSIS) && m_Width > 0)
    {
        if (contentW > m_Width) return m_Width;
    }
    return contentW;
}

int TextElement::GetAutoHeight()
{
    std::wstring processedText = GetProcessedText();
    if (processedText.empty()) return 0;

    std::wstring fontFace = m_FontFace.empty() ? L"Arial" : m_FontFace;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
    if (!m_FontPath.empty()) {
        pCollection = FontManager::GetFontCollection(m_FontPath);
        if (pCollection) {
            UINT32 index;
            BOOL exists;
            if (FAILED(pCollection->FindFamilyName(fontFace.c_str(), &index, &exists)) || !exists) {
                pCollection = nullptr;
            }
        }
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        fontFace.c_str(), pCollection.Get(),
        (DWRITE_FONT_WEIGHT)m_FontWeight,
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
            // GetWidth() already includes padding, but DirectWrite layout range needs content width
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
        processedText.c_str(), (UINT32)processedText.length(), pTextFormat.Get(),
        maxWidth, 10000.0f, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return 0;

    for (const auto& segment : m_Segments) {
        DWRITE_TEXT_RANGE range = { segment.startPos, segment.length };
        if (segment.style.fontWeight.has_value()) pLayout->SetFontWeight((DWRITE_FONT_WEIGHT)segment.style.fontWeight.value(), range);
        if (segment.style.italic.has_value()) pLayout->SetFontStyle(segment.style.italic.value() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
        if (segment.style.underline.has_value()) pLayout->SetUnderline(segment.style.underline.value(), range);
        if (segment.style.strikethrough.has_value()) pLayout->SetStrikethrough(segment.style.strikethrough.value(), range);
        if (segment.style.fontSize.has_value()) pLayout->SetFontSize((float)segment.style.fontSize.value(), range);
        if (segment.style.fontFace.has_value()) pLayout->SetFontFamilyName(segment.style.fontFace.value().c_str(), range);
    }

    if (m_LetterSpacing != 0.0f) {
        Microsoft::WRL::ComPtr<IDWriteTextLayout1> pLayout1;
        if (SUCCEEDED(pLayout.As(&pLayout1))) {
            DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
            pLayout1->SetCharacterSpacing(m_LetterSpacing, 0.0f, 0.0f, range);
        }
    }

    if (m_UnderLine) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetUnderline(TRUE, range);
    }

    if (m_StrikeThrough) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetStrikethrough(TRUE, range);
    }

    DWRITE_TEXT_METRICS metrics;
    pLayout->GetMetrics(&metrics);

    int contentH = (int)ceil(metrics.height);
    
    // Logging::Log(LogLevel::Debug, L"TextElement(%s): GetAutoHeight = %d (Font: %s, Size: %d, Wrap: %s, MaxWidth: %.0f)", 
    //     m_Id.c_str(), contentH, fontFace.c_str(), m_FontSize, wrap ? L"YES" : L"NO", maxWidth);

    if (!m_HDefined && (m_ClipString == TEXT_CLIP_ON || m_ClipString == TEXT_CLIP_ELLIPSIS) && m_Height > 0)
    {
        if (contentH > m_Height) return m_Height;
    }
    return contentH;
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
    if ((m_HasSolidColor && m_SolidAlpha > 0) || (m_SolidGradient.type != GRADIENT_NONE)) return true;

    // Use DirectWrite for precise hit testing
    std::wstring fontFace = m_FontFace.empty() ? L"Arial" : m_FontFace;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
    if (!m_FontPath.empty()) {
        pCollection = FontManager::GetFontCollection(m_FontPath);
        if (pCollection) {
            UINT32 index;
            BOOL exists;
            if (FAILED(pCollection->FindFamilyName(fontFace.c_str(), &index, &exists)) || !exists) {
                pCollection = nullptr;
            }
        }
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
    HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
        fontFace.c_str(), pCollection.Get(),
        (DWRITE_FONT_WEIGHT)m_FontWeight,
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

    std::wstring processedText = GetProcessedText();
    Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
    hr = Direct2D::GetWriteFactory()->CreateTextLayout(
        processedText.c_str(), (UINT32)processedText.length(), pTextFormat.Get(),
        layoutW, layoutH, pLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    for (const auto& segment : m_Segments) {
        DWRITE_TEXT_RANGE range = { segment.startPos, segment.length };
        if (segment.style.fontWeight.has_value()) pLayout->SetFontWeight((DWRITE_FONT_WEIGHT)segment.style.fontWeight.value(), range);
        if (segment.style.italic.has_value()) pLayout->SetFontStyle(segment.style.italic.value() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
        if (segment.style.underline.has_value()) pLayout->SetUnderline(segment.style.underline.value(), range);
        if (segment.style.strikethrough.has_value()) pLayout->SetStrikethrough(segment.style.strikethrough.value(), range);
        if (segment.style.fontSize.has_value()) pLayout->SetFontSize((float)segment.style.fontSize.value(), range);
        if (segment.style.fontFace.has_value()) pLayout->SetFontFamilyName(segment.style.fontFace.value().c_str(), range);
    }

    if (m_LetterSpacing != 0.0f) {
        Microsoft::WRL::ComPtr<IDWriteTextLayout1> pLayout1;
        if (SUCCEEDED(pLayout.As(&pLayout1))) {
            DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
            pLayout1->SetCharacterSpacing(m_LetterSpacing, 0.0f, 0.0f, range);
        }
    }

    if (m_UnderLine) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetUnderline(TRUE, range);
    }

    if (m_StrikeThrough) {
        DWRITE_TEXT_RANGE range = { 0, (UINT32)processedText.length() };
        pLayout->SetStrikethrough(TRUE, range);
    }

    // Get point relative to layout area
    float relX = (float)x - (bounds.X + m_PaddingLeft);
    float relY = (float)y - (bounds.Y + m_PaddingTop);

    BOOL isTrailingHit;
    BOOL isInside;
    DWRITE_HIT_TEST_METRICS hitMetrics;
    pLayout->HitTestPoint(relX, relY, &isTrailingHit, &isInside, &hitMetrics);

    return isInside;
}

std::wstring TextElement::GetProcessedText() const
{
    if (m_TextCase == TEXT_CASE_NORMAL) return m_CleanText;

    std::wstring processed = m_CleanText;
    if (m_TextCase == TEXT_CASE_UPPER) {
        std::transform(processed.begin(), processed.end(), processed.begin(), ::towupper);
    }
    else if (m_TextCase == TEXT_CASE_LOWER) {
        std::transform(processed.begin(), processed.end(), processed.begin(), ::towlower);
    }
    else if (m_TextCase == TEXT_CASE_CAPITALIZE) {
        bool nextUpper = true;
        for (size_t i = 0; i < processed.length(); ++i) {
            if (iswspace(processed[i])) {
                nextUpper = true;
            } else if (nextUpper) {
                processed[i] = towupper(processed[i]);
                nextUpper = false;
            } else {
                processed[i] = towlower(processed[i]);
            }
        }
    }
    else if (m_TextCase == TEXT_CASE_SENTENCE) {
        bool nextUpper = true;
        for (size_t i = 0; i < processed.length(); ++i) {
            if (nextUpper && !iswspace(processed[i])) {
                processed[i] = towupper(processed[i]);
                nextUpper = false;
            } else {
                processed[i] = towlower(processed[i]);
            }
        }
    }

    return processed;
}

void TextElement::ParseInlineStyles()
{
    m_CleanText.clear();
    m_Segments.clear();
    
    std::vector<TextSegmentStyle> styleStack;
    TextSegmentStyle currentStyle;
    bool forceNextUpper = false;

    for (size_t i = 0; i < m_Text.length(); ++i)
    {
        if (m_Text[i] == L'<')
        {
            size_t endTag = m_Text.find(L'>', i);
            if (endTag != std::wstring::npos)
            {
                std::wstring tag = m_Text.substr(i + 1, endTag - i - 1);
                bool isClosing = (!tag.empty() && tag[0] == L'/');
                if (isClosing) tag = tag.substr(1);

                if (isClosing)
                {
                    if (!styleStack.empty())
                    {
                        currentStyle = styleStack.back();
                        styleStack.pop_back();
                        forceNextUpper = false;
                    }
                }
                else
                {
                    styleStack.push_back(currentStyle);
                    
                    if (tag == L"b") currentStyle.fontWeight = 700;
                    else if (tag == L"i") currentStyle.italic = true;
                    else if (tag == L"u") currentStyle.underline = true;
                    else if (tag == L"s") currentStyle.strikethrough = true;
                    else if (tag.find(L"color=") == 0)
                    {
                        std::wstring colorStr = tag.substr(6);
                        
                        GradientInfo gi;
                        if (Utils::ParseGradientString(colorStr, gi))
                        {
                            currentStyle.gradient = gi;
                            currentStyle.color.reset();
                        }
                        else
                        {
                            COLORREF color;
                            BYTE alpha;
                            if (ColorUtil::ParseRGBA(colorStr, color, alpha))
                            {
                                currentStyle.color = color;
                                currentStyle.alpha = alpha;
                                currentStyle.gradient.reset();
                            }
                        }
                    }
                    else if (tag.find(L"size=") == 0)
                    {
                        try { currentStyle.fontSize = std::stoi(tag.substr(5)); } catch(...) {}
                    }
                    else if (tag.find(L"font=") == 0)
                    {
                        currentStyle.fontFace = tag.substr(5);
                    }
                    else if (tag.find(L"case=") == 0)
                    {
                        std::wstring c = tag.substr(5);
                        if (c == L"upper") currentStyle.textCase = TEXT_CASE_UPPER;
                        else if (c == L"lower") currentStyle.textCase = TEXT_CASE_LOWER;
                        else if (c == L"capitalize") currentStyle.textCase = TEXT_CASE_CAPITALIZE;
                        else if (c == L"sentence") currentStyle.textCase = TEXT_CASE_SENTENCE;
                        else if (c == L"normal") currentStyle.textCase = TEXT_CASE_NORMAL;
                        
                        if (currentStyle.textCase == TEXT_CASE_SENTENCE) forceNextUpper = true;
                    }
                }

                i = endTag;
                continue;
            }
        }

        wchar_t c = m_Text[i];
        TextCase activeCase = currentStyle.textCase.value_or(TEXT_CASE_NORMAL);
        if (activeCase == TEXT_CASE_UPPER) c = towupper(c);
        else if (activeCase == TEXT_CASE_LOWER) c = towlower(c);
        else if (activeCase == TEXT_CASE_CAPITALIZE) {
            bool isStart = m_CleanText.empty() || iswspace(m_CleanText.back());
            if (isStart) c = towupper(c);
            else c = towlower(c);
        }
        else if (activeCase == TEXT_CASE_SENTENCE) {
            bool isStart = m_CleanText.empty() || forceNextUpper;
            if (!isStart) {
                size_t j = m_CleanText.length();
                isStart = true;
                while (j > 0) {
                    wchar_t prev = m_CleanText[j - 1];
                    if (iswalpha(prev)) { isStart = false; break; }
                    if (prev == L'.' || prev == L'!' || prev == L'?') { isStart = true; break; }
                    j--;
                }
            }
            if (iswalpha(c)) {
                if (isStart) c = towupper(c);
                else c = towlower(c);
                forceNextUpper = false;
            }
        }

        m_CleanText += c;
        
        UINT32 currentPos = (UINT32)m_CleanText.length() - 1;
        if (!m_Segments.empty() && m_Segments.back().startPos + m_Segments.back().length == currentPos)
        {
            const auto& lastStyle = m_Segments.back().style;
            bool same = (lastStyle.fontWeight == currentStyle.fontWeight &&
                         lastStyle.italic == currentStyle.italic &&
                         lastStyle.underline == currentStyle.underline &&
                         lastStyle.strikethrough == currentStyle.strikethrough &&
                         lastStyle.color == currentStyle.color &&
                         lastStyle.alpha == currentStyle.alpha &&
                         lastStyle.fontSize == currentStyle.fontSize &&
                         lastStyle.fontFace == currentStyle.fontFace &&
                         lastStyle.textCase == currentStyle.textCase &&
                         lastStyle.gradient.has_value() == currentStyle.gradient.has_value() && // Simple check for gradient equality (can be improved)
                         (!lastStyle.gradient.has_value() || (lastStyle.gradient->type == currentStyle.gradient->type && lastStyle.gradient->angle == currentStyle.gradient->angle && lastStyle.gradient->stops.size() == currentStyle.gradient->stops.size())));
            
            if (same)
            {
                m_Segments.back().length++;
                m_Segments.back().text += m_Text[i];
                continue;
            }
        }

        TextSegment seg;
        seg.text = m_Text[i];
        seg.style = currentStyle;
        seg.startPos = currentPos;
        seg.length = 1;
        m_Segments.push_back(seg);
    }
}
