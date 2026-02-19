/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "JSCommon.h"
#include "JSUtils.h"
#include "JSSystem.h"
#include "JSWidget.h"
#include "JSElement.h"
#include "JSContextMenu.h"
#include "JSEvents.h"
#include "JSIPC.h"
#include "JSJson.h"
#include "JSPath.h"
#include "JSNovadeskTray.h"
#include "JSWebFetch.h"
#include "JSAudio.h"
#include "JSRegistry.h"
#include "JSClipboard.h"
#include "JSPower.h"
#include "../Widget.h"
#include "../Settings.h"
#include "../Logging.h"
#include "../PathUtils.h"
#include "../FileUtils.h"
#include "../TimerManager.h"
#include "../Hotkey.h"
#include "../Utils.h"

#include <vector>
#include <algorithm>

extern std::vector<Widget*> widgets;

namespace JSApi {

    static HWND s_MessageWindow = nullptr;
    static bool WrapExportedFunctionWithModuleDir(duk_context* ctx, duk_idx_t fnIndex, const std::string& dirname);
    static void WrapModuleExportsWithDirContext(duk_context* ctx, duk_idx_t exportsIndex, const std::string& dirname);

    static bool IsMainScriptPath(const std::wstring& scriptPath) {
        std::wstring normalized = PathUtils::NormalizePath(scriptPath);
        size_t slash = normalized.find_last_of(L"/\\");
        std::wstring filename = (slash == std::wstring::npos) ? normalized : normalized.substr(slash + 1);
        return filename == L"index.js";
    }

    static bool HasJsExtension(const std::wstring& path) {
        size_t slash = path.find_last_of(L"/\\");
        size_t dot = path.find_last_of(L'.');
        if (dot == std::wstring::npos) return false;
        if (slash != std::wstring::npos && dot < slash) return false;
        return true;
    }

    static void PushRequireStackPath(duk_context* ctx, const std::string& path) {
        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__require_stack")) {
            duk_pop(ctx);
            duk_push_array(ctx);
            duk_put_prop_string(ctx, -2, "__require_stack");
            duk_get_prop_string(ctx, -1, "__require_stack");
        }

