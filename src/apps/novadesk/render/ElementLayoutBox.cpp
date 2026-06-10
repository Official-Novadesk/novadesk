/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ElementLayoutBox.h"
#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include <algorithm>
#include <cmath>
#include <d2d1effects.h>
#include <vector>

ElementLayoutBox::ElementLayoutBox(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height, ELEMENT_LAYOUT_BOX)
{
}

int ElementLayoutBox::GetAutoWidth()
{
    // If width is explicitly defined, use it
    if (m_WDefined)
        return m_Width;
    
    // Calculate auto width based on children and layout direction
    // NOTE: Do NOT include padding here - Element::GetWidth() will add it
    const auto& children = GetContainerItems();
    if (children.empty())
        return 0; // Element::GetWidth() will add padding
    
    // Parse flex direction to determine layout
    std::wstring flexDir = m_FlexDirection;
    std::transform(flexDir.begin(), flexDir.end(), flexDir.begin(), ::towlower);
    const bool isHorizontal = (flexDir == L"row" || flexDir == L"rowreverse");
    
    int contentWidth = 0;
    
    if (isHorizontal)
    {
        // Horizontal layout (row): sum all children widths + gaps
        for (size_t i = 0; i < children.size(); ++i)
        {
            Element* child = children[i];
            if (!child) continue;
            contentWidth += child->GetWidth();
            
            // Add gap between items (not after last item)
            if (i < children.size() - 1)
                contentWidth += m_LayoutGap;
        }
    }
    else
    {
        // Vertical layout (column): use maximum child width
        for (Element* child : children)
        {
            if (!child) continue;
            contentWidth = std::max(contentWidth, child->GetWidth());
        }
    }
    
    // Return content width WITHOUT padding (Element::GetWidth() adds it)
    return contentWidth;
}

int ElementLayoutBox::GetAutoHeight()
{
    // If height is explicitly defined, use it
    if (m_HDefined)
        return m_Height;
    
    // Calculate auto height based on children and layout direction
    // NOTE: Do NOT include padding here - Element::GetHeight() will add it
    const auto& children = GetContainerItems();
    if (children.empty())
        return 0; // Element::GetHeight() will add padding
    
    // Parse flex direction to determine layout
    std::wstring flexDir = m_FlexDirection;
    std::transform(flexDir.begin(), flexDir.end(), flexDir.begin(), ::towlower);
    const bool isHorizontal = (flexDir == L"row" || flexDir == L"rowreverse");
    
    int contentHeight = 0;
    
    if (isHorizontal)
    {
        // Horizontal layout (row): use maximum child height
        for (Element* child : children)
        {
            if (!child) continue;
            contentHeight = std::max(contentHeight, child->GetHeight());
        }
    }
    else
    {
        // Vertical layout (column): sum all children heights + gaps
        for (size_t i = 0; i < children.size(); ++i)
        {
            Element* child = children[i];
            if (!child) continue;
            contentHeight += child->GetHeight();
            
            // Add gap between items (not after last item)
            if (i < children.size() - 1)
                contentHeight += m_LayoutGap;
        }
    }
    
    // Return content height WITHOUT padding (Element::GetHeight() adds it)
    return contentHeight;
}

bool ElementLayoutBox::HitTestLocal(const D2D1_POINT_2F& point)
{
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory)
        return false;
    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry))
        return false;
    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0)
    {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit)
            return true;
    }
    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0)
    {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit)
            return true;
    }
    return false;
}

bool ElementLayoutBox::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    if (!factory)
        return false;
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded;
    D2D1_ROUNDED_RECT rect;
    
    // Use Element::GetWidth() and GetHeight() which handle auto-sizing and padding
    const int totalWidth = const_cast<ElementLayoutBox*>(this)->GetWidth();
    const int totalHeight = const_cast<ElementLayoutBox*>(this)->GetHeight();
    
    rect.rect = D2D1::RectF(
        (float)m_X, 
        (float)m_Y, 
        (float)(m_X + totalWidth), 
        (float)(m_Y + totalHeight));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;
    if (FAILED(factory->CreateRoundedRectangleGeometry(rect, rounded.GetAddressOf())))
        return false;
    geometry = rounded;
    return true;
}

