/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "WidgetContextMenuHelper.h"

#include <string>
#include <cstdlib>

#include "Widget.h"
#include "../shared/MenuUtils.h"
#include "PathUtils.h"
#include "../scripting/quickjs/engine/JSEngine.h"

namespace
{
    constexpr int CMD_REFRESH = 1001;
    constexpr int CMD_CLOSE = 1002;
    constexpr int CMD_EXIT = 1003;

    constexpr int CMD_MANAGE_ZPOS_NORMAL = 1101;
    constexpr int CMD_MANAGE_ZPOS_DESKTOP = 1102;
    constexpr int CMD_MANAGE_ZPOS_BOTTOM = 1103;
    constexpr int CMD_MANAGE_ZPOS_ONTOP = 1104;
    constexpr int CMD_MANAGE_ZPOS_ONTOPMOST = 1105;

    constexpr int CMD_MANAGE_OPACITY_START = 1110;

    constexpr int CMD_MANAGE_DRAGGABLE = 1130;
    constexpr int CMD_MANAGE_CLICKTHROUGH = 1131;
    constexpr int CMD_MANAGE_SNAPEDGES = 1132;
    constexpr int CMD_MANAGE_KEEPOFFSCREEN = 1133;
}

namespace WidgetContextMenuHelper
{
    int ShowContextMenu(
        HWND hWnd,
        const std::vector<MenuItem> &customMenu,
        bool showDefaultItems,
        ZPOSITION windowZPos,
        const WidgetOptions &options)
    {
        POINT pt{};
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        MenuUtils::BuildMenu(hMenu, customMenu);

        if (showDefaultItems)
        {
            if (!customMenu.empty())
            {
                AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            }

            HMENU hManageMenu = CreatePopupMenu();

            HMENU hZPosMenu = CreatePopupMenu();
            AppendMenuW(hZPosMenu, MF_STRING | (windowZPos == ZPOSITION_NORMAL ? MF_CHECKED : 0), CMD_MANAGE_ZPOS_NORMAL, L"Normal");
            AppendMenuW(hZPosMenu, MF_STRING | (windowZPos == ZPOSITION_ONDESKTOP ? MF_CHECKED : 0), CMD_MANAGE_ZPOS_DESKTOP, L"OnDesktop");
            AppendMenuW(hZPosMenu, MF_STRING | (windowZPos == ZPOSITION_ONTOP ? MF_CHECKED : 0), CMD_MANAGE_ZPOS_ONTOP, L"OnTop");
            AppendMenuW(hZPosMenu, MF_STRING | (windowZPos == ZPOSITION_ONTOPMOST ? MF_CHECKED : 0), CMD_MANAGE_ZPOS_ONTOPMOST, L"OnTopMost");
            AppendMenuW(hZPosMenu, MF_STRING | (windowZPos == ZPOSITION_ONBOTTOM ? MF_CHECKED : 0), CMD_MANAGE_ZPOS_BOTTOM, L"Bottom");
            AppendMenuW(hManageMenu, MF_POPUP, (UINT_PTR)hZPosMenu, L"Zpos");

            HMENU hOpacityMenu = CreatePopupMenu();
            for (int i = 0; i <= 100; i += 10)
            {
                const BYTE targetOpacity = static_cast<BYTE>(i * 255 / 100);
                const std::wstring label = std::to_wstring(i) + L"%";
                const bool isCurrent = std::abs(static_cast<int>(options.windowOpacity) - static_cast<int>(targetOpacity)) < 5;
                AppendMenuW(hOpacityMenu, MF_STRING | (isCurrent ? MF_CHECKED : 0), CMD_MANAGE_OPACITY_START + (i / 10), label.c_str());
            }
            AppendMenuW(hManageMenu, MF_POPUP, (UINT_PTR)hOpacityMenu, L"Opacity");
            AppendMenuW(hManageMenu, MF_SEPARATOR, 0, nullptr);

            AppendMenuW(hManageMenu, MF_STRING | (options.draggable ? MF_CHECKED : 0), CMD_MANAGE_DRAGGABLE, L"Draggable");
            AppendMenuW(hManageMenu, MF_STRING | (options.clickThrough ? MF_CHECKED : 0), CMD_MANAGE_CLICKTHROUGH, L"Clickthrough");
            AppendMenuW(hManageMenu, MF_STRING | (options.snapEdges ? MF_CHECKED : 0), CMD_MANAGE_SNAPEDGES, L"Snap to Edges");
            AppendMenuW(hManageMenu, MF_STRING | (options.keepOnScreen ? MF_CHECKED : 0), CMD_MANAGE_KEEPOFFSCREEN, L"Keep On Screen");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hManageMenu, L"Manage");

            HMENU hAppMenu = CreatePopupMenu();
            AppendMenuW(hAppMenu, MF_STRING, CMD_REFRESH, L"Refresh");
            AppendMenuW(hAppMenu, MF_STRING, CMD_EXIT, L"Exit");
            std::wstring appTitle = PathUtils::GetProductName();
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hAppMenu, appTitle.c_str());
        }

        SetForegroundWindow(hWnd);
        const int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
        return cmd;
    }

    void HandleContextCommand(Widget &widget, int cmd)
    {
        const WidgetOptions &options = widget.GetOptions();
        if (cmd >= 2000)
        {
            JSEngine::OnWidgetContextCommand(options.id, cmd);
            return;
        }
        if (cmd == CMD_REFRESH)
        {
            JSEngine::Reload();
            return;
        }
        if (cmd == CMD_CLOSE)
        {
            DestroyWindow(widget.GetWindow());
            return;
        }
        if (cmd == CMD_EXIT)
        {
            PostQuitMessage(0);
            return;
        }
        if (cmd == CMD_MANAGE_ZPOS_NORMAL)
        {
            widget.ChangeZPos(ZPOSITION_NORMAL);
            return;
        }
        if (cmd == CMD_MANAGE_ZPOS_DESKTOP)
        {
            widget.ChangeZPos(ZPOSITION_ONDESKTOP);
            return;
        }
        if (cmd == CMD_MANAGE_ZPOS_BOTTOM)
        {
            widget.ChangeZPos(ZPOSITION_ONBOTTOM);
            return;
        }
        if (cmd == CMD_MANAGE_ZPOS_ONTOP)
        {
            widget.ChangeZPos(ZPOSITION_ONTOP);
            return;
        }
        if (cmd == CMD_MANAGE_ZPOS_ONTOPMOST)
        {
            widget.ChangeZPos(ZPOSITION_ONTOPMOST);
            return;
        }
        if (cmd >= CMD_MANAGE_OPACITY_START && cmd <= CMD_MANAGE_OPACITY_START + 10)
        {
            const int percent = (cmd - CMD_MANAGE_OPACITY_START) * 10;
            widget.SetWindowOpacity(static_cast<BYTE>(percent * 255 / 100));
            return;
        }
        if (cmd == CMD_MANAGE_DRAGGABLE)
        {
            widget.SetDraggable(!options.draggable);
            return;
        }
        if (cmd == CMD_MANAGE_CLICKTHROUGH)
        {
            widget.SetClickThrough(!options.clickThrough);
            return;
        }
        if (cmd == CMD_MANAGE_SNAPEDGES)
        {
            widget.SetSnapEdges(!options.snapEdges);
            return;
        }
        if (cmd == CMD_MANAGE_KEEPOFFSCREEN)
        {
            widget.SetKeepOnScreen(!options.keepOnScreen);
            return;
        }
    }
}

