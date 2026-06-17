/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_FLEX_LAYOUT_ENGINE_H__
#define __NOVADESK_FLEX_LAYOUT_ENGINE_H__

#include <string>
#include "Element.h"

/**
 * Configuration for flexbox layout
 */
struct FlexLayoutConfig
{
    std::wstring direction = L"ltr";        // "ltr" | "rtl" - Text directionality
    std::wstring flexDirection = L"row";    // "row" | "rowreverse" | "column" | "columnreverse"
    int gap = 0;                            // Gap between items
    std::wstring align = L"start";          // "normal" | "stretch" | "center" | "start" | "end" | "flexstart" | "flexend"
    std::wstring justify = L"start";        // "start" | "center" | "end" - main axis alignment
    int paddingLeft = 0;
    int paddingTop = 0;
    int paddingRight = 0;
    int paddingBottom = 0;
};

/**
 * FlexLayoutEngine - Implements CSS Flexbox layout algorithm
 * 
 * This engine handles positioning and sizing of child elements within a flex container
 * according to CSS Flexbox specification rules.
 */
class FlexLayoutEngine
{
public:
    /**
     * Apply flexbox layout to a container and its children
     * 
     * @param container The container element to layout
     * @param config The flexbox configuration (direction, alignment, gaps, etc.)
     */
    static void ApplyLayout(Element* container, const FlexLayoutConfig& config);

private:
    // No instances - static utility class
    FlexLayoutEngine() = delete;
};

#endif // __NOVADESK_FLEX_LAYOUT_ENGINE_H__