void ElementLayoutBox::Render(ID2D1DeviceContext* context)
{
    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);
    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
    Microsoft::WRL::ComPtr<ID2D1Brush> strokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
    TryCreateStrokeBrush(context, strokeBrush);
    TryCreateFillBrush(context, fillBrush);
    D2D1_ROUNDED_RECT rect;
    
    // Use Element::GetWidth() and GetHeight() which handle auto-sizing and padding
    const int totalWidth = GetWidth();
    const int totalHeight = GetHeight();
    
    // Logging::Log(LogLevel::Debug, L"[AUTO-SIZE] Render '%s': WDefined=%d HDefined=%d, total W=%d H=%d (includes padding L=%d T=%d R=%d B=%d)",
    //     m_Id.c_str(), m_WDefined, m_HDefined, totalWidth, totalHeight,
    //     m_PaddingLeft, m_PaddingTop, m_PaddingRight, m_PaddingBottom);
    
    rect.rect = D2D1::RectF(
        (float)m_X, 
        (float)m_Y, 
        (float)(m_X + totalWidth), 
        (float)(m_Y + totalHeight));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;
    for (const auto& shadow : m_BoxShadows)
    {
        if (!shadow.inset)
            RenderSingleShadow(context, rect, shadow);
    }
    D2D1_ROUNDED_RECT fillRect = rect;
    if (fillBrush)
    {
        if (fillRect.rect.right > fillRect.rect.left && fillRect.rect.bottom > fillRect.rect.top)
            context->FillRoundedRectangle(fillRect, fillBrush.Get());
    }
    for (const auto& shadow : m_BoxShadows)
    {
        if (shadow.inset && fillRect.rect.right > fillRect.rect.left && fillRect.rect.bottom > fillRect.rect.top)
            RenderSingleShadow(context, fillRect, shadow);
    }
    if (strokeBrush)
    {
        BoxBorderPaint::PaintForElement(context, rect, BuildBorderPaintParams(), strokeBrush.Get());
    }
    
    // Render list marker if this is a list item
    RenderListMarker(context);
    
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

