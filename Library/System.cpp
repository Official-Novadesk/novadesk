#include "System.h"
#include "Widget.h"
#include <algorithm>

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

void System::Initialize(HINSTANCE instance)
{
    // Initialize monitors
    c_Monitors.monitors.clear();
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
    }
    
    c_Monitors.monitors.push_back(info);
    return TRUE;
}

HWND System::GetDefaultShellWindow()
{
    return GetShellWindow();
}

bool System::ShouldUseShellWindowAsDesktopIconsHost()
{
    return true;
}

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

void System::ChangeZPosInOrder()
{
    for (auto w : widgets)
    {
        w->ChangeZPos(w->GetWindowZPosition());
    }
}

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

HWND System::GetBackmostTopWindow()
{
    HWND winPos = c_HelperWindow;
    if (!winPos) return HWND_NOTOPMOST;

    // Skip all ZPOSITION_ONDESKTOP, ZPOSITION_ONBOTTOM, and ZPOSITION_NORMAL windows
    while (HWND next = ::GetNextWindow(winPos, GW_HWNDPREV))
    {
        Widget* wnd = FindWidget(next);
        if (!wnd ||
            (wnd->GetWindowZPosition() != ZPOSITION_NORMAL &&
             wnd->GetWindowZPosition() != ZPOSITION_ONDESKTOP &&
             wnd->GetWindowZPosition() != ZPOSITION_ONBOTTOM))
        {
            break;
        }
        winPos = next;
    }

    return winPos;
}
