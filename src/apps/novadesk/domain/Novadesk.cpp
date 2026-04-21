/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "framework.h"
#include "Novadesk.h"
#include "Widget.h"
#include "DesktopManager.h"
#include "Settings.h"
#include "Resource.h"
#include <vector>
#include <unordered_map>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#include "../shared/MenuUtils.h"
#include <windef.h>
#include "Utils.h"
#include "PathUtils.h"
#include <commctrl.h>
#include "Direct2DHelper.h"
#include "FontManager.h"
#include "../shared/Logging.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../scripting/quickjs/modules/NovadeskModule.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;               // current instance
WCHAR szTitle[MAX_LOADSTRING]; // The title bar text
duk_context *ctx = nullptr;
std::vector<Widget *> widgets;
struct TrayState
{
    bool initialized = false;
    bool iconOwned = false;
    HWND hWnd = nullptr;
    HICON icon = nullptr;
    std::wstring toolTip;
    std::vector<MenuItem> menu;
    NOTIFYICONDATAW nid = {};
};

std::unordered_map<int, TrayState> g_trayStates;
int g_nextTrayId = 1;
HWND g_trayMessageWindow = nullptr;
static HANDLE g_singleInstanceMutex = nullptr;
static std::wstring g_singleInstanceMutexName;
static HHOOK g_trayMouseHook = nullptr;

static BOOL WINAPI NovadeskConsoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_trayMessageWindow)
        {
            PostMessage(g_trayMessageWindow, WM_CLOSE, 0, 0);
        }
        else
        {
            PostQuitMessage(0);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

// Forward declarations of functions included in this code module:
void InitTrayIcon(int trayId);
void RemoveTrayIcon(int trayId);
void RemoveAllTrayIcons();
static TrayState *GetTrayState(int trayId);
static void DispatchTrayMouseEvent(int trayId, const char *name);

static RECT GetTrayIconRect(int trayId)
{
    TrayState *state = GetTrayState(trayId);
    if (!state || !state->initialized)
        return {0, 0, 0, 0};

    NOTIFYICONIDENTIFIER identifier = {sizeof(NOTIFYICONIDENTIFIER)};
    identifier.hWnd = state->hWnd;
    identifier.uID = static_cast<UINT>(trayId);

    // Dynamically load Shell_NotifyIconGetRect for compatibility with MinGW and older systems
    typedef HRESULT(WINAPI * PFN_Shell_NotifyIconGetRect)(const NOTIFYICONIDENTIFIER *, RECT *);
    static PFN_Shell_NotifyIconGetRect pfnGetRect = nullptr;
    static bool pfnChecked = false;

    if (!pfnChecked)
    {
        HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
        if (hShell32)
        {
            pfnGetRect = (PFN_Shell_NotifyIconGetRect)GetProcAddress(hShell32, "Shell_NotifyIconGetRect");
        }
        pfnChecked = true;
    }

    RECT rc = {0};
    if (pfnGetRect && pfnGetRect(&identifier, &rc) == S_OK)
    {
        return rc;
    }
    return {0, 0, 0, 0};
}

static LRESULT CALLBACK TrayMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL)
    {
        MSLLHOOKSTRUCT *pMouseStruct = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
        POINT pt = pMouseStruct->pt;

        for (auto &kv : g_trayStates)
        {
            if (kv.second.initialized)
            {
                RECT rc = GetTrayIconRect(kv.first);
                if (PtInRect(&rc, pt))
                {
                    int zDelta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData);
                    if (zDelta > 0)
                    {
                        DispatchTrayMouseEvent(kv.first, "scroll-up");
                    }
                    else if (zDelta < 0)
                    {
                        DispatchTrayMouseEvent(kv.first, "scroll-down");
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_trayMouseHook, nCode, wParam, lParam);
}
static void ShowTrayMenuInternal(int trayId, const std::vector<MenuItem> *menu, const POINT *position);

static bool SendIpcCommand(HWND hWnd, const std::wstring &command, const std::wstring &path)
{
    if (!hWnd || command.empty())
        return false;
    std::wstring resolved = path;
    if (!resolved.empty() && PathUtils::IsPathRelative(resolved))
    {
        std::error_code ec;
        const std::filesystem::path cwd = std::filesystem::current_path(ec);
        if (!ec)
        {
            resolved = PathUtils::ResolvePath(resolved, cwd.wstring());
        }
    }
    else if (!resolved.empty())
    {
        resolved = PathUtils::NormalizePath(resolved);
    }

    const std::wstring payload = command + L"|" + resolved;
    COPYDATASTRUCT cds{};
    cds.cbData = static_cast<DWORD>((payload.size() + 1) * sizeof(wchar_t));
    cds.lpData = const_cast<wchar_t *>(payload.c_str());
    return SendMessage(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds)) != 0;
}

static std::wstring BuildSingleInstanceMutexName()
{
    std::wstring appTitle = PathUtils::GetProductName();
    return L"Global\\NovadeskMutex_" + appTitle;
}

bool RequestSingleInstanceLock()
{
    if (g_singleInstanceMutex)
    {
        return true;
    }

    g_singleInstanceMutexName = BuildSingleInstanceMutexName();

    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    SECURITY_ATTRIBUTES sa = {sizeof(sa), &sd, FALSE};

    HANDLE hMutex = CreateMutexW(&sa, TRUE, g_singleInstanceMutexName.c_str());
    if (!hMutex)
    {
        return false;
    }

    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED)
    {
        CloseHandle(hMutex);
        return false;
    }

    g_singleInstanceMutex = hMutex;
    return true;
}

