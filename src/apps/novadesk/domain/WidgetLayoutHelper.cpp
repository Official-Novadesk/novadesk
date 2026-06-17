/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WidgetLayoutHelper.h"
#include "Widget.h"
#include "../shared/Logging.h"
#include "../render/ElementLayoutBox.h"
#include "../render/ShapeElement.h"
#include "../render/FlexLayoutEngine.h"
#include "../render/Direct2DHelper.h"

// Helper to access Widget's layout configs
std::unordered_map<std::wstring, WidgetLayoutHelper::LayoutConfig>& 
WidgetLayoutHelper::GetLayoutConfigs(Widget& widget)
{
    return widget.m_LayoutConfigs;
}

const std::unordered_map<std::wstring, WidgetLayoutHelper::LayoutConfig>& 
WidgetLayoutHelper::GetLayoutConfigs(const Widget& widget)
{
    return widget.m_LayoutConfigs;
}

void WidgetLayoutHelper::SetLayoutConfig(Widget& widget, const std::wstring& id, const LayoutConfig& config)
{
    if (id.empty())
        return;
    
    GetLayoutConfigs(widget)[id] = config;
    ReflowLayout(widget, id);
}

bool WidgetLayoutHelper::TryGetLayoutConfig(const Widget& widget, const std::wstring& id, LayoutConfig& config)
{
    const auto& configs = GetLayoutConfigs(widget);
    auto it = configs.find(id);
    if (it == configs.end())
        return false;
    
    config = it->second;
    return true;
}

bool WidgetLayoutHelper::IsLayoutContainer(const Widget& widget, const std::wstring& id)
{
    if (id.empty())
        return false;
    
    const auto& configs = GetLayoutConfigs(widget);
    return configs.find(id) != configs.end();
}

void WidgetLayoutHelper::ReflowLayout(Widget& widget, const std::wstring& id)
{
    if (id.empty())
        return;
    
    Element* container = widget.FindElementById(id);
    if (!container)
        return;
    
    ApplyLayoutForContainer(widget, container);
}

void WidgetLayoutHelper::ApplyLayoutForContainer(const Widget& widget, Element* container)
{
    if (!container)
        return;

    const auto& configs = GetLayoutConfigs(widget);
    auto cfgIt = configs.find(container->GetId());
    if (cfgIt == configs.end())
        return;

    const LayoutConfig& cfg = cfgIt->second;
    const auto& items = container->GetContainerItems();
    if (items.empty())
        return;

    // Delegate to FlexLayoutEngine
    FlexLayoutEngine::ApplyLayout(container, cfg);
}

void WidgetLayoutHelper::UpdateContainerForElement(Widget& widget, Element* element, const std::wstring& newContainerId)
{
    if (!element)
        return;

    Element* currentContainer = element->GetContainer();
    
    // Remove from container
    if (newContainerId.empty())
    {
        if (currentContainer)
        {
            currentContainer->RemoveContainerItem(element);
            if (IsLayoutContainer(widget, currentContainer->GetId()))
            {
                ApplyLayoutForContainer(widget, currentContainer);
            }
            element->SetContainer(nullptr);
        }
        element->SetContainerId(L"");
        return;
    }

    // Validate self-reference
    if (element->GetId() == newContainerId)
    {
        Logging::Log(LogLevel::Error, L"Container cannot self-reference: %s", newContainerId.c_str());
        return;
    }

    // Find new container
    Element* newContainer = widget.FindElementById(newContainerId);
    if (!newContainer)
    {
        Logging::Log(LogLevel::Error, L"Invalid container: %s", newContainerId.c_str());
        return;
    }

    // Validate: containers cannot be contained (except layout containers)
    if (element->IsContainer())
    {
        Logging::Log(LogLevel::Error, L"Container cannot be contained: %s", element->GetId().c_str());
        return;
    }

    // Allow nested layout containers (LayoutBox tree). Keep legacy protection
    // for non-layout containers to avoid behavior changes in older widgets.
    if (newContainer->IsContained() && !IsLayoutContainer(widget, newContainer->GetId()))
    {
        Logging::Log(LogLevel::Error, L"Nested containers are not allowed: %s", newContainerId.c_str());
        return;
    }

    // Validate: no cycles
    if (WouldCreateContainerCycle(element, newContainer))
    {
        Logging::Log(LogLevel::Error, L"Container cycle detected for: %s", newContainerId.c_str());
        return;
    }

    // Update container assignment
    if (currentContainer != newContainer)
    {
        if (currentContainer)
        {
            currentContainer->RemoveContainerItem(element);
            if (IsLayoutContainer(widget, currentContainer->GetId()))
            {
                ApplyLayoutForContainer(widget, currentContainer);
            }
        }
        newContainer->AddContainerItem(element);
        element->SetContainer(newContainer);
    }

    element->SetContainerId(newContainerId);
    if (IsLayoutContainer(widget, newContainer->GetId()))
    {
        ApplyLayoutForContainer(widget, newContainer);
    }
}