void ElementLayoutBox::RenderSingleShadow(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& baseRect, const BoxShadow& shadow)
{
    if (!context || shadow.alpha == 0)
        return;
    const float width = baseRect.rect.right - baseRect.rect.left;
    const float height = baseRect.rect.bottom - baseRect.rect.top;
    const float pad = std::max(2.0f, std::ceil(std::max(0.0f, shadow.blur) + std::fabs(shadow.spread) + 4.0f));
    D2D1_ROUNDED_RECT shadowRect = D2D1::RoundedRect(
        D2D1::RectF(pad, pad, pad + width, pad + height),
        baseRect.radiusX,
        baseRect.radiusY);
    if (shadow.spread != 0.0f)
    {
        shadowRect.rect.left -= shadow.spread;
        shadowRect.rect.top -= shadow.spread;
        shadowRect.rect.right += shadow.spread;
        shadowRect.rect.bottom += shadow.spread;
        shadowRect.radiusX = std::max(0.0f, baseRect.radiusX + shadow.spread);
        shadowRect.radiusY = std::max(0.0f, baseRect.radiusY + shadow.spread);
        const float minExtent = 1.0f;
        if ((shadowRect.rect.right - shadowRect.rect.left) < minExtent)
        {
            const float cx = (shadowRect.rect.left + shadowRect.rect.right) * 0.5f;
            shadowRect.rect.left = cx - (minExtent * 0.5f);
            shadowRect.rect.right = cx + (minExtent * 0.5f);
        }
        if ((shadowRect.rect.bottom - shadowRect.rect.top) < minExtent)
        {
            const float cy = (shadowRect.rect.top + shadowRect.rect.bottom) * 0.5f;
            shadowRect.rect.top = cy - (minExtent * 0.5f);
            shadowRect.rect.bottom = cy + (minExtent * 0.5f);
        }
    }
    Microsoft::WRL::ComPtr<ID2D1CommandList> commandList;
    if (FAILED(context->CreateCommandList(&commandList)))
        return;
    Microsoft::WRL::ComPtr<ID2D1Device> device;
    context->GetDevice(&device);
    if (!device)
        return;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> recordingContext;
    if (FAILED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &recordingContext)))
        return;
    recordingContext->SetTarget(commandList.Get());
    recordingContext->BeginDraw();
    recordingContext->Clear(D2D1::ColorF(0, 0.0f));
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    if (Direct2D::CreateSolidBrush(recordingContext.Get(), RGB(255, 255, 255), 1.0f, &whiteBrush))
    {
        if (shadow.inset)
        {
            float insetThickness = std::max(
                1.0f,
                (shadow.blur * 1.15f) +
                (std::max(0.0f, shadow.spread) * 1.6f) +
                (std::max(std::fabs(shadow.x), std::fabs(shadow.y)) * 0.45f));
            if (shadow.spread < 0.0f)
            {
                insetThickness = std::max(1.0f, insetThickness + shadow.spread);
            }
            recordingContext->DrawRoundedRectangle(shadowRect, whiteBrush.Get(), insetThickness);
        }
        else
        {
            recordingContext->FillRoundedRectangle(shadowRect, whiteBrush.Get());
        }
    }
    recordingContext->EndDraw();
    commandList->Close();
    Microsoft::WRL::ComPtr<ID2D1Effect> shadowEffect;
    Microsoft::WRL::ComPtr<ID2D1Effect> colorMatrixEffect;
    if (FAILED(context->CreateEffect(CLSID_D2D1Shadow, &shadowEffect)) ||
        FAILED(context->CreateEffect(CLSID_D2D1ColorMatrix, &colorMatrixEffect)))
    {
        return;
    }
    shadowEffect->SetInput(0, commandList.Get());
    shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, std::max(0.0f, shadow.blur));
    colorMatrixEffect->SetInputEffect(0, shadowEffect.Get());
    D2D1_COLOR_F c = Direct2D::ColorToD2D(shadow.color, shadow.alpha / 255.0f);
    D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        c.r * c.a, c.g * c.a, c.b * c.a, c.a,
        0, 0, 0, 0);
    colorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
    const D2D1_POINT_2F shadowOffset = D2D1::Point2F(
        baseRect.rect.left + shadow.x - pad,
        baseRect.rect.top + shadow.y - pad);
    if (shadow.inset)
    {
        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> clipGeometry;
        ID2D1Factory1* factory = Direct2D::GetFactory();
        if (!factory || FAILED(factory->CreateRoundedRectangleGeometry(baseRect, &clipGeometry)))
            return;
        D2D1_LAYER_PARAMETERS1 layerParams = D2D1::LayerParameters1(
            baseRect.rect,
            clipGeometry.Get(),
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS1_NONE);
        context->PushLayer(layerParams, nullptr);
        context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
        context->PopLayer();
        return;
    }
    context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
}

BoxBorderPaintParams ElementLayoutBox::BuildBorderPaintParams() const
{
    BoxBorderPaintParams params{};
    params.position = BoxBorder::Position::Inside;
    params.styleTop = m_BorderStyleTop;
    params.styleRight = m_BorderStyleRight;
    params.styleBottom = m_BorderStyleBottom;
    params.styleLeft = m_BorderStyleLeft;
    params.elementRadiusX = m_RadiusX;
    params.elementRadiusY = m_RadiusY;
    params.strokeWidth = m_StrokeWidth;
    params.strokeColor = m_StrokeColor;
    params.strokeAlpha = m_StrokeAlpha;
    params.strokeStartCap = m_StrokeStartCap;
    params.strokeEndCap = m_StrokeEndCap;
    params.strokeDashCap = m_StrokeDashCap;
    params.strokeLineJoin = m_StrokeLineJoin;
    params.strokeDashOffset = m_StrokeDashOffset;
    return params;
}

