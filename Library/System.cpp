/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


/*
** Most of the Code taken from Rainmeter (https://github.com/rainmeter/rainmeter/blob/master/Library/System.cpp)
*/

#include "System.h"
#include "Widget.h"
#include <algorithm>
#include <shellapi.h>
#include <winreg.h>

extern std::vector<Widget*> widgets; // Defined in Novadesk.cpp

HWND System::c_Window = nullptr;
HWND System::c_HelperWindow = nullptr;
MultiMonitorInfo System::c_Monitors = { 0 };
bool System::c_ShowDesktop = false;

#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

// Internal helper for GetBackmostTopWindow
static Widget* FindWidget(HWND hWnd)
{
    for (auto w : widgets)
    {
        if (w->GetWindow() == hWnd) return w;
    }
    return nullptr;
}

/*
** Initialize the System module.
** Sets up helper windows and initializes multi-monitor information.
*/

void System::Initialize(HINSTANCE instance)
{
    // Initialize monitors
    c_Monitors.monitors.clear();
    c_Monitors.vsL = GetSystemMetrics(SM_XVIRTUALSCREEN);
    c_Monitors.vsT = GetSystemMetrics(SM_YVIRTUALSCREEN);
    c_Monitors.vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    c_Monitors.vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    c_Monitors.primaryIndex = 0;

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    // Register a specialized class for system tracking
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = (WNDPROC)System::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NovadeskSystem";
    RegisterClass(&wc);

    // Create a dummy window for System tracking
    c_Window = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        L"NovadeskSystem",
        L"System",
        WS_POPUP | WS_DISABLED,
        0, 0, 0, 0,
        nullptr, nullptr, instance, nullptr);

    PrepareHelperWindow();

    SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);
}

/*
** Finalize the System module.
** Cleans up resources and destroys helper windows.
*/

void System::Finalize()
{
    if (c_HelperWindow)
    {
        DestroyWindow(c_HelperWindow);
        c_HelperWindow = nullptr;
    }
    if (c_Window)
    {
        DestroyWindow(c_Window);
        c_Window = nullptr;
    }
}

/*
** Callback for enumerating monitors.
** Used to populate multi-monitor information.
*/

BOOL CALLBACK System::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MonitorInfo info;
    info.handle = hMonitor;
    info.screen = *lprcMonitor;
    info.active = true;
    
    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hMonitor, &mi))
    {
        info.work = mi.rcWork;
        info.deviceName = mi.szDevice;

        DISPLAY_DEVICE ddm = { sizeof(DISPLAY_DEVICE) };
        DWORD dwMon = 0;
        while (EnumDisplayDevices(info.deviceName.c_str(), dwMon++, &ddm, 0))
        {
            if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
            {
                info.monitorName = ddm.DeviceString;
                break;
            }
        }

        if (mi.dwFlags & MONITORINFOF_PRIMARY)
        {
            c_Monitors.primaryIndex = (int)c_Monitors.monitors.size();
        }
    }
    
    c_Monitors.monitors.push_back(info);
    return TRUE;
}

/*
** Get the default shell window.
*/

HWND System::GetDefaultShellWindow()
{
    return GetShellWindow();
}

/*
** Determine if the shell window should be used as the desktop icons host.
*/

bool System::ShouldUseShellWindowAsDesktopIconsHost()
{
    // Check for the existence of GetCurrentMonitorTopologyId, which should be present only
    // on Windows 11 build 10.0.26100.2454.
    static bool result = GetProcAddress(GetModuleHandle(L"user32"), "GetCurrentMonitorTopologyId") != nullptr;
    return result;
}

/*
** Get the window that hosts desktop icons.
** This is typically the shell window or WorkerW window.
*/

HWND System::GetDesktopIconsHostWindow()
{
    HWND shellW = GetDefaultShellWindow();
    if (!shellW) return nullptr;

    if (ShouldUseShellWindowAsDesktopIconsHost())
    {
        if (FindWindowEx(shellW, nullptr, L"SHELLDLL_DefView", L""))
        {
            return shellW;
        }
    }

    HWND workerW = nullptr;
    HWND defView = nullptr;
    while (workerW = FindWindowEx(nullptr, workerW, L"WorkerW", L""))
    {
        if (IsWindowVisible(workerW) &&
            (defView = FindWindowEx(workerW, nullptr, L"SHELLDLL_DefView", L"")))
        {
            break;
        }
    }

    return (defView != nullptr) ? workerW : nullptr;
}

/*
** Prepare the helper window for desktop icon management.
** Can optionally specify a custom desktop icons host window.
*/