bool WidgetLayoutHelper::WouldCreateContainerCycle(const Element* element, const Element* container)
{
    if (!element || !container)
        return false;
    
    if (element == container)
        return true;

    const Element* cursor = container;
    while (cursor)
    {
        if (cursor == element)
            return true;
        cursor = cursor->GetContainer();
    }
    
    return false;
}

void WidgetLayoutHelper::RenderContainerChildren(const Widget& widget, Element* container)
{
    if (!container || !container->IsContainer())
        return;

    ID2D1DeviceContext* context = widget.GetDeviceContext();
    if (!context)
        return;

    GfxRect bounds = container->GetBounds();
    D2D1_RECT_F clipRect = D2D1::RectF(
        (float)bounds.X, (float)bounds.Y,
        (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height));

    Microsoft::WRL::ComPtr<ID2D1Layer> layer;
    context->CreateLayer(layer.GetAddressOf());
    if (!layer)
        return;

    Microsoft::WRL::ComPtr<ID2D1BitmapBrush> opacityBrush;
    bool hasOpacityMask = false;
    const bool isLayoutContainer = IsLayoutContainer(widget, container->GetId());

    // Layout containers are structural wrappers; they should clip children,
    // but must not apply alpha masking from their own fill/stroke.
    if (!isLayoutContainer && bounds.Width > 0 && bounds.Height > 0)
    {
        Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> maskTarget;
        HRESULT hr = context->CreateCompatibleRenderTarget(
            D2D1::SizeF((FLOAT)bounds.Width, (FLOAT)bounds.Height),
            maskTarget.GetAddressOf());
        
        if (SUCCEEDED(hr) && maskTarget)
        {
            maskTarget->BeginDraw();
            maskTarget->Clear(D2D1::ColorF(0, 0.0f));

            if (ShapeElement* shapeContainer = dynamic_cast<ShapeElement*>(container))
            {
                Microsoft::WRL::ComPtr<ID2D1Geometry> geom;
                ID2D1Factory1* factory = Direct2D::GetFactory();
                if (factory && shapeContainer->CreateGeometry(factory, geom))
                {
                    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
                    if (Direct2D::CreateSolidBrush(maskTarget.Get(), RGB(255, 255, 255), 1.0f, &brush))
                    {
                        maskTarget->FillGeometry(geom.Get(), brush.Get());
                    }
                }
            }

            maskTarget->EndDraw();

            Microsoft::WRL::ComPtr<ID2D1Bitmap> maskBitmap;
            maskTarget->GetBitmap(&maskBitmap);
            if (maskBitmap)
            {
                Microsoft::WRL::ComPtr<ID2D1BitmapBrush> brush1;
                HRESULT brushResult = context->CreateBitmapBrush(maskBitmap.Get(), &brush1);
                if (SUCCEEDED(brushResult) && brush1)
                {
                    brush1->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)bounds.X, (FLOAT)bounds.Y));
                    opacityBrush = brush1;
                    hasOpacityMask = true;
                }
            }
        }
    }

    // Push layer with optional opacity mask
    if (hasOpacityMask)
    {
        context->PushLayer(
            D2D1::LayerParameters(clipRect, nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                 D2D1::Matrix3x2F::Identity(), 1.0f, opacityBrush.Get()),
            layer.Get());
    }
    else
    {
        context->PushLayer(D2D1::LayerParameters(clipRect), layer.Get());
    }

    // Apply translation for container offset
    D2D1_MATRIX_3X2_F originalTransform;
    context->GetTransform(&originalTransform);
    D2D1_MATRIX_3X2_F translate = D2D1::Matrix3x2F::Translation((float)bounds.X, (float)bounds.Y);
    context->SetTransform(translate * originalTransform);

    // Render all children recursively
    for (Element* child : container->GetContainerItems())
    {
        if (!child || !child->IsVisible())
            continue;
        
        child->Render(context);
        
        if (child->IsContainer())
        {
            RenderContainerChildren(widget, child);
        }
    }

    // Restore transform and pop layer
    context->SetTransform(originalTransform);
    context->PopLayer();
}

