/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FlexLayoutEngine.h"
#include "ElementLayoutBox.h"
#include "../shared/Logging.h"
#include <algorithm>
#include <vector>

void FlexLayoutEngine::ApplyLayout(Element* container, const FlexLayoutConfig& config)
{
    if (!container)
        return;

    const auto& items = container->GetContainerItems();
    if (items.empty())
        return;

    // Set layout properties on ElementLayoutBox for auto-sizing calculations
    ElementLayoutBox* layoutBox = dynamic_cast<ElementLayoutBox*>(container);
    if (layoutBox)
    {
        layoutBox->SetLayoutDirection(config.flexDirection);
        layoutBox->SetLayoutGap(config.gap);
    }

    GfxRect bounds = container->GetBounds();
    int innerW = bounds.Width - config.paddingLeft - config.paddingRight;
    int innerH = bounds.Height - config.paddingTop - config.paddingBottom;
    if (innerW < 0) innerW = 0;
    if (innerH < 0) innerH = 0;

    // Logging::Log(LogLevel::Debug, L"[FLEX_LAYOUT] ApplyLayout on '%s': bounds W=%d H=%d, padding L=%d T=%d R=%d B=%d, inner W=%d H=%d",
    //     container->GetId().c_str(), bounds.Width, bounds.Height,
    //     config.paddingLeft, config.paddingTop, config.paddingRight, config.paddingBottom,
    //     innerW, innerH);

    // Parse direction (for text directionality - affects alignment)
    std::wstring dir = config.direction;
    std::transform(dir.begin(), dir.end(), dir.begin(), ::towlower);
    const bool isRtl = (dir == L"rtl");

    // Parse flexDirection (for layout flow)
    std::wstring flexDir = config.flexDirection;
    std::transform(flexDir.begin(), flexDir.end(), flexDir.begin(), ::towlower);

    const bool isHorizontal = (flexDir == L"row" || flexDir == L"rowreverse");
    const bool isReverse = (flexDir == L"rowreverse" || flexDir == L"columnreverse");

    // Calculate total size needed for layout
    int mainTotal = 0;
    for (Element* child : items)
    {
        if (!child) continue;
        mainTotal += isHorizontal ? child->GetWidth() : child->GetHeight();
    }
    if (items.size() > 1)
    {
        mainTotal += config.gap * static_cast<int>(items.size() - 1);
    }

    int mainAvail = isHorizontal ? innerW : innerH;
    int mainStart = 0;
    std::wstring justify = config.justify;
    std::transform(justify.begin(), justify.end(), justify.begin(), ::towlower);
    if (justify == L"center")
    {
        mainStart = (mainAvail - mainTotal) / 2;
    }
    else if (justify == L"end")
    {
        mainStart = (mainAvail - mainTotal);
    }
    if (mainStart < 0) mainStart = 0;

    // Create list of children (potentially reversed)
    std::vector<Element*> orderedItems;
    if (isReverse)
    {
        orderedItems.assign(items.rbegin(), items.rend());
    }
    else
    {
        orderedItems.assign(items.begin(), items.end());
    }

    int cursor = mainStart;
    for (Element* child : orderedItems)
    {
        if (!child) continue;

        int childW = child->GetWidth();
        int childH = child->GetHeight();

        std::wstring align = config.align;
        std::transform(align.begin(), align.end(), align.begin(), ::towlower);

        int crossPos = 0;
        bool shouldStretch = false;

        if (isHorizontal)
        {
            // Horizontal layout (row/rowReverse)
            // Cross axis is vertical
            if (align == L"center")
            {
                crossPos = (innerH - childH) / 2;
            }
            else if (align == L"end" || align == L"flexend")
            {
                crossPos = (innerH - childH);
            }
            else if (align == L"start" || align == L"flexstart")
            {
                crossPos = 0;
            }
            else if (align == L"stretch" || align == L"normal")
            {
                // CSS Spec: For flex items, both "stretch" and "normal" behave identically
                // They stretch items to fill the cross axis
                crossPos = 0;
                shouldStretch = true;
                if (innerH > 0)
                {
                    childH = innerH;
                    child->SetSize(childW, childH);
                }
            }
            else
            {
                // Default to start
                crossPos = 0;
            }

            child->SetPosition(config.paddingLeft + cursor, config.paddingTop + (crossPos < 0 ? 0 : crossPos));
            cursor += childW + config.gap;
        }
        else
        {
            // Vertical layout (column/columnReverse)
            // Cross axis is horizontal, affected by text direction
            if (align == L"center")
            {
                crossPos = (innerW - childW) / 2;
            }
            else if (align == L"start" || align == L"flexstart")
            {
                // RTL: start means right side, LTR: start means left side
                crossPos = isRtl ? (innerW - childW) : 0;
            }
            else if (align == L"end" || align == L"flexend")
            {
                // RTL: end means left side, LTR: end means right side
                crossPos = isRtl ? 0 : (innerW - childW);
            }
            else if (align == L"stretch" || align == L"normal")
            {
                // CSS Spec: For flex items, both "stretch" and "normal" behave identically
                // They stretch items to fill the cross axis
                crossPos = 0;
                shouldStretch = true;
                if (innerW > 0)
                {
                    childW = innerW;
                    child->SetSize(childW, childH);
                }
            }
            else
            {
                // Default alignment based on text direction
                crossPos = isRtl ? (innerW - childW) : 0;
            }

            child->SetPosition(config.paddingLeft + (crossPos < 0 ? 0 : crossPos), config.paddingTop + cursor);
            cursor += childH + config.gap;
        }
    }
}
