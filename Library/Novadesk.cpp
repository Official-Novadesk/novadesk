/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "framework.h"
#include "Novadesk.h"
#include "duktape/duktape.h"
#include "Widget.h"
#include "System.h"
#include "Logging.h"
#include "JSApi.h"
#include "Settings.h"
#include "Resource.h"
#include <vector>
#include <shellapi.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
duk_context *ctx = nullptr;
std::vector<Widget*> widgets;
NOTIFYICONDATA nid = {};
ULONG_PTR gdiplusToken;

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
    // Enable DPI Awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Single instance enforcement
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"Global\\NovadeskMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Another instance is running, check arguments for commands
        if (lpCmdLine && wcslen(lpCmdLine) > 0)
        {
            std::wstring cmd = lpCmdLine;
            HWND hExisting = FindWindow(L"NovadeskTrayClass", NULL);

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
    System::Initialize(hInstance);
    
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize Settings
    Settings::Initialize();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
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
    wcex.lpszClassName = L"NovadeskTrayClass";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"NovadeskTrayClass", szTitle, WS_OVERLAPPEDWINDOW,
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
        // Remove quotes if present
        scriptPath = lpCmdLine;
        if (!scriptPath.empty() && scriptPath.front() == L'"' && scriptPath.back() == L'"')
        {
            scriptPath = scriptPath.substr(1, scriptPath.length() - 2);
        }
        Logging::Log(LogLevel::Info, L"Using custom script: %s", scriptPath.c_str());
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
    for (auto w : widgets) delete w;
    duk_destroy_heap(ctx);
    System::Finalize();
    
    // Convert GDI+ shutdown
    GdiplusShutdown(gdiplusToken);

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

    Shell_NotifyIcon(NIM_ADD, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon initialized");
}

void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon removed");
}

void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

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