        duk_uarridx_t len = (duk_uarridx_t)duk_get_length(ctx, -1);
        duk_push_string(ctx, path.c_str());
        duk_put_prop_index(ctx, -2, len);
        duk_pop_2(ctx);
    }

    static void PopRequireStackPath(duk_context* ctx) {
        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "__require_stack")) {
            duk_uarridx_t len = (duk_uarridx_t)duk_get_length(ctx, -1);
            if (len > 0) {
                duk_set_length(ctx, -1, (duk_size_t)(len - 1));
            }
        }
        duk_pop_2(ctx);
    }

    static bool GetRequireBasePath(duk_context* ctx, std::wstring& outPath) {
        outPath.clear();
        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "__require_stack")) {
            duk_uarridx_t len = (duk_uarridx_t)duk_get_length(ctx, -1);
            if (len > 0) {
                duk_get_prop_index(ctx, -1, len - 1);
                if (duk_is_string(ctx, -1)) {
                    outPath = Utils::ToWString(duk_get_string(ctx, -1));
                }
                duk_pop(ctx);
            }
        }
        duk_pop_2(ctx);
        return !outPath.empty();
    }

    static bool IsInRequireStack(duk_context* ctx, const std::string& pathKey) {
        bool found = false;
        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "__require_stack")) {
            duk_uarridx_t len = (duk_uarridx_t)duk_get_length(ctx, -1);
            for (duk_uarridx_t i = 0; i < len; ++i) {
                duk_get_prop_index(ctx, -1, i);
                if (duk_is_string(ctx, -1)) {
                    const char* cur = duk_get_string(ctx, -1);
                    if (cur && pathKey == cur) {
                        found = true;
                        duk_pop(ctx);
                        break;
                    }
                }
                duk_pop(ctx);
            }
        }
        duk_pop_2(ctx);
        return found;
    }

    static duk_ret_t js_require(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;

        std::wstring request = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring basePath;
        std::wstring fallbackBaseDir;
        if (!GetRequireBasePath(ctx, basePath)) {
            // For deferred callbacks, prefer the current global __dirname context.
            duk_get_global_string(ctx, "__dirname");
            if (duk_is_string(ctx, -1)) {
                fallbackBaseDir = Utils::ToWString(duk_get_string(ctx, -1));
            }
            duk_pop(ctx);

            basePath = s_CurrentScriptPath;
        }

        if (basePath.empty() && fallbackBaseDir.empty()) {
            basePath = PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
        }

        std::wstring baseDir = fallbackBaseDir.empty()
            ? PathUtils::GetParentDir(basePath)
            : fallbackBaseDir;
        std::wstring resolved = request;
        if (!HasJsExtension(resolved)) {
            resolved += L".js";
        }

        if (PathUtils::IsPathRelative(resolved)) {
            resolved = PathUtils::ResolvePath(resolved, baseDir);
        } else {
            resolved = PathUtils::ResolvePath(resolved);
        }

        resolved = PathUtils::NormalizePath(resolved);
        std::string resolvedKey = Utils::ToString(resolved);

        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__module_cache")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__module_cache");
            duk_get_prop_string(ctx, -1, "__module_cache");
        }

        if (IsInRequireStack(ctx, resolvedKey)) {
            Logging::Log(LogLevel::Warn, L"require: circular dependency detected for %s", resolved.c_str());
            if (duk_get_prop_string(ctx, -1, resolvedKey.c_str())) {
                duk_remove(ctx, -2); // remove cache object, leave exports
                duk_remove(ctx, -2); // remove stash
                return 1;
            }
            duk_pop(ctx); // cached value
            duk_pop_2(ctx); // cache + stash
            duk_push_object(ctx); // fallback empty exports
            return 1;
        }

        if (duk_get_prop_string(ctx, -1, resolvedKey.c_str())) {
            duk_remove(ctx, -2); // remove cache object, leave exports
            duk_remove(ctx, -2); // remove stash
            return 1;
        }
        duk_pop(ctx); // cached value

        // Create module + exports and cache immediately to support circular deps
        duk_push_object(ctx); // module
        duk_push_object(ctx); // exports
        duk_dup(ctx, -1);
        duk_put_prop_string(ctx, -3, "exports");
        duk_dup(ctx, -1); // exports
        duk_put_prop_string(ctx, -4, resolvedKey.c_str()); // cache[path] = exports

        std::string content = FileUtils::ReadFileContent(resolved);
        if (content.empty()) {
            duk_pop_2(ctx);
            duk_error(ctx, DUK_ERR_ERROR, "require: failed to load module '%s'", resolvedKey.c_str());
            return DUK_RET_ERROR;
        }

        std::string wrapped = "(function(require, module, exports, __filename, __dirname) {\n";
        wrapped += content;
        wrapped += "\n})";

        if (duk_peval_string(ctx, wrapped.c_str()) != 0) {
            std::string err = duk_safe_to_string(ctx, -1);
            duk_pop(ctx); // error
            duk_del_prop_string(ctx, -3, resolvedKey.c_str()); // remove cached exports
            duk_pop_2(ctx); // cache + stash
            duk_error(ctx, DUK_ERR_ERROR, "require: failed to compile '%s': %s", resolvedKey.c_str(), err.c_str());
            return DUK_RET_ERROR;
        }

        std::string dirname = Utils::ToString(PathUtils::GetParentDir(resolved));
        if (dirname.length() > 3 && dirname.back() == '\\') dirname.pop_back();

        PushRequireStackPath(ctx, resolvedKey);

        duk_push_c_function(ctx, js_require, 1); // require
        duk_dup(ctx, -4); // module
        duk_dup(ctx, -4); // exports
        duk_push_string(ctx, resolvedKey.c_str()); // __filename
        duk_push_string(ctx, dirname.c_str()); // __dirname

        if (duk_pcall(ctx, 5) != 0) {
            std::string err = duk_safe_to_string(ctx, -1);
            duk_pop(ctx); // error
            PopRequireStackPath(ctx);
            duk_del_prop_string(ctx, -3, resolvedKey.c_str()); // remove cached exports
            duk_pop_2(ctx); // cache, stash
            duk_error(ctx, DUK_ERR_ERROR, "require: error in '%s': %s", resolvedKey.c_str(), err.c_str());
            return DUK_RET_ERROR;
        }
        duk_pop(ctx); // wrapper result

        PopRequireStackPath(ctx);

        // module.exports may have been replaced
        duk_get_prop_string(ctx, -2, "exports"); // module.exports

        // Preserve module-local relative path semantics for exported functions
        // invoked later from other scripts.
        WrapModuleExportsWithDirContext(ctx, -1, dirname);

        // cache[modulePath] = module.exports (update in case replaced)
        duk_dup(ctx, -1);
        duk_put_prop_string(ctx, -5, resolvedKey.c_str());

        duk_remove(ctx, -2); // exports
        duk_remove(ctx, -2); // module
        duk_remove(ctx, -2); // cache
        duk_remove(ctx, -2); // stash
        return 1;
    }

    void InitializeJavaScriptAPI(duk_context* ctx) {
        s_JsContext = ctx;

        // Register console object
        duk_push_object(ctx);
        BindConsoleMethods(ctx);
        duk_put_global_string(ctx, "console");

        // Register app object
        duk_push_object(ctx);
        BindNovadeskAppMethods(ctx);
        BindNovadeskTrayMethods(ctx);
        duk_put_global_string(ctx, "app");

        // Initialize Global Stash for Object Tracking
        duk_push_global_stash(ctx);
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "widget_objects");
        duk_pop(ctx);

        // Register Managers
        HotkeyManager::SetCallbackHandler([](int idx) {
            CallHotkeyCallback(idx);
        });
        TimerManager::SetCallbackHandler([](int idx) {
            CallStoredCallback(idx);
        });

        // Register system object
        duk_push_object(ctx);
        BindSystemBaseMethods(ctx);
        BindJsonMethods(ctx);
        BindSystemMonitors(ctx);
        BindWebFetch(ctx);
        BindAudioMethods(ctx);
        BindRegistryMethods(ctx);
        BindClipboardMethods(ctx);
        BindPowerMethods(ctx);
        duk_put_global_string(ctx, "system");

        // Register timers
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 1);
        duk_put_global_string(ctx, "setInterval");
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 0);
        duk_put_global_string(ctx, "setTimeout");
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearInterval");
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearTimeout");
        duk_push_c_function(ctx, js_set_immediate, 1);
        duk_put_global_string(ctx, "setImmediate");

        // Register widgetWindow
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        // Register global modules
        BindPathMethods(ctx);
        
        duk_push_c_function(ctx, js_include, 1);
        duk_put_global_string(ctx, "include");

        BindIPCMethods(ctx);

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath) {
        Settings::ApplyGlobalSettings();
        std::wstring finalScriptPath;
        if (!scriptPath.empty()) {
            finalScriptPath = scriptPath;
            if (PathUtils::IsPathRelative(finalScriptPath)) {
                finalScriptPath = PathUtils::ResolvePath(finalScriptPath);
            }
            s_CurrentScriptPath = finalScriptPath;
        } else {
            if (s_CurrentScriptPath.empty()) {
                finalScriptPath = PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
                s_CurrentScriptPath = finalScriptPath;
            } else {
                finalScriptPath = s_CurrentScriptPath;
            }
        }

        Logging::Log(LogLevel::Info, L"Loading script from: %s", finalScriptPath.c_str());
        std::string content = FileUtils::ReadFileContent(finalScriptPath);
        if (content.empty()) {
            Logging::Log(LogLevel::Error, L"Failed to open script at %s", finalScriptPath.c_str());
            return false;
        }

        // Provide __dirname and __filename
        std::string dirname = Utils::ToString(PathUtils::GetParentDir(finalScriptPath));
        if (dirname.length() > 3 && dirname.back() == '\\') dirname.pop_back();
        std::string filename = Utils::ToString(finalScriptPath);
        duk_push_string(ctx, dirname.c_str());
        duk_put_global_string(ctx, "__dirname");
        duk_push_string(ctx, dirname.c_str());
        duk_put_global_string(ctx, "__widgetsDir");
        duk_push_string(ctx, filename.c_str());
        duk_put_global_string(ctx, "__filename");

        if (IsMainScriptPath(finalScriptPath)) {
            duk_push_c_function(ctx, js_require, 1);
            duk_put_global_string(ctx, "require");
        } else {
            duk_push_global_object(ctx);
            duk_del_prop_string(ctx, -1, "require");
            duk_pop(ctx);
        }

        // Wrap the script in an IIFE to prevent global scope pollution
        std::string wrappedContent = "(function() {\n";
        wrappedContent += content;
        wrappedContent += "\n})();";

        if (duk_peval_string(ctx, wrappedContent.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
            duk_pop(ctx);
            return false;
        }

        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "readyCallback")) {
            if (duk_is_function(ctx, -1)) {
                if (duk_pcall(ctx, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Ready callback failed: %S", duk_safe_to_string(ctx, -1));
                }
            }
        }
        duk_pop_2(ctx);
        return true;
    }

    void ReloadScripts(duk_context* ctx) {
        Logging::Log(LogLevel::Info, L"Reloading scripts...");
        HotkeyManager::UnregisterAll();
        TimerManager::ClearAll();
        std::vector<Widget*> widgetsCopy = widgets;
        widgets.clear();
        for (auto w : widgetsCopy) delete w;

        duk_push_global_stash(ctx);
        duk_del_prop_string(ctx, -1, "__hotkeys");
        duk_del_prop_string(ctx, -1, "__timers");
        duk_del_prop_string(ctx, -1, "__trayCallbacks");
        duk_del_prop_string(ctx, -1, "__module_cache");
        duk_del_prop_string(ctx, -1, "__require_stack");
        duk_pop(ctx);
 
        CleanupAddons();

        s_NextTempId = 1;
        s_NextTimerId = 1;
        s_NextImmId = 1;

        LoadAndExecuteScript(ctx, L"");
    }

    void Reload() {
        if (s_JsContext) ReloadScripts(s_JsContext);
    }

    void OnTimer(UINT_PTR id) {
        TimerManager::HandleTimer(id);
    }
 
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        static const UINT WM_DISPATCH_IPC = WM_USER + 102;

        if (message == WM_NOVADESK_DISPATCH) {
            typedef void (*DispatchFn)(void*);
            DispatchFn fn = (DispatchFn)wParam;
            if (fn) fn((void*)lParam);
            return;
        }

        if (message == WM_DISPATCH_IPC) {
            DispatchPendingIPC(s_JsContext);
            return;
        }
        TimerManager::HandleMessage(message, wParam, lParam);
        HandleWebFetchMessage(message, wParam, lParam);
    }
 
    void SetMessageWindow(HWND hWnd) {
        s_MessageWindow = hWnd;
        TimerManager::Initialize(hWnd);
    }

    HWND GetMessageWindow() {
        return s_MessageWindow;
    }

        static bool WrapExportedFunctionWithModuleDir(duk_context* ctx, duk_idx_t fnIndex, const std::string& dirname) {
        fnIndex = duk_normalize_index(ctx, fnIndex);
        if (!duk_is_function(ctx, fnIndex)) {
            return false;
        }

        // Build: (function(fn, dirname){ return function(){ ... }})
        if (duk_peval_string(ctx,
            "(function(fn, dirname) {"
            "  return function() {"
            "    var oldDir = __dirname;"
            "    __dirname = dirname;"
            "    try {"
            "      return fn.apply(this, arguments);"
            "    } finally {"
            "      __dirname = oldDir;"
            "    }"
            "  };"
            "})") != 0) {
            duk_pop(ctx);
            return false;
        }

        duk_dup(ctx, fnIndex);
        duk_push_string(ctx, dirname.c_str());
        if (duk_pcall(ctx, 2) != 0) {
            duk_pop(ctx);
            return false;
        }
        return true;
    }

    static void WrapModuleExportsWithDirContext(duk_context* ctx, duk_idx_t exportsIndex, const std::string& dirname) {
        exportsIndex = duk_normalize_index(ctx, exportsIndex);
        if (duk_is_function(ctx, exportsIndex)) {
            if (WrapExportedFunctionWithModuleDir(ctx, exportsIndex, dirname)) {
                duk_replace(ctx, exportsIndex);
            }
            return;
        }

        if (!duk_is_object(ctx, exportsIndex)) {
            return;
        }

        duk_enum(ctx, exportsIndex, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1)) {
            // [ ... enum key value ]
            if (duk_is_string(ctx, -2) && duk_is_function(ctx, -1)) {
                std::string key = duk_get_string(ctx, -2);
                if (WrapExportedFunctionWithModuleDir(ctx, -1, dirname)) {
                    // wrapped function on top
                    duk_put_prop_string(ctx, exportsIndex, key.c_str());
                } else {
                    duk_pop(ctx); // pop error/partial wrapper result
                }
            }
            duk_pop_2(ctx); // key, value
        }
        duk_pop(ctx); // enum
    }
    
    // ==============================================
    // Host API Implementation
    // ==============================================
    
    // Helper to wrap addon functions
    static duk_ret_t addon_function_wrapper(duk_context* ctx) {
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "\xff" "fn");
        typedef int (*AddonFn)(void*);
        AddonFn fn = (AddonFn)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (fn) {
            return (duk_ret_t)fn(ctx);
        }
        return 0;
    }

    static void host_RegisterString(void* ctx, const char* name, const char* value) {
        if (value) duk_push_string((duk_context*)ctx, value);
        else duk_push_null((duk_context*)ctx);
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterNumber(void* ctx, const char* name, double value) {
        duk_push_number((duk_context*)ctx, value);
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterBool(void* ctx, const char* name, int value) {
        duk_push_boolean((duk_context*)ctx, value != 0);
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterObjectStart(void* ctx, const char* name) {
        duk_push_object((duk_context*)ctx);
    }

    static void host_RegisterObjectEnd(void* ctx, const char* name) {
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterArrayString(void* ctx, const char* name, const char** values, size_t count) {
        duk_idx_t arr_idx = duk_push_array((duk_context*)ctx);
        for (size_t i = 0; i < count; ++i) {
            duk_push_string((duk_context*)ctx, values[i]);
            duk_put_prop_index((duk_context*)ctx, arr_idx, (duk_uarridx_t)i);
        }
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterArrayNumber(void* ctx, const char* name, const double* values, size_t count) {
        duk_idx_t arr_idx = duk_push_array((duk_context*)ctx);
        for (size_t i = 0; i < count; ++i) {
            duk_push_number((duk_context*)ctx, values[i]);
            duk_put_prop_index((duk_context*)ctx, arr_idx, (duk_uarridx_t)i);
        }
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_RegisterFunction(void* ctx, const char* name, int (*func)(void*), int nargs) {
        duk_push_c_function((duk_context*)ctx, addon_function_wrapper, (duk_idx_t)nargs);
        duk_push_pointer((duk_context*)ctx, (void*)func);
        duk_put_prop_string((duk_context*)ctx, -2, "\xff" "fn");
        duk_put_prop_string((duk_context*)ctx, -2, name);
    }

    static void host_PushString(void* ctx, const char* value) { duk_push_string((duk_context*)ctx, value); }
    static void host_PushNumber(void* ctx, double value) { duk_push_number((duk_context*)ctx, value); }
    static void host_PushBool(void* ctx, int value) { duk_push_boolean((duk_context*)ctx, value != 0); }
    static void host_PushNull(void* ctx) { duk_push_null((duk_context*)ctx); }
    static void host_PushObject(void* ctx) { duk_push_object((duk_context*)ctx); }

    static double host_GetNumber(void* ctx, int index) { return duk_get_number((duk_context*)ctx, (duk_idx_t)index); }
    static const char* host_GetString(void* ctx, int index) { return duk_get_string((duk_context*)ctx, (duk_idx_t)index); }
    static int host_GetBool(void* ctx, int index) { return duk_get_boolean((duk_context*)ctx, (duk_idx_t)index); }

    static int host_IsNumber(void* ctx, int index) { return duk_is_number((duk_context*)ctx, (duk_idx_t)index); }
    static int host_IsString(void* ctx, int index) { return duk_is_string((duk_context*)ctx, (duk_idx_t)index); }
    static int host_IsBool(void* ctx, int index) { return duk_is_boolean((duk_context*)ctx, (duk_idx_t)index); }
    static int host_IsObject(void* ctx, int index) { return duk_is_object((duk_context*)ctx, (duk_idx_t)index); }
    static int host_IsFunction(void* ctx, int index) { return duk_is_function((duk_context*)ctx, (duk_idx_t)index); }
    static int host_IsNull(void* ctx, int index) { return duk_is_null((duk_context*)ctx, (duk_idx_t)index); }

    static int host_GetTop(void* ctx) { return (int)duk_get_top((duk_context*)ctx); }
    static void host_Pop(void* ctx) { duk_pop((duk_context*)ctx); }
    static void host_PopN(void* ctx, int n) { duk_pop_n((duk_context*)ctx, (duk_idx_t)n); }

    static void host_ThrowError(void* ctx, const char* message) {
        duk_error((duk_context*)ctx, DUK_ERR_ERROR, "%s", message);
    }

    static void* host_JsGetFunctionPtr(void* ctx, int index) {
        if (duk_is_function((duk_context*)ctx, (duk_idx_t)index)) {
            return duk_get_heapptr((duk_context*)ctx, (duk_idx_t)index);
        }
        return nullptr;
    }

    static void host_JsCallFunction(void* ctx, void* funcPtr, int nargs) {
        duk_push_heapptr((duk_context*)ctx, funcPtr);
        // Move arguments above the function
        if (nargs > 0) {
            duk_insert((duk_context*)ctx, -(nargs + 1));
        }
        if (duk_pcall((duk_context*)ctx, (duk_idx_t)nargs) != 0) {
            duk_pop((duk_context*)ctx); // error
        } else {
            duk_pop((duk_context*)ctx); // result
        }
    }

    static const NovadeskHostAPI s_HostAPI = {
        host_RegisterString,
        host_RegisterNumber,
        host_RegisterBool,
        host_RegisterObjectStart,
        host_RegisterObjectEnd,
        host_RegisterArrayString,
        host_RegisterArrayNumber,
        host_RegisterFunction,
        host_PushString,
        host_PushNumber,
        host_PushBool,
        host_PushNull,
        host_PushObject,
        host_GetNumber,
        host_GetString,
        host_GetBool,
        host_IsNumber,
        host_IsString,
        host_IsBool,
        host_IsObject,
        host_IsFunction,
        host_IsNull,
        host_GetTop,
        host_Pop,
        host_PopN,
        host_ThrowError,
        host_JsGetFunctionPtr,
        host_JsCallFunction
    };

    const NovadeskHostAPI* GetHostAPI() {
        return &s_HostAPI;
    }
}
