#pragma once

#include <windows.h>

struct WidgetOptions;

namespace WidgetWindowChromeHelper
{
    void ApplyToolbarStyle(HWND hWnd, bool showInToolbar);
    void DestroyToolbarIcon(HICON &iconHandle, bool &iconOwned);
    void ApplyToolbarIcon(HWND hWnd, const WidgetOptions &options, HICON &iconHandle, bool &iconOwned);
    void ApplyToolbarTitle(HWND hWnd, const WidgetOptions &options);
}