void ReleaseSingleInstanceLock()
{
    if (g_singleInstanceMutex)
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
    }
}

static std::wstring CreateTempListPath()
{
    wchar_t tempPath[MAX_PATH + 1] = {};
    if (!GetTempPathW(MAX_PATH, tempPath))
        return L"";
    wchar_t filePath[MAX_PATH + 1] = {};
    if (!GetTempFileNameW(tempPath, L"nds", 0, filePath))
        return L"";
    return std::wstring(filePath);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    // Attach to parent console for logging if present
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        FILE *fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        _setmode(_fileno(stdout), _O_U16TEXT);
    }

    // Clear log file on startup
    std::wstring logPath = PathUtils::GetAppDataPath() + L"logs.log";
    DeleteFileW(logPath.c_str());

    // Enable DPI awareness with runtime fallback for older SDK headers/toolchains.
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        using SetProcessDpiAwarenessContextFn = BOOL(WINAPI *)(HANDLE);
        using SetProcessDPIAwareFn = BOOL(WINAPI *)();
        auto setDpiCtx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiCtx)
        {
            HANDLE kPerMonitorAwareV2 = reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4));
            setDpiCtx(kPerMonitorAwareV2);
        }
        else
        {
            auto setDpiAware = reinterpret_cast<SetProcessDPIAwareFn>(
                GetProcAddress(user32, "SetProcessDPIAware"));
            if (setDpiAware)
            {
                setDpiAware();
            }
        }
    }

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    std::wstring appTitle = PathUtils::GetProductName();
    std::wstring mutexName = BuildSingleInstanceMutexName();
    std::wstring className = L"NovadeskTrayClass_" + appTitle;
    bool requestSingleInstanceLock = false;
    bool forceNewInstance = false;
    {
        int argc = 0;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv)
        {
            for (int i = 1; i < argc; ++i)
            {
                const std::wstring arg = argv[i];
                if (arg == L"--request-single-instance-lock")
                {
                    requestSingleInstanceLock = true;
                    continue;
                }
                if (arg == L"--new-instance")
                {
                    forceNewInstance = true;
                }
            }
            LocalFree(argv);
        }
    }

    bool lockAlreadyHeld = false;
    if (!forceNewInstance)
    {
        HANDLE hExistingMutex = OpenMutexW(SYNCHRONIZE, FALSE, mutexName.c_str());
        if (hExistingMutex)
        {
            lockAlreadyHeld = true;
            CloseHandle(hExistingMutex);
        }
        else if (requestSingleInstanceLock && !RequestSingleInstanceLock())
        {
            HANDLE hRaceMutex = OpenMutexW(SYNCHRONIZE, FALSE, mutexName.c_str());
            if (hRaceMutex)
            {
                lockAlreadyHeld = true;
                CloseHandle(hRaceMutex);
            }
        }
    }

    if (lockAlreadyHeld)
    {
        // Another instance is running, check arguments for commands
        bool handledCommand = false;
        bool onlyInternalLockArg = false;
        int argc = 0;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv && argc > 1)
        {
            onlyInternalLockArg = true;
            std::wstring cmd;
            std::vector<std::wstring> loadPaths;
            std::wstring refreshPath;
            std::wstring unloadPath;
            bool refreshAll = false;
            bool listScripts = false;
            std::wstring listScriptsFile;
            std::optional<bool> setHardwareAcceleration;
            std::optional<bool> setDebugging;
            std::optional<bool> setLogging;
            std::optional<bool> setSaveLogToFile;
            for (int i = 1; i < argc; ++i)
            {
                if (!cmd.empty())
                    cmd += L" ";
                cmd += argv[i];

                const std::wstring arg = argv[i];
                if (arg != L"--request-single-instance-lock")
                {
                    onlyInternalLockArg = false;
                }
                if (arg == L"--list-scripts")
                {
                    listScripts = true;
                    continue;
                }
                if (arg == L"--new-instance")
                {
                    continue;
                }
                if (arg == L"--list-scripts-file" && i + 1 < argc)
                {
                    listScripts = true;
                    listScriptsFile = argv[++i];
                    continue;
                }
                if (arg == L"--refresh" && i + 1 < argc)
                {
                    refreshPath = argv[++i];
                    continue;
                }
                if (arg == L"--refresh-all")
                {
                    refreshAll = true;
                    continue;
                }
                if (arg == L"--unload" && i + 1 < argc)
                {
                    unloadPath = argv[++i];
                    continue;
                }
                if (arg == L"--load" && i + 1 < argc)
                {
                    loadPaths.push_back(argv[++i]);
                    continue;
                }
                if (arg == L"--enable-hardware-acceleration")
                {
                    setHardwareAcceleration = true;
                    continue;
                }
                if (arg == L"--disable-hardware-acceleration")
                {
                    setHardwareAcceleration = false;
                    continue;
                }
                if (arg == L"--enable-debugging")
                {
                    setDebugging = true;
                    continue;
                }
                if (arg == L"--disable-debugging")
                {
                    setDebugging = false;
                    continue;
                }
                if (arg == L"--enable-logging")
                {
                    setLogging = true;
                    continue;
                }
                if (arg == L"--disable-logging")
                {
                    setLogging = false;
                    continue;
                }
                if (arg == L"--enable-save-log-to-file")
                {
                    setSaveLogToFile = true;
                    continue;
                }
                if (arg == L"--disable-save-log-to-file")
                {
                    setSaveLogToFile = false;
                    continue;
                }
                if (!arg.empty() && arg[0] != L'-')
                {
                    loadPaths.push_back(arg);
                }
            }
            HWND hExisting = FindWindowW(className.c_str(), NULL);

            if (hExisting)
            {
                if (cmd.find(L"/exit") != std::wstring::npos ||
                    cmd.find(L"-exit") != std::wstring::npos ||
                    cmd.find(L"--exit") != std::wstring::npos)
                {
                    SendMessage(hExisting, WM_COMMAND, ID_TRAY_EXIT, 0);
                    handledCommand = true;
                }
                if (listScripts)
                {
                    std::wstring tempList = listScriptsFile;
                    if (tempList.empty())
                    {
                        tempList = CreateTempListPath();
                    }
                    if (!tempList.empty())
                    {
                        handledCommand = SendIpcCommand(hExisting, L"list", tempList) || handledCommand;
                        if (listScriptsFile.empty())
                        {
                            std::wifstream in(tempList.c_str());
                            if (in.is_open())
                            {
                                std::wstring line;
                                while (std::getline(in, line))
                                {
                                    if (!line.empty())
                                        std::wcout << line << std::endl;
                                }
                                in.close();
                            }
                            DeleteFileW(tempList.c_str());
                        }
                    }
                    else
                    {
                        handledCommand = SendIpcCommand(hExisting, L"list", L"") || handledCommand;
                    }
                }
                if (!refreshPath.empty())
                {
                    handledCommand = SendIpcCommand(hExisting, L"refresh", refreshPath) || handledCommand;
                }
                if (refreshAll)
                {
                    handledCommand = SendIpcCommand(hExisting, L"refresh-all", L"") || handledCommand;
                }
                if (!unloadPath.empty())
                {
                    handledCommand = SendIpcCommand(hExisting, L"unload", unloadPath) || handledCommand;
                }
                for (const auto &p : loadPaths)
                {
                    handledCommand = SendIpcCommand(hExisting, L"load", p) || handledCommand;
                }
                if (setHardwareAcceleration.has_value())
                {
                    handledCommand = SendIpcCommand(hExisting, *setHardwareAcceleration ? L"set-hardware-acceleration-on" : L"set-hardware-acceleration-off", L"") || handledCommand;
                }
                if (setDebugging.has_value())
                {
                    handledCommand = SendIpcCommand(hExisting, *setDebugging ? L"set-debugging-on" : L"set-debugging-off", L"") || handledCommand;
                }
                if (setLogging.has_value())
                {
                    handledCommand = SendIpcCommand(hExisting, *setLogging ? L"set-logging-on" : L"set-logging-off", L"") || handledCommand;
                }
                if (setSaveLogToFile.has_value())
                {
                    handledCommand = SendIpcCommand(hExisting, *setSaveLogToFile ? L"set-save-log-to-file-on" : L"set-save-log-to-file-off", L"") || handledCommand;
                }
            }
        }
        if (argv)
        {
            LocalFree(argv);
        }

        if (!handledCommand && !onlyInternalLockArg)
        {
            std::wstring message = appTitle + L" is already running.";
            MessageBoxW(nullptr, message.c_str(), appTitle.c_str(), MB_OK | MB_ICONINFORMATION);
        }

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
    FontManager::Initialize();

    // Initialize Settings
    Settings::Initialize();

    // Initialize global strings
    wcscpy_s(szTitle, MAX_LOADSTRING, appTitle.c_str());
    hInst = hInstance;

    // Ensure Ctrl+C / console close shuts down the app and unloads addons.
    SetConsoleCtrlHandler(NovadeskConsoleCtrlHandler, TRUE);

    // Create a hidden message-only window for tray icon
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        // Route custom JSEngine messages and timers
        JSEngine::OnMessage(message, wParam, lParam);

        switch (message)
        {
        case WM_TIMER:
            JSEngine::OnTimer(wParam);
            break;
        case WM_TRAYICON:
        {
            const int trayId = static_cast<int>(wParam);
            switch (lParam)
            {
            case WM_LBUTTONDBLCLK:
                DispatchTrayMouseEvent(trayId, "double-click");
                break;
            case WM_LBUTTONUP:
                DispatchTrayMouseEvent(trayId, "click");
                break;
            case WM_RBUTTONUP:
                DispatchTrayMouseEvent(trayId, "right-click");
                ShowTrayMenuInternal(trayId, nullptr, nullptr);
                break;
            case WM_MOUSEMOVE:
                break;
            default:
                break;
            }
            break;
        }
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == ID_TRAY_EXIT)
            {
                DestroyWindow(hWnd);
            }
            else if (wmId == ID_TRAY_REFRESH)
            {
                JSEngine::Reload();
            }
            else if (wmId >= 2000)
            {
                JSEngine::OnTrayCommand(wmId);
            }
        }
        break;
        case WM_COPYDATA:
        {
            const COPYDATASTRUCT *cds = reinterpret_cast<const COPYDATASTRUCT *>(lParam);
            if (!cds || !cds->lpData || cds->cbData < sizeof(wchar_t))
            {
                return FALSE;
            }
            const wchar_t *data = reinterpret_cast<const wchar_t *>(cds->lpData);
            std::wstring payload(data, cds->cbData / sizeof(wchar_t));
            if (!payload.empty() && payload.back() == L'\0')
            {
                payload.pop_back();
            }

            const size_t sep = payload.find(L'|');
            if (sep == std::wstring::npos)
            {
                return FALSE;
            }
            const std::wstring command = payload.substr(0, sep);
            const std::wstring path = payload.substr(sep + 1);
            if (command == L"refresh")
            {
                JSEngine::RefreshScript(path);
            }
            else if (command == L"refresh-all")
            {
                JSEngine::Reload();
            }
            else if (command == L"unload")
            {
                JSEngine::RemoveScript(path);
            }
            else if (command == L"load")
            {
                JSEngine::AddScript(path);
            }
            else if (command == L"list")
            {
                const std::vector<std::wstring> scripts = JSEngine::GetLoadedScripts();
                if (!path.empty())
                {
                    std::wofstream out(path.c_str(), std::ios::trunc);
                    if (out.is_open())
                    {
                        for (const auto &s : scripts)
                        {
                            out << s << L"\n";
                        }
                        out.close();
                    }
                }
            }
            else if (command == L"set-hardware-acceleration-on")
            {
                Settings::SetGlobalBool("useHardwareAcceleration", true);
            }
            else if (command == L"set-hardware-acceleration-off")
            {
                Settings::SetGlobalBool("useHardwareAcceleration", false);
            }
            else if (command == L"set-debugging-on")
            {
                Settings::SetGlobalBool("enableDebugging", true);
                Settings::ApplyGlobalSettings();
            }
            else if (command == L"set-debugging-off")
            {
                Settings::SetGlobalBool("enableDebugging", false);
                Settings::ApplyGlobalSettings();
            }
            else if (command == L"set-logging-on")
            {
                Settings::SetGlobalBool("disableLogging", false);
                Settings::ApplyGlobalSettings();
            }
            else if (command == L"set-logging-off")
            {
                Settings::SetGlobalBool("disableLogging", true);
                Settings::ApplyGlobalSettings();
            }
            else if (command == L"set-save-log-to-file-on")
            {
                Settings::SetGlobalBool("saveLogToFile", true);
                Settings::ApplyGlobalSettings();
            }
            else if (command == L"set-save-log-to-file-off")
            {
                Settings::SetGlobalBool("saveLogToFile", false);
                Settings::ApplyGlobalSettings();
            }
            return TRUE;
        }
        case WM_DESTROY:
            if (g_trayMouseHook)
            {
                UnhookWindowsHookEx(g_trayMouseHook);
                g_trayMouseHook = nullptr;
            }
            novadesk::scripting::quickjs::UnloadAllAddons();
            RemoveAllTrayIcons();
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

    if (!hWnd)
    {
        Logging::Log(LogLevel::Error, L"Failed to create message window");
        return FALSE;
    }

    // Initialize JS message routing; tray icon is lazy-initialized on first use.
    JSEngine::SetMessageWindow(hWnd);
    g_trayMessageWindow = hWnd;

    g_trayMouseHook = SetWindowsHookEx(WH_MOUSE_LL, TrayMouseHookProc, GetModuleHandle(nullptr), 0);
    if (!g_trayMouseHook)
    {
        Logging::Log(LogLevel::Warn, L"Failed to install tray mouse hook (Error: %d)", GetLastError());
    }

    // Scripting runtime context is handled by the migrated QuickJS path.
    ctx = nullptr;

    // Initialize JavaScript API
    JSEngine::InitializeJavaScriptAPI(ctx);

    // Parse command line for custom script path using argv semantics.
    // argv[0] is the executable path; the first user arg is argv[1].
    std::vector<std::wstring> scriptPaths;
    std::optional<bool> setHardwareAcceleration;
    std::optional<bool> setDebugging;
    std::optional<bool> setLogging;
    std::optional<bool> setSaveLogToFile;
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv)
    {
        if (argc > 1)
        {
            for (int i = 1; i < argc; ++i)
            {
                const std::wstring arg = argv[i];
                if (arg == L"--refresh" || arg == L"--unload")
                {
                    if (i + 1 < argc)
                        ++i;
                    continue;
                }
                if (arg == L"--new-instance")
                {
                    continue;
                }
                if (arg == L"--refresh-all")
                {
                    continue;
                }
                if (arg == L"--load")
                {
                    if (i + 1 < argc)
                    {
                        scriptPaths.push_back(argv[++i]);
                    }
                    continue;
                }
                if (arg == L"--enable-hardware-acceleration")
                {
                    setHardwareAcceleration = true;
                    continue;
                }
                if (arg == L"--disable-hardware-acceleration")
                {
                    setHardwareAcceleration = false;
                    continue;
                }
                if (arg == L"--enable-debugging")
                {
                    setDebugging = true;
                    continue;
                }
                if (arg == L"--disable-debugging")
                {
                    setDebugging = false;
                    continue;
                }
                if (arg == L"--enable-logging")
                {
                    setLogging = true;
                    continue;
                }
                if (arg == L"--disable-logging")
                {
                    setLogging = false;
                    continue;
                }
                if (arg == L"--enable-save-log-to-file")
                {
                    setSaveLogToFile = true;
                    continue;
                }
                if (arg == L"--disable-save-log-to-file")
                {
                    setSaveLogToFile = false;
                    continue;
                }
                if (!arg.empty() && arg[0] == L'-')
                    continue;
                scriptPaths.push_back(arg);
            }
        }
        LocalFree(argv);
    }

    bool appliedCliSettings = false;
    if (setHardwareAcceleration.has_value())
    {
        Settings::SetGlobalBool("useHardwareAcceleration", *setHardwareAcceleration);
        appliedCliSettings = true;
    }
    if (setDebugging.has_value())
    {
        Settings::SetGlobalBool("enableDebugging", *setDebugging);
        appliedCliSettings = true;
    }
    if (setLogging.has_value())
    {
        Settings::SetGlobalBool("disableLogging", !*setLogging);
        appliedCliSettings = true;
    }
    if (setSaveLogToFile.has_value())
    {
        Settings::SetGlobalBool("saveLogToFile", *setSaveLogToFile);
        appliedCliSettings = true;
    }
    if (appliedCliSettings)
    {
        Settings::ApplyGlobalSettings();
    }
    if (!scriptPaths.empty())
    {
        std::wstring joined;
        for (size_t i = 0; i < scriptPaths.size(); ++i)
        {
            if (i > 0)
                joined += L", ";
            joined += scriptPaths[i];
        }
        Logging::Log(LogLevel::Info, L"Using custom scripts: %s", joined.c_str());
    }

    // Load and execute script (with optional custom path)
    if (!JSEngine::LoadAndExecuteScripts(ctx, scriptPaths))
    {
        Logging::Log(LogLevel::Error, L"Script execution failed. See QuickJS exception logs above.");
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    std::vector<Widget *> widgetsCopy = widgets;
    widgets.clear();
    for (auto w : widgetsCopy)
        delete w;

    ctx = nullptr;
    System::Finalize();

    // Convert GDI+ shutdown
    FontManager::Cleanup();
    Direct2D::Cleanup();

    ReleaseSingleInstanceLock();

    return (int)msg.wParam;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nCmdShow)
{
    // Forward an empty command line; wWinMain now reads argv directly via GetCommandLineW().
    return wWinMain(hInstance, hPrevInstance, nullptr, nCmdShow);
}