bool WidgetLayoutHelper::HitTestContainerChildren(Element* container, int x, int y, Element*& outElement)
{
    // Note: This overload exists for backwards compatibility
    // In practice, we need the Widget reference, so we pass through detailed version
    // The detailed version will handle the container hit testing logic
    Element* outActionElement = nullptr;
    Element* outMouseActionElement = nullptr;
    Element* outToolTipElement = nullptr;
    
    // For the simple version, we create a minimal hit test without Widget-specific features
    // This is safe because the basic hit testing doesn't require Widget state
    if (!container || !container->IsContainer() || !container->IsVisible())
        return false;

    GfxRect bounds = container->GetBounds();
    if (x < bounds.X || x >= bounds.X + bounds.Width ||
        y < bounds.Y || y >= bounds.Y + bounds.Height)
        return false;

    if (ShapeElement* shapeContainer = dynamic_cast<ShapeElement*>(container))
    {
        if (!shapeContainer->HitTest(x, y))
            return false;
    }

    int localX = x - bounds.X;
    int localY = y - bounds.Y;
    bool foundAny = false;

    const auto& items = container->GetContainerItems();
    for (auto it = items.rbegin(); it != items.rend(); ++it)
    {
        Element* child = *it;
        if (!child || !child->IsVisible())
            continue;

        if (child->IsContainer())
        {
            if (HitTestContainerChildren(child, localX, localY, outElement))
                return true;
            continue;
        }

        if (!child->HitTest(localX, localY))
            continue;

        foundAny = true;
        if (!outElement)
            outElement = child;
    }

    return foundAny;
}

bool WidgetLayoutHelper::HitTestContainerChildrenDetailed(
    const Widget& widget,
    Element* container,
    int x, int y,
    UINT message,
    WPARAM wParam,
    Element*& outHitElement,
    Element*& outActionElement,
    Element*& outMouseActionElement,
    Element*& outToolTipElement)
{
    if (!container || !container->IsContainer() || !container->IsVisible())
        return false;

    GfxRect bounds = container->GetBounds();
    
    // Check if point is within container bounds
    if (x < bounds.X || x >= bounds.X + bounds.Width ||
        y < bounds.Y || y >= bounds.Y + bounds.Height)
    {
        return false;
    }

    // For shape containers, check shape geometry
    if (ShapeElement* shapeContainer = dynamic_cast<ShapeElement*>(container))
    {
        if (!shapeContainer->HitTest(x, y))
        {
            return false;
        }
    }

    // Test children in reverse order (top to bottom)
    int localX = x - bounds.X;
    int localY = y - bounds.Y;
    bool foundAny = false;

    const auto& items = container->GetContainerItems();
    for (auto it = items.rbegin(); it != items.rend(); ++it)
    {
        Element* child = *it;
        if (!child || !child->IsVisible())
            continue;

        // Recursively test child containers
        if (child->IsContainer())
        {
            if (HitTestContainerChildrenDetailed(
                    widget, child,
                    x, y, message, wParam,
                    outHitElement, outActionElement, outMouseActionElement, outToolTipElement))
            {
                return true;
            }
            continue;
        }

        // Test child element
        if (!child->HitTest(localX, localY))
            continue;

        // Found hit - record results
        foundAny = true;
        if (!outHitElement)
            outHitElement = child;
        if (!outActionElement && child->HasAction(message, wParam))
            outActionElement = child;
        if (!outMouseActionElement && child->HasMouseAction())
            outMouseActionElement = child;
        if (!outToolTipElement && child->HasToolTip())
            outToolTipElement = child;
    }

    return foundAny;
}