void System::PrepareHelperWindow(HWND desktopIconsHostWindow)
{
    if (!c_HelperWindow || !IsWindow(c_HelperWindow))
    {
        c_HelperWindow = CreateWindowEx(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"NovadeskSystem", L"PositioningHelper",
            WS_POPUP,
            0, 0, 0, 0,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    }

    if (!desktopIconsHostWindow) desktopIconsHostWindow = GetDesktopIconsHostWindow();

    SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

    if (c_ShowDesktop && desktopIconsHostWindow)
    {
        SetWindowPos(c_HelperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
    }
    else if (desktopIconsHostWindow)
    {
        SetWindowPos(c_HelperWindow, desktopIconsHostWindow, 0, 0, 0, 0, ZPOS_FLAGS);
    }
    else
    {
        SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
    }
}

/*
** Check if the desktop is currently being shown.
*/

bool System::CheckDesktopState(HWND desktopIconsHostWindow)
{
    HWND hwnd = nullptr;

    if (desktopIconsHostWindow && IsWindowVisible(desktopIconsHostWindow))
    {
        hwnd = FindWindowEx(nullptr, desktopIconsHostWindow, L"NovadeskSystem", L"System");
    }

    bool stateChanged = (hwnd && !c_ShowDesktop) || (!hwnd && c_ShowDesktop);

    if (stateChanged)
    {
        c_ShowDesktop = !c_ShowDesktop;

        PrepareHelperWindow(desktopIconsHostWindow);
        ChangeZPosInOrder();

        SetTimer(c_Window, TIMER_SHOWDESKTOP, c_ShowDesktop ? INTERVAL_RESTOREWINDOWS : INTERVAL_SHOWDESKTOP, nullptr);
    }

    return stateChanged;
}

static BOOL CALLBACK EnumWidgetsProc(HWND hwnd, LPARAM lParam)
{
    std::vector<Widget*>* windowsInZOrder = (std::vector<Widget*>*)lParam;
    Widget* widget = FindWidget(hwnd);
    if (widget)
    {
        windowsInZOrder->push_back(widget);
    }
    return TRUE;
}

/*
** Change z-order positions for all widgets in the correct order.
** Ensures widgets maintain their relative z-order positions.
*/

void System::ChangeZPosInOrder()
{
    std::vector<Widget*> windowsInZOrder;
    
    // Enumerate all windows to get current Z-order
    EnumWindows(EnumWidgetsProc, (LPARAM)(&windowsInZOrder));
    
    // Reapply Z-positions in the order they were found (preserves user's stacking)
    for (auto w : windowsInZOrder)
    {
        w->ChangeZPos(w->GetWindowZPosition());
    }
}

/*
** Window procedure for the system helper window.
*/

LRESULT CALLBACK System::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == TIMER_SHOWDESKTOP)
        {
            CheckDesktopState(GetDesktopIconsHostWindow());
        }
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
** Get the backmost top-level window.
** Used for z-order management of desktop widgets.
*/

HWND System::GetBackmostTopWindow()
{
    HWND winPos = c_HelperWindow;
    if (!winPos) return HWND_NOTOPMOST;

    // Skip all ZPOSITION_ONDESKTOP, ZPOSITION_ONBOTTOM, and ZPOSITION_NORMAL windows
    while (winPos = ::GetNextWindow(winPos, GW_HWNDPREV))
    {
        Widget* wnd = FindWidget(winPos);
        if (!wnd ||
            (wnd->GetWindowZPosition() != ZPOSITION_NORMAL &&
             wnd->GetWindowZPosition() != ZPOSITION_ONDESKTOP &&
             wnd->GetWindowZPosition() != ZPOSITION_ONBOTTOM))
        {
            break;
        }
    }

    return winPos;
}

bool System::Execute(const std::wstring& target, const std::wstring& parameters, const std::wstring& workingDir, int show)
{
    HINSTANCE result = ShellExecuteW(NULL, L"open", target.c_str(), parameters.empty() ? NULL : parameters.c_str(), workingDir.empty() ? NULL : workingDir.c_str(), show);
    return (intptr_t)result > 32;
}

static bool SetWallpaperStyleValue(const std::wstring& style)
{
    std::wstring lower = style;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    const wchar_t* wallpaperStyle = L"10"; // fill
    const wchar_t* tileWallpaper = L"0";

    if (lower == L"fill") {
        wallpaperStyle = L"10"; tileWallpaper = L"0";
    } else if (lower == L"fit") {
        wallpaperStyle = L"6"; tileWallpaper = L"0";
    } else if (lower == L"stretch") {
        wallpaperStyle = L"2"; tileWallpaper = L"0";
    } else if (lower == L"tile") {
        wallpaperStyle = L"0"; tileWallpaper = L"1";
    } else if (lower == L"center") {
        wallpaperStyle = L"0"; tileWallpaper = L"0";
    } else if (lower == L"span") {
        wallpaperStyle = L"22"; tileWallpaper = L"0";
    } else {
        return false;
    }

    HKEY hKey = nullptr;
    LONG status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    status = RegSetValueExW(
        hKey,
        L"WallpaperStyle",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(wallpaperStyle),
        static_cast<DWORD>((wcslen(wallpaperStyle) + 1) * sizeof(wchar_t)));

    if (status == ERROR_SUCCESS) {
        status = RegSetValueExW(
            hKey,
            L"TileWallpaper",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(tileWallpaper),
            static_cast<DWORD>((wcslen(tileWallpaper) + 1) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}

bool System::SetWallpaper(const std::wstring& imagePath, const std::wstring& style)
{
    if (imagePath.empty()) return false;

    DWORD attrs = GetFileAttributesW(imagePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    if (!SetWallpaperStyleValue(style)) {
        return false;
    }

    BOOL ok = SystemParametersInfoW(
        SPI_SETDESKWALLPAPER,
        0,
        (PVOID)imagePath.c_str(),
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    return ok == TRUE;
}

bool System::GetCurrentWallpaperPath(std::wstring& outPath)
{
    outPath.clear();

    wchar_t buffer[MAX_PATH] = {};
    if (!SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, buffer, 0)) {
        return false;
    }

    if (buffer[0] == L'\0') {
        return false;
    }

    outPath = buffer;
    return true;
}