static TrayState *GetTrayState(int trayId)
{
    auto it = g_trayStates.find(trayId);
    if (it == g_trayStates.end())
        return nullptr;
    return &it->second;
}

int TrayCreate(const std::wstring &path)
{
    const int trayId = g_nextTrayId++;
    TrayState state{};
    state.hWnd = g_trayMessageWindow;
    state.toolTip = szTitle;
    g_trayStates.emplace(trayId, std::move(state));

    if (!path.empty())
    {
        TraySetImage(trayId, path);
    }
    else
    {
        InitTrayIcon(trayId);
    }

    return trayId;
}

void TrayDestroy(int trayId)
{
    RemoveTrayIcon(trayId);
}

void InitTrayIcon(int trayId)
{
    TrayState *state = GetTrayState(trayId);
    if (!state)
        return;

    state->hWnd = g_trayMessageWindow;
    if (state->initialized)
    {
        return;
    }
    if (!state->icon)
    {
        state->icon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOVADESK));
        state->iconOwned = false;
    }

    state->nid = {};
    state->nid.cbSize = sizeof(NOTIFYICONDATAW);
    state->nid.hWnd = state->hWnd;
    state->nid.uID = static_cast<UINT>(trayId);
    state->nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    state->nid.uCallbackMessage = WM_TRAYICON;
    state->nid.hIcon = state->icon;
    const std::wstring tip = state->toolTip.empty() ? std::wstring(szTitle) : state->toolTip;
    wcsncpy_s(state->nid.szTip, _countof(state->nid.szTip), tip.c_str(), _TRUNCATE);

    state->initialized = true;
    if (Settings::GetGlobalBool("hideTrayIcon", false))
    {
        Logging::Log(LogLevel::Info, L"Tray icon hidden by settings");
        return;
    }

    Shell_NotifyIconW(NIM_ADD, &state->nid);
    Logging::Log(LogLevel::Info, L"Tray icon initialized (id=%d)", trayId);
}

