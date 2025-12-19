#include "framework.h"
#include "Novadesk.h"
#include "duktape/duktape.h"
#include "Widget.h"
#include "System.h"
#include "Logging.h"
#include "ColorUtil.h"
#include <vector>
#include <fstream>
#include <sstream>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
duk_context *ctx = nullptr;
std::vector<Widget*> widgets;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Helper to convert std::string to std::wstring
std::wstring ToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// JS API: novadesk.log(...)
static duk_ret_t js_log(duk_context *ctx) {
    duk_idx_t n = duk_get_top(ctx);
    std::wstring msg;
    for (duk_idx_t i = 0; i < n; i++) {
        if (i > 0) msg += L" ";
        msg += ToWString(duk_safe_to_string(ctx, i));
    }
    Logging::Log(LogLevel::Info, L"%s", msg.c_str());
    return 0;
}

// JS API: novadesk.error(...)
static duk_ret_t js_error(duk_context *ctx) {
    duk_idx_t n = duk_get_top(ctx);
    std::wstring msg;
    for (duk_idx_t i = 0; i < n; i++) {
        if (i > 0) msg += L" ";
        msg += ToWString(duk_safe_to_string(ctx, i));
    }
    Logging::Log(LogLevel::Error, L"%s", msg.c_str());
    return 0;
}

// JS API: novadesk.debug(...)
static duk_ret_t js_debug(duk_context *ctx) {
    duk_idx_t n = duk_get_top(ctx);
    std::wstring msg;
    for (duk_idx_t i = 0; i < n; i++) {
        if (i > 0) msg += L" ";
        msg += ToWString(duk_safe_to_string(ctx, i));
    }
    Logging::Log(LogLevel::Debug, L"%s", msg.c_str());
    return 0;
}

// JS API: novadesk.createWidgetWindow(options)
static duk_ret_t js_create_widget_window(duk_context *ctx) {
    Logging::Log(LogLevel::Debug, L"js_create_widget_window called");
    if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

    WidgetOptions options;
    options.width = 400;
    options.height = 300;
    options.backgroundColor = L"rgba(255,255,255,255)";
    options.zPos = ZPOSITION_NORMAL;
    options.alpha = 255;
    options.color = RGB(255, 255, 255);

    if (duk_get_prop_string(ctx, 0, "width")) options.width = duk_get_int(ctx, -1);
    duk_pop(ctx);
    if (duk_get_prop_string(ctx, 0, "height")) options.height = duk_get_int(ctx, -1);
    duk_pop(ctx);
    if (duk_get_prop_string(ctx, 0, "backgroundColor")) {
        options.backgroundColor = ToWString(duk_get_string(ctx, -1));
        ColorUtil::ParseRGBA(options.backgroundColor, options.color, options.alpha);
    }
    duk_pop(ctx);
    if (duk_get_prop_string(ctx, 0, "zPos")) {
        std::string zPosStr = duk_get_string(ctx, -1);
        if (zPosStr == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
        else if (zPosStr == "ontop") options.zPos = ZPOSITION_ONTOP;
        else if (zPosStr == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
        else if (zPosStr == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
    }
    duk_pop(ctx);

    Widget* widget = new Widget(options);
    if (widget->Create()) {
        widget->Show();
        widgets.push_back(widget);
        Logging::Log(LogLevel::Info, L"Widget created and shown. HWND: 0x%p", widget->GetWindow());
    } else {
        Logging::Log(LogLevel::Error, L"Failed to create widget.");
        delete widget;
    }

    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Logging::Log(LogLevel::Info, L"Application starting...");
    System::Initialize(hInstance);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOVADESK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    // Initialize Duktape
    Logging::Log(LogLevel::Info, L"Initializing Duktape (version %ld)...", (long)DUK_VERSION);
    ctx = duk_create_heap_default();
    if (!ctx) {
        Logging::Log(LogLevel::Error, L"Failed to create Duktape heap.");
        return FALSE;
    }

    // Register novadesk object and methods
    duk_push_object(ctx);
    duk_push_c_function(ctx, js_create_widget_window, 1);
    duk_put_prop_string(ctx, -2, "createWidgetWindow");
    duk_push_c_function(ctx, js_log, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "log");
    duk_push_c_function(ctx, js_error, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "error");
    duk_push_c_function(ctx, js_debug, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "debug");
    duk_put_global_string(ctx, "novadesk");

    // Bootstrap widgetWindow class
    const char* bootstrap = "var widgetWindow = function(options) { novadesk.createWidgetWindow(options); };";
    duk_eval_string(ctx, bootstrap);

    // Get executable path to find index.js
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath = path;
    size_t lastBackslash = exePath.find_last_of(L"\\");
    std::wstring scriptPath = exePath.substr(0, lastBackslash + 1) + L"index.js";

    Logging::Log(LogLevel::Info, L"Loading script from: %s", scriptPath.c_str());

    std::ifstream t(scriptPath);
    if (t.is_open()) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::string content = buffer.str();
        Logging::Log(LogLevel::Info, L"Script loaded, size: %zu", content.size());
        if (duk_peval_string(ctx, content.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
        } else {
            Logging::Log(LogLevel::Info, L"Script execution successful. Calling novadeskAppReady()...");
            
            // Call novadeskAppReady if defined
            duk_get_global_string(ctx, "novadeskAppReady");
            if (duk_is_function(ctx, -1)) {
                if (duk_pcall(ctx, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"novadeskAppReady failed: %S", duk_safe_to_string(ctx, -1));
                }
            } else {
                Logging::Log(LogLevel::Info, L"novadeskAppReady not defined.");
            }
            duk_pop(ctx);
        }
        duk_pop(ctx);
    } else {
        Logging::Log(LogLevel::Error, L"Failed to open index.js at %s", scriptPath.c_str());
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOVADESK));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup
    for (auto w : widgets) delete w;
    duk_destroy_heap(ctx);
    System::Finalize();

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOVADESK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NOVADESK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_NOVADESK));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
