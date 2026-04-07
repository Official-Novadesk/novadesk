#pragma once

#include <string>
#include <vector>
#include <windows.h>

#include "quickjs.h"

struct duk_hthread;
using duk_context = duk_hthread;

class Widget;

namespace JSEngine
{
    struct MouseEventData
    {
        int clientX = 0;
        int clientY = 0;
        int screenX = 0;
        int screenY = 0;
        int offsetX = 0;
        int offsetY = 0;
        int offsetXPercent = 0;
        int offsetYPercent = 0;
    };

    void InitializeJavaScriptAPI(duk_context *ctx);
    bool LoadAndExecuteScript(duk_context *ctx, const std::wstring &scriptPath = L"");
    bool LoadAndExecuteScripts(duk_context *ctx, const std::vector<std::wstring> &scriptPaths);
    std::wstring GetEntryScriptDir();
    std::wstring GetCurrentScriptDir();
    std::wstring GetCurrentScriptPath();
    void RegisterWidgetOwner(Widget *widget, const std::wstring &scriptPath);
    void RegisterTrayOwner(int trayId, const std::wstring &scriptPath);
    void UnregisterTrayOwner(int trayId);
    void Reload();
    bool AddScript(const std::wstring &scriptPath);
    bool RemoveScript(const std::wstring &scriptPath);
    bool RefreshScript(const std::wstring &scriptPath);
    std::vector<std::wstring> GetLoadedScripts();

    void OnTimer(UINT_PTR id);
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void SetMessageWindow(HWND hWnd);
    HWND GetMessageWindow();

    void OnTrayCommand(int commandId);
    void DispatchTrayEvent(int trayId, const std::string &eventName);
    void OnWidgetContextCommand(const std::wstring &widgetId, int commandId);

    void TriggerWidgetEvent(Widget *widget, const char *eventName, const MouseEventData *data = nullptr);
    void CallEventCallback(int callbackId, Widget *widget = nullptr, const MouseEventData *data = nullptr);
    int RegisterEventCallback(JSContext *ctx, JSValueConst fn);
    bool RegisterWidgetEventListener(JSContext *ctx, Widget *widget, const std::string &eventName, JSValueConst fn);
    bool RegisterWidgetContextMenuCallback(JSContext *ctx, const std::wstring &widgetId, int commandId, JSValueConst fn);
    void ClearWidgetContextMenuCallbacks(const std::wstring &widgetId);
    bool RegisterTrayCommandCallback(JSContext *ctx, int trayId, int commandId, JSValueConst fn);
    void ClearTrayCommandCallbacks(int trayId);
    void ClearAllTrayCommandCallbacks();
    bool RegisterTrayEventCallback(JSContext *ctx, int trayId, const std::string &eventName, JSValueConst fn);
    void ClearTrayEventCallbacks(int trayId);
    void ClearAllTrayEventCallbacks();
    bool ExecuteWidgetScript(Widget *widget);
    JSValue CreateUiIpcObject(JSContext *ctx);

    static const UINT WM_NOVADESK_DISPATCH = WM_USER + 101;
} // namespace JSEngine