void RemoveTrayIcon(int trayId)
{
    TrayState *state = GetTrayState(trayId);
    if (!state)
        return;

    if (state->initialized)
    {
        Shell_NotifyIconW(NIM_DELETE, &state->nid);
    }
    if (state->iconOwned && state->icon)
    {
        DestroyIcon(state->icon);
    }
    g_trayStates.erase(trayId);
    Logging::Log(LogLevel::Info, L"Tray icon removed (id=%d)", trayId);
}

void RemoveAllTrayIcons()
{
    std::vector<int> ids;
    ids.reserve(g_trayStates.size());
    for (auto &kv : g_trayStates)
        ids.push_back(kv.first);
    for (int id : ids)
        RemoveTrayIcon(id);
}

void TraySetImage(int trayId, const std::wstring &path)
{
    TrayState *state = GetTrayState(trayId);
    if (!state)
        return;

    if (!state->initialized)
    {
        InitTrayIcon(trayId);
    }

    HICON newIcon = nullptr;
    bool owned = false;
    if (!path.empty())
    {
        newIcon = (HICON)LoadImageW(nullptr, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        owned = (newIcon != nullptr);
        if (!newIcon)
        {
            Logging::Log(LogLevel::Warn, L"Tray setImage failed to load: %s", path.c_str());
            return;
        }
    }
    else
    {
        newIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOVADESK));
        owned = false;
    }

    if (state->iconOwned && state->icon)
    {
        DestroyIcon(state->icon);
    }
    state->icon = newIcon;
    state->iconOwned = owned;
    state->nid.hIcon = newIcon;
    state->nid.uFlags = NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &state->nid);
}

