/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include <windows.h>
#include <vector>

#include "MenuItem.h"
#include "DesktopManager.h"

struct WidgetOptions;
class Widget;

namespace WidgetContextMenuHelper
{
    int ShowContextMenu(
        HWND hWnd,
        const std::vector<MenuItem> &customMenu,
        bool showDefaultItems,
        ZPOSITION windowZPos,
        const WidgetOptions &options);

    void HandleContextCommand(Widget &widget, int cmd);
}

