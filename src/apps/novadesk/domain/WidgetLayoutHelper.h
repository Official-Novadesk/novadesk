/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGET_LAYOUT_HELPER_H__
#define __NOVADESK_WIDGET_LAYOUT_HELPER_H__

#include <string>
#include <unordered_map>
#include "../render/Element.h"
#include "../render/FlexLayoutEngine.h"

// Forward declaration
class Widget;

/**
 * WidgetLayoutHelper - Manages layout container operations for Widget
 * 
 * This helper class extracts layout container management logic from Widget.cpp
 * to improve code organization and maintainability.
 * 
 * Responsibilities:
 * - Layout configuration storage and retrieval
 * - Container hierarchy validation
 * - Layout reflow orchestration
 * - Container rendering and hit testing
 */
class WidgetLayoutHelper
{
public:
    // Type alias for layout configuration
    using LayoutConfig = FlexLayoutConfig;

    /**
     * Set layout configuration for a container element
     * @param widget The widget instance
     * @param id Element ID
     * @param config Layout configuration
     */
    static void SetLayoutConfig(Widget& widget, const std::wstring& id, const LayoutConfig& config);

    /**
     * Try to get layout configuration for an element
     * @param widget The widget instance
     * @param id Element ID
     * @param config Output parameter for configuration
     * @return true if config was found, false otherwise
     */
    static bool TryGetLayoutConfig(const Widget& widget, const std::wstring& id, LayoutConfig& config);

    /**
     * Check if an element is a layout container
     * @param widget The widget instance
     * @param id Element ID
     * @return true if element has layout configuration
     */
    static bool IsLayoutContainer(const Widget& widget, const std::wstring& id);

    /**
     * Reflow layout for a specific container
     * @param widget The widget instance
     * @param id Container element ID
     */
    static void ReflowLayout(Widget& widget, const std::wstring& id);

    /**
     * Apply layout to a container element
     * @param widget The widget instance
     * @param container The container element
     */
    static void ApplyLayoutForContainer(const Widget& widget, Element* container);

    /**
     * Update the container assignment for an element
     * @param widget The widget instance
     * @param element The element to update
     * @param newContainerId ID of the new container (empty string to remove from container)
     */
    static void UpdateContainerForElement(Widget& widget, Element* element, const std::wstring& newContainerId);

    /**
     * Check if assigning an element to a container would create a cycle
     * @param element The element to be assigned
     * @param container The potential container
     * @return true if a cycle would be created
     */
    static bool WouldCreateContainerCycle(const Element* element, const Element* container);

    /**
     * Render all children of a container element
     * @param widget The widget instance
     * @param container The container element
     */
    static void RenderContainerChildren(const Widget& widget, Element* container);

    /**
     * Hit test children of a container
     * @param container The container element
     * @param x X coordinate
     * @param y Y coordinate
     * @param outElement Output parameter for hit element
     * @return true if an element was hit
     */
    static bool HitTestContainerChildren(Element* container, int x, int y, Element*& outElement);

    /**
     * Detailed hit test for container children
     * @param widget The widget instance
     * @param container The container element
     * @param x X coordinate
     * @param y Y coordinate
     * @param message Windows message
     * @param wParam Windows wParam
     * @param outHitElement Output: element under cursor
     * @param outActionElement Output: element with action for this message
     * @param outMouseActionElement Output: element with any mouse action
     * @param outToolTipElement Output: element with tooltip
     * @return true if an element was hit
     */
    static bool HitTestContainerChildrenDetailed(
        const Widget& widget,
        Element* container,
        int x,
        int y,
        UINT message,
        WPARAM wParam,
        Element*& outHitElement,
        Element*& outActionElement,
        Element*& outMouseActionElement,
        Element*& outToolTipElement);

    /**
     * Get the layout configs map from widget (for internal use)
     * @param widget The widget instance
     * @return Reference to layout configs map
     */
    static std::unordered_map<std::wstring, LayoutConfig>& GetLayoutConfigs(Widget& widget);
    static const std::unordered_map<std::wstring, LayoutConfig>& GetLayoutConfigs(const Widget& widget);

private:
    // No instances - static utility class
    WidgetLayoutHelper() = delete;
};

#endif // __NOVADESK_WIDGET_LAYOUT_HELPER_H__