void TraySetToolTip(int trayId, const std::wstring &toolTip)
{
    TrayState *state = GetTrayState(trayId);
    if (!state)
        return;
    if (!state->initialized)
    {
        InitTrayIcon(trayId);
    }
    state->toolTip = toolTip;
    state->nid.cbSize = sizeof(NOTIFYICONDATAW);
    state->nid.uFlags = NIF_TIP;
    wcsncpy_s(state->nid.szTip, _countof(state->nid.szTip), toolTip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &state->nid);
}

void TraySetContextMenu(int trayId, const std::vector<MenuItem> &menu)
{
    TrayState *state = GetTrayState(trayId);
    if (!state)
        return;
    if (!state->initialized)
    {
        InitTrayIcon(trayId);
    }
    state->menu = menu;
}

static void ShowTrayMenuInternal(int trayId, const std::vector<MenuItem> *menu, const POINT *position)
{
    TrayState *state = GetTrayState(trayId);
    if (!state || !state->hWnd)
        return;
    const std::vector<MenuItem> &useMenu = menu ? *menu : state->menu;
    if (useMenu.empty())
        return;

    POINT pt{};
    if (position)
    {
        pt = *position;
    }
    else
    {
        GetCursorPos(&pt);
    }

    HMENU hMenu = CreatePopupMenu();
    MenuUtils::BuildMenu(hMenu, useMenu);
    SetForegroundWindow(state->hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, state->hWnd, nullptr);
    DestroyMenu(hMenu);
}

static void DispatchTrayMouseEvent(int trayId, const char *name)
{
    (void)name;
    JSEngine::DispatchTrayEvent(trayId, name);
}
