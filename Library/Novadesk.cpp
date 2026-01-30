/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "framework.h"
#include "Novadesk.h"
#include "JSApi/duktape/duktape.h"
#include "Widget.h"
#include "System.h"
#include "Logging.h"
#include "JSApi/JSApi.h"
#include "JSApi/JSNovadeskTray.h"
#include "Settings.h"
#include "Resource.h"
#include <vector>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#include "MenuUtils.h"
#include "Utils.h"
#include "PathUtils.h"
#include <commctrl.h>
#include "Direct2DHelper.h"

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
duk_context *ctx = nullptr;
std::vector<Widget*> widgets;
NOTIFYICONDATA nid = {};

// Forward declarations of functions included in this code module:
void InitTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void ShowTrayMenu(HWND hWnd);
void ShowTrayIconDynamic();
void HideTrayIconDynamic();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // Attach to parent console for logging if present
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        _setmode(_fileno(stdout), _O_U16TEXT);
    }

    // Clear log file on startup
    std::wstring logPath = PathUtils::GetAppDataPath() + L"logs.log";
    DeleteFileW(logPath.c_str());

    // Enable DPI Awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    std::wstring appTitle = Utils::GetAppTitle();
    std::wstring mutexName = L"Global\\NovadeskMutex_" + appTitle;
    std::wstring className = L"NovadeskTrayClass_" + appTitle;

    // Single instance enforcement
    HANDLE hMutex = CreateMutex(NULL, TRUE, mutexName.c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Another instance is running, check arguments for commands
        if (lpCmdLine && wcslen(lpCmdLine) > 0)
        {
            std::wstring cmd = lpCmdLine;
            HWND hExisting = FindWindow(className.c_str(), NULL);

            if (hExisting)
            {
                if (cmd.find(L"/exit") != std::wstring::npos || 
                         cmd.find(L"-exit") != std::wstring::npos ||
                         cmd.find(L"--exit") != std::wstring::npos)
                {
                    SendMessage(hExisting, WM_COMMAND, ID_TRAY_EXIT, 0);
                    return 0;
                }
            }
        }

        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    Logging::Log(LogLevel::Info, L"Application starting...");

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icce);

    System::Initialize(hInstance);

    // Initialize Direct2D
    Direct2D::Initialize();

    // Initialize Settings
    Settings::Initialize();

    // Initialize global strings
    wcscpy_s(szTitle, MAX_LOADSTRING, appTitle.c_str());
    hInst = hInstance;

    // Create a hidden message-only window for tray icon
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
        // Route custom JSApi messages and timers
        JSApi::OnMessage(message, wParam, lParam);

        switch (message)
        {
        case WM_TIMER:
            JSApi::OnTimer(wParam);
            break;
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP)
            {
                ShowTrayMenu(hWnd);
            }
            break;
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == ID_TRAY_EXIT)
            {
                DestroyWindow(hWnd);
            }
            else if (wmId == ID_TRAY_REFRESH)
            {
                JSApi::Reload();
            }
            else if (wmId >= 2000)
            {
                JSApi::OnTrayCommand(wmId);
            }

        }
        break;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    };
    wcex.hInstance = hInstance;
    wcex.lpszClassName = className.c_str();

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(className.c_str(), szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        Logging::Log(LogLevel::Error, L"Failed to create message window");
        return FALSE;
    }

    // Initialize tray icon and JS message routing
    JSApi::SetMessageWindow(hWnd);
    InitTrayIcon(hWnd);

    // Initialize Duktape
    Logging::Log(LogLevel::Info, L"Initializing Duktape (version %ld)...", (long)DUK_VERSION);
    ctx = duk_create_heap_default();
    if (!ctx) {
        Logging::Log(LogLevel::Error, L"Failed to create Duktape heap.");
        return FALSE;
    }

    // Initialize JavaScript API
    JSApi::InitializeJavaScriptAPI(ctx);

    // Parse command line for custom script path
    std::wstring scriptPath;
    if (lpCmdLine && wcslen(lpCmdLine) > 0)
    {
        scriptPath = lpCmdLine;
        
        // Trim leading and trailing whitespace
        size_t first = scriptPath.find_first_not_of(L" \t\r\n");
        if (std::wstring::npos != first) {
            size_t last = scriptPath.find_last_not_of(L" \t\r\n");
            scriptPath = scriptPath.substr(first, (last - first + 1));
        }

        // Remove quotes if present
        if (!scriptPath.empty() && scriptPath.front() == L'"' && scriptPath.back() == L'"')
        {
            scriptPath = scriptPath.substr(1, scriptPath.length() - 2);
        }
        
        // Trim again after quote removal in case there were spaces inside quotes (unlikely but safe)
        first = scriptPath.find_first_not_of(L" \t\r\n");
        if (std::wstring::npos != first) {
            size_t last = scriptPath.find_last_not_of(L" \t\r\n");
            scriptPath = scriptPath.substr(first, (last - first + 1));
        }

        if (!scriptPath.empty()) {
            Logging::Log(LogLevel::Info, L"Using custom script: %s", scriptPath.c_str());
        }
    }

    // Load and execute script (with optional custom path)
    JSApi::LoadAndExecuteScript(ctx, scriptPath.empty() ? L"" : scriptPath);

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    std::vector<Widget*> widgetsCopy = widgets;
    widgets.clear();
    for (auto w : widgetsCopy) delete w;
    
    duk_destroy_heap(ctx);
    System::Finalize();
    
    // Convert GDI+ shutdown
    Direct2D::Cleanup();

    // Close mutex
    if (hMutex) CloseHandle(hMutex);

    return (int) msg.wParam;
}

void InitTrayIcon(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOVADESK));
    wcscpy_s(nid.szTip, szTitle);

    if (Settings::GetGlobalBool("hideTrayIcon", false)) {
        Logging::Log(LogLevel::Info, L"Tray icon hidden by settings");
        return;
    }

    Shell_NotifyIcon(NIM_ADD, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon initialized");
}

void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon removed");
}

std::vector<MenuItem> g_TrayMenu;
bool g_ShowDefaultTrayItems = true;

void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();

    MenuUtils::BuildMenu(hMenu, g_TrayMenu);

    if (g_ShowDefaultTrayItems)
    {
        if (!g_TrayMenu.empty())
        {
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        }
        
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenu(hSubMenu, MF_STRING, ID_TRAY_REFRESH, L"Refresh");
        AppendMenu(hSubMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
        
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, szTitle);
    }

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

void ShowTrayIconDynamic()
{
    if (nid.hWnd)
    {
        Shell_NotifyIcon(NIM_ADD, &nid);
        Logging::Log(LogLevel::Info, L"Tray icon shown");
    }
}

void HideTrayIconDynamic()
{
    if (nid.hWnd)
    {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        Logging::Log(LogLevel::Info, L"Tray icon hidden");
    }
}


void SetTrayMenu(const std::vector<MenuItem>& menu)
{
    g_TrayMenu = menu;
}

void ClearTrayMenu()
{
    g_TrayMenu.clear();
}

void SetShowDefaultTrayItems(bool show)
{
    g_ShowDefaultTrayItems = show;
}