void ElementLayoutBox::RenderTextMarker(
    ID2D1DeviceContext* context,
    const std::wstring& text,
    float markerCenterX, float markerCenterY, float markerSize,
    ID2D1SolidColorBrush* brush)
{
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory.GetAddressOf()))))
        return;

    if (FAILED(writeFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        markerSize * 2.0f,
        L"en-us",
        textFormat.GetAddressOf())))
        return;

    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    const D2D1_RECT_F textRect = D2D1::RectF(
        markerCenterX - 50.0f,
        markerCenterY - (markerSize * 1.5f),
        markerCenterX,
        markerCenterY + (markerSize * 1.5f)
    );

    context->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        textFormat.Get(),
        textRect,
        brush
    );
}

void ElementLayoutBox::RenderListMarker(ID2D1DeviceContext* context)
{
    // Only render marker for list items
    if (m_DisplayType != DisplayType::ListItem || !context)
        return;
    
    // Don't render if marker type is None
    if (m_ListMarker.type == ListStyleType::None)
        return;
    
    // Check if this listitem has children
    const auto& children = GetContainerItems();
    if (children.empty())
        return;  // No children, no markers to render
    
    // Logging::Log(LogLevel::Debug, L"[LIST-MARKER] Rendering markers for listitem '%s' with %d children", 
    //     m_Id.c_str(), static_cast<int>(children.size()));
    
    // Auto-calculate starting index based on sibling position
    int startingIndex = 1;
    Element* container = GetContainer();
    if (container)
    {
        const auto& siblings = container->GetContainerItems();
        for (Element* sibling : siblings)
        {
            if (sibling == this)
                break;  // Found ourselves, stop counting
            
            // Count only list items with same list style type
            ElementLayoutBox* siblingBox = dynamic_cast<ElementLayoutBox*>(sibling);
            if (siblingBox && 
                siblingBox->GetDisplayType() == DisplayType::ListItem &&
                siblingBox->GetListStyleType() == m_ListMarker.type)
            {
                // Add the number of children in the sibling (each child gets a marker)
                const auto& siblingChildren = siblingBox->GetContainerItems();
                startingIndex += std::max(1, static_cast<int>(siblingChildren.size()));
            }
        }
    }

    // Helper lambdas for converting an index to a marker string
    auto toRoman = [](int num, bool upper) -> std::wstring
    {
        if (num <= 0 || num > 3999)
            return L"?";
        struct Pair { int value; const wchar_t* upper; const wchar_t* lower; };
        const Pair pairs[] = {
            {1000,L"M",L"m"},{900,L"CM",L"cm"},{500,L"D",L"d"},{400,L"CD",L"cd"},
            {100,L"C",L"c"},{90,L"XC",L"xc"},{50,L"L",L"l"},{40,L"XL",L"xl"},
            {10,L"X",L"x"},{9,L"IX",L"ix"},{5,L"V",L"v"},{4,L"IV",L"iv"},{1,L"I",L"i"}
        };
        std::wstring result;
        for (const auto& p : pairs)
        {
            while (num >= p.value)
            {
                result += upper ? p.upper : p.lower;
                num -= p.value;
            }
        }
        return result;
    };

    auto toAlpha = [](int num, bool upper) -> std::wstring
    {
        if (num <= 0)
            return L"?";
        std::wstring result;
        while (num > 0)
        {
            --num;  // Make 0-indexed
            const wchar_t ch = upper
                ? static_cast<wchar_t>(L'A' + (num % 26))
                : static_cast<wchar_t>(L'a' + (num % 26));
            result = ch + result;
            num /= 26;
        }
        return result;
    };

    // Render marker for each child element
    int childIndex = 0;
    for (Element* child : children)
    {
        if (!child || !child->IsVisible())
            continue;
        
        const int displayIndex = startingIndex + childIndex;
        
        // Logging::Log(LogLevel::Debug, L"[LIST-MARKER] Child %d: pos(%d,%d) index=%d", 
        //     childIndex, child->GetX(), child->GetY(), displayIndex);
        
        // Create brush for the marker
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> markerBrush;
        if (!Direct2D::CreateSolidBrush(context, m_ListMarker.color, m_ListMarker.alpha / 255.0f, &markerBrush))
            continue;
        
        // Calculate marker position
        // Children have RELATIVE positions, so we need to add the parent's position
        // to get the absolute screen position
        const float childAbsoluteX = (float)(m_X + child->GetX());
        const float childAbsoluteY = (float)(m_Y + child->GetY());
        const float markerCenterX = childAbsoluteX + m_ListMarker.offsetX;
        const float markerCenterY = childAbsoluteY + (m_ListMarker.size * 1.5f);
        const float markerSize = m_ListMarker.size;
        
        // Logging::Log(LogLevel::Debug, L"[LIST-MARKER] Marker at (%.1f, %.1f) size=%.1f type=%d", 
        //     markerCenterX, markerCenterY, markerSize, static_cast<int>(m_ListMarker.type));
        
        // Render different marker types
        switch (m_ListMarker.type)
        {
            case ListStyleType::Disc:
            {
                // Filled circle (•)
                D2D1_ELLIPSE disc = D2D1::Ellipse(
                    D2D1::Point2F(markerCenterX, markerCenterY),
                    markerSize / 2.0f,
                    markerSize / 2.0f
                );
                context->FillEllipse(disc, markerBrush.Get());
                break;
            }
            
            case ListStyleType::Circle:
            {
                // Hollow circle (○)
                D2D1_ELLIPSE circle = D2D1::Ellipse(
                    D2D1::Point2F(markerCenterX, markerCenterY),
                    markerSize / 2.0f,
                    markerSize / 2.0f
                );
                context->DrawEllipse(circle, markerBrush.Get(), 1.0f);
                break;
            }
            
            case ListStyleType::Square:
            {
                // Filled square (■)
                D2D1_RECT_F square = D2D1::RectF(
                    markerCenterX - (markerSize / 2.0f),
                    markerCenterY - (markerSize / 2.0f),
                    markerCenterX + (markerSize / 2.0f),
                    markerCenterY + (markerSize / 2.0f)
                );
                context->FillRectangle(square, markerBrush.Get());
                break;
            }
            
            case ListStyleType::UpperRoman:
                RenderTextMarker(context, toRoman(displayIndex, true) + L".",
                    markerCenterX, markerCenterY, markerSize, markerBrush.Get());
                break;
            
            case ListStyleType::LowerRoman:
                RenderTextMarker(context, toRoman(displayIndex, false) + L".",
                    markerCenterX, markerCenterY, markerSize, markerBrush.Get());
                break;

            case ListStyleType::Decimal:
                RenderTextMarker(context, std::to_wstring(displayIndex) + L".",
                    markerCenterX, markerCenterY, markerSize, markerBrush.Get());
                break;
            
            case ListStyleType::LowerAlpha:
                RenderTextMarker(context, toAlpha(displayIndex, false) + L".",
                    markerCenterX, markerCenterY, markerSize, markerBrush.Get());
                break;

            case ListStyleType::UpperAlpha:
                RenderTextMarker(context, toAlpha(displayIndex, true) + L".",
                    markerCenterX, markerCenterY, markerSize, markerBrush.Get());
                break;
            
            case ListStyleType::None:
            default:
                // Do nothing
                break;
        }
        
        childIndex++;
    }
}
