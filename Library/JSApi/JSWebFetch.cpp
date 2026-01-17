/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSWebFetch.h"
#include "JSEvents.h"
#include "../Utils.h"
#include "../Logging.h"
#include "../FileUtils.h"
#include "../TimerManager.h"
#include "../PathUtils.h"
#include "JSUtils.h"
#include <windows.h>
#include <wininet.h>
#include <thread>
#include <string>
#include <vector>
#include <mutex>

#pragma comment(lib, "wininet.lib")

namespace JSApi {

    static const UINT WM_WEB_FETCH_COMPLETE = WM_USER + 105;

    struct FetchResult {
        int callbackId;
        std::string data;
        bool success;
    };

    static std::mutex g_FetchMutex;
    static std::vector<FetchResult> g_PendingResults;

    void FetchThreadProc(std::wstring url, int callbackId, HWND hNotifyWnd) {
        std::string resultData;
        bool success = false;

        // Check if it's a local file
        if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && url.find(L"file://") != 0) {
            // Treat as local file path
            resultData = FileUtils::ReadFileContent(url);
            success = !resultData.empty();
        } else if (url.find(L"file://") == 0) {
            // Strip file:// prefix
            std::wstring path = url.substr(7);
            // Handle optional leading slash (file:///C:/...)
            if (path.length() > 0 && path[0] == L'/') {
                if (path.length() > 2 && path[2] == L':') {
                    path = path.substr(1); // /C:/... -> C:/...
                }
            }
            resultData = FileUtils::ReadFileContent(path);
            success = !resultData.empty();
        } else {
            // Use WinINet for HTTP/HTTPS
            HINTERNET hInternet = InternetOpenW(L"Novadesk WebFetch", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (hInternet) {
                HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
                if (hUrl) {
                    char buffer[4096];
                    DWORD bytesRead = 0;
                    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                        resultData.append(buffer, bytesRead);
                    }
                    InternetCloseHandle(hUrl);
                    success = true;
                }
                InternetCloseHandle(hInternet);
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_FetchMutex);
            g_PendingResults.push_back({ callbackId, resultData, success });
        }

        if (hNotifyWnd) {
            PostMessage(hNotifyWnd, WM_WEB_FETCH_COMPLETE, 0, 0);
        }
    }

    duk_ret_t js_system_fetch(duk_context* ctx) {
        if (duk_get_top(ctx) < 2) return DUK_RET_TYPE_ERROR;

        std::wstring url = Utils::ToWString(duk_get_string(ctx, 0));
        if (!duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;

        // Resolve local paths relative to the script directory if it's not a URL
        if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && url.find(L"file://") != 0 && PathUtils::IsPathRelative(url)) {
            url = ResolveScriptPath(ctx, url);
        }

        int callbackId = RegisterEventCallback(ctx, 1);
        HWND hNotifyWnd = TimerManager::GetWindow();

        std::thread fetchThread(FetchThreadProc, url, callbackId, hNotifyWnd);
        fetchThread.detach();

        return 0;
    }

    void HandleWebFetchMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_WEB_FETCH_COMPLETE) {
            std::vector<FetchResult> results;
            {
                std::lock_guard<std::mutex> lock(g_FetchMutex);
                results.swap(g_PendingResults);
            }

            for (const auto& res : results) {
                if (!s_JsContext) continue;

                duk_push_global_stash(s_JsContext);
                if (duk_get_prop_string(s_JsContext, -1, "__events")) {
                    duk_push_int(s_JsContext, res.callbackId);
                    if (duk_get_prop(s_JsContext, -2)) {
                        if (duk_is_function(s_JsContext, -1)) {
                            if (res.success) {
                                duk_push_string(s_JsContext, res.data.c_str());
                            } else {
                                duk_push_null(s_JsContext);
                            }
                            
                            if (duk_pcall(s_JsContext, 1) != 0) {
                                Logging::Log(LogLevel::Error, L"WebFetch callback error: %S", duk_safe_to_string(s_JsContext, -1));
                            }
                        }
                        duk_pop(s_JsContext); // Pop result or error
                    } else {
                        duk_pop(s_JsContext); // Pop undefined
                    }
                    
                    // Unregister callback to prevent leak
                    duk_push_int(s_JsContext, res.callbackId);
                    duk_del_prop(s_JsContext, -2);
                }
                duk_pop_2(s_JsContext); // Pop __events, stash
            }
        }
    }

    void BindWebFetch(duk_context* ctx) {
        // "system" object is expected to be on top of stack
        duk_push_c_function(ctx, js_system_fetch, 2);
        duk_put_prop_string(ctx, -2, "fetch");
    }
}
