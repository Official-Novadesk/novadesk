/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "WidgetWindowChromeHelper.h"

#include "Widget.h"
#include "Resource.h"
#include "../shared/Logging.h"

namespace WidgetWindowChromeHelper
{
    void ApplyToolbarStyle(HWND hWnd, bool showInToolbar)
    {
        if (!hWnd)
            return;

        LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
        if (showInToolbar)
        {
            exStyle &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
            exStyle |= WS_EX_APPWINDOW;
        }
        else
        {
            exStyle &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW);
            exStyle |= WS_EX_TOOLWINDOW;
        }

        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(
            hWnd,
            nullptr,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        if (IsWindowVisible(hWnd))
        {
            ShowWindow(hWnd, SW_HIDE);
            ShowWindow(hWnd, SW_SHOWNOACTIVATE);
        }
    }

    void DestroyToolbarIcon(HICON &iconHandle, bool &iconOwned)
    {
        if (iconOwned && iconHandle)
        {
            DestroyIcon(iconHandle);
        }
        iconHandle = nullptr;
        iconOwned = false;
    }

    void ApplyToolbarIcon(HWND hWnd, const WidgetOptions &options, HICON &iconHandle, bool &iconOwned)
    {
        if (!hWnd)
            return;

        DestroyToolbarIcon(iconHandle, iconOwned);

        HICON icon = nullptr;
        if (!options.toolbarIcon.empty())
        {
            icon = reinterpret_cast<HICON>(LoadImageW(
                nullptr,
                options.toolbarIcon.c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE));

            if (icon)
            {
                iconHandle = icon;
                iconOwned = true;
            }
            else
            {
                Logging::Log(LogLevel::Warn, L"Widget '%s': failed to load toolbarIcon '%s'", options.id.c_str(), options.toolbarIcon.c_str());
            }
        }

        if (!icon)
        {
            icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_NOVADESK));
        }

        SendMessageW(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        SendMessageW(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
    }

    void ApplyToolbarTitle(HWND hWnd, const WidgetOptions &options)
    {
        if (!hWnd)
            return;

        const std::wstring title = options.toolbarTitle.empty() ? options.id : options.toolbarTitle;
        SetWindowTextW(hWnd, title.c_str());
    }
}

