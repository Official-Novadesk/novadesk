/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "Widget.h"
#include "Settings.h"
#include "Logging.h"
#include "ColorUtil.h"
#include "Utils.h"
#include "CPUMonitor.h"
#include "MemoryMonitor.h"
#include "NetworkMonitor.h"
#include "MouseMonitor.h"
#include "DiskMonitor.h"
#include "PathUtils.h"
#include "FileUtils.h"
#include "TimerManager.h"
#include "PropertyParser.h"
#include <map>
#include <sstream>
#include <Windows.h>
#include "Novadesk.h"
#include <cwctype>
#include <algorithm>

extern std::vector<Widget*> widgets;

namespace JSApi {
    // Forward declarations
    duk_ret_t js_widget_add_image(duk_context* ctx);
    duk_ret_t js_widget_add_text(duk_context* ctx);
    duk_ret_t js_widget_add_context_menu_item(duk_context* ctx);
    duk_ret_t js_widget_clear_context_menu(duk_context* ctx);
    duk_ret_t js_widget_show_default_context_menu_items(duk_context* ctx);
    duk_ret_t js_widget_set_element_properties(duk_context* ctx);
    duk_ret_t js_widget_remove_elements(duk_context* ctx);
    duk_ret_t js_widget_get_element_properties(duk_context* ctx);
    duk_ret_t js_widget_set_properties(duk_context* ctx);
    duk_ret_t js_widget_get_properties(duk_context* ctx);
    duk_ret_t js_widget_close(duk_context* ctx);
    duk_ret_t js_novadesk_refresh(duk_context* ctx);
    duk_ret_t js_log(duk_context* ctx);
    duk_ret_t js_error(duk_context* ctx);
    duk_ret_t js_debug(duk_context* ctx);
    duk_ret_t js_include(duk_context* ctx);
    duk_ret_t js_on_ready(duk_context* ctx);
    duk_ret_t js_get_exe_path(duk_context* ctx);
    duk_ret_t js_get_env(duk_context* ctx);
    duk_ret_t js_get_all_env(duk_context* ctx);
    duk_ret_t js_register_hotkey(duk_context* ctx);
    duk_ret_t js_unregister_hotkey(duk_context* ctx);
    duk_ret_t js_system_get_display_metrics(duk_context* ctx);
    duk_ret_t js_set_timer(duk_context* ctx);
    duk_ret_t js_clear_timer(duk_context* ctx);
    duk_ret_t js_set_immediate(duk_context* ctx);
    
    // Monitor constructors/methods
    duk_ret_t js_cpu_constructor(duk_context* ctx);
    duk_ret_t js_cpu_usage(duk_context* ctx);
    duk_ret_t js_cpu_destroy(duk_context* ctx);
    duk_ret_t js_cpu_finalizer(duk_context* ctx);
    duk_ret_t js_memory_constructor(duk_context* ctx);
    duk_ret_t js_memory_stats(duk_context* ctx);
    duk_ret_t js_memory_destroy(duk_context* ctx);
    duk_ret_t js_memory_finalizer(duk_context* ctx);
    duk_ret_t js_network_constructor(duk_context* ctx);
    duk_ret_t js_network_stats(duk_context* ctx);
    duk_ret_t js_network_destroy(duk_context* ctx);
    duk_ret_t js_network_finalizer(duk_context* ctx);
    duk_ret_t js_mouse_constructor(duk_context* ctx);
    duk_ret_t js_mouse_position(duk_context* ctx);
    duk_ret_t js_mouse_destroy(duk_context* ctx);
    duk_ret_t js_mouse_finalizer(duk_context* ctx);
    duk_ret_t js_disk_constructor(duk_context* ctx);
    duk_ret_t js_disk_stats(duk_context* ctx);
    duk_ret_t js_disk_destroy(duk_context* ctx);
    duk_ret_t js_disk_finalizer(duk_context* ctx);

    // Internal helpers
    void ReloadScripts(duk_context* ctx);
    void CallHotkeyCallback(int callbackIdx);
    void CallStoredCallback(int id);
    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath);
    std::string ParseConfigDirectives(const std::string& script);

    // Ready callback storage
    static int s_ReadyCallbackIndex = -1;

    // Global context pointer for callbacks
    static duk_context* s_JsContext = nullptr;
    static std::wstring s_CurrentScriptPath = L""; // Remember script path for reloads
    static int s_NextTempId = 1; 
    static int s_NextTimerId = 1;
    static int s_NextImmId = 1;

    // Helper to call a JS function by its index in the "pending handlers" object
    void CallStoredCallback(int id) {
        if (!s_JsContext) return;

        // Get the novadesk.__timers object
        duk_get_global_string(s_JsContext, "novadesk");
        duk_get_prop_string(s_JsContext, -1, "__timers");
        
        // Push the ID
        duk_push_int(s_JsContext, id);
        if (duk_get_prop(s_JsContext, -2)) {
            if (duk_is_function(s_JsContext, -1)) {
                if (duk_pcall(s_JsContext, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Timer callback error: %S", duk_safe_to_string(s_JsContext, -1));
                }
            }
            duk_pop(s_JsContext); // Pop result or error
        } else {
            duk_pop(s_JsContext); // Pop undefined
        }

        duk_pop_2(s_JsContext); // Pop __timers and novadesk
    }

    // Helper to call any JS function by its index in novadesk.__hotkeys
    void CallHotkeyCallback(int callbackIdx) {
        if (!s_JsContext || callbackIdx < 0) return;

        duk_get_global_string(s_JsContext, "novadesk");
        if (duk_get_prop_string(s_JsContext, -1, "__hotkeys")) {
            duk_push_int(s_JsContext, callbackIdx);
            if (duk_get_prop(s_JsContext, -2)) {
                if (duk_is_function(s_JsContext, -1)) {
                    if (duk_pcall(s_JsContext, 0) != 0) {
                        Logging::Log(LogLevel::Error, L"Hotkey callback error: %S", duk_safe_to_string(s_JsContext, -1));
                    }
                }
                duk_pop(s_JsContext); // Pop result or error
            } else {
                duk_pop(s_JsContext); // Pop undefined
            }
        }
        duk_pop_2(s_JsContext); // Pop __hotkeys and novadesk
    }

    // JS API: novadesk.setInterval(cb, ms)
    duk_ret_t js_set_timer(duk_context* ctx) {
        bool repeating = duk_get_current_magic(ctx) != 0;
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int ms = duk_get_int_default(ctx, 1, 0);

        int tempId = s_NextTimerId++;
        
        // Store callback in novadesk.__timers[tempId]
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, tempId);
        duk_dup(ctx, 0); // Duplicate the callback function
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx); // Pop __timers and novadesk
 
        int id = TimerManager::Register(ms, tempId, repeating);
 
        duk_push_int(ctx, id);
        return 1;
    }

    // JS API: novadesk.clearTimer(id)
    duk_ret_t js_clear_timer(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return 0;
        int id = duk_get_int(ctx, 0);
        
        TimerManager::Clear(id);
 
        // Remove from JS side
        duk_get_global_string(ctx, "novadesk");
        if (duk_get_prop_string(ctx, -1, "__timers")) {
            duk_push_int(ctx, id);
            duk_del_prop(ctx, -2);
        }
        duk_pop_2(ctx);
 
        return 0;
    }

    // JS API: novadesk.setImmediate(cb) / process.nextTick(cb)
    duk_ret_t js_set_immediate(duk_context* ctx) {
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
 
        int tempId = s_NextImmId++;
 
        // Same storage logic
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, tempId);
        duk_dup(ctx, 0);
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx);
 
        int id = TimerManager::PushImmediate(tempId);
 
        duk_push_int(ctx, id);
        return 1;
    }

    // CPU Monitor JS methods
    duk_ret_t js_cpu_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        int proc = 0;
        if (duk_get_top(ctx) > 0 && duk_is_object(ctx, 0)) {
            if (duk_get_prop_string(ctx, 0, "processor")) proc = duk_get_int(ctx, -1);
            duk_pop(ctx);
        }
        CPUMonitor* monitor = new CPUMonitor(proc);
        duk_push_this(ctx);
        duk_push_pointer(ctx, monitor);
        duk_put_prop_string(ctx, -2, "\xFF" "monitorPtr");
        return 0;
    }

    duk_ret_t js_cpu_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "monitorPtr");
        CPUMonitor* monitor = (CPUMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) delete monitor;
        return 0;
    }

    duk_ret_t js_cpu_usage(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        CPUMonitor* monitor = (CPUMonitor*)duk_get_pointer(ctx, -1);
        if (!monitor) return DUK_RET_ERROR;
        duk_push_number(ctx, monitor->GetUsage());
        return 1;
    }

    duk_ret_t js_cpu_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        CPUMonitor* monitor = (CPUMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) {
            delete monitor;
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, -3, "\xFF" "monitorPtr");
        }
        return 0;
    }

    // Memory Monitor JS methods
    duk_ret_t js_memory_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        MemoryMonitor* monitor = new MemoryMonitor();
        duk_push_this(ctx);
        duk_push_pointer(ctx, monitor);
        duk_put_prop_string(ctx, -2, "\xFF" "monitorPtr");
        return 0;
    }

    duk_ret_t js_memory_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "monitorPtr");
        MemoryMonitor* monitor = (MemoryMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) delete monitor;
        return 0;
    }

    duk_ret_t js_memory_stats(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        MemoryMonitor* monitor = (MemoryMonitor*)duk_get_pointer(ctx, -1);
        if (!monitor) return DUK_RET_ERROR;
        auto stats = monitor->GetStats();
        duk_push_object(ctx);
        duk_push_number(ctx, (double)stats.totalPhys); duk_put_prop_string(ctx, -2, "total");
        duk_push_number(ctx, (double)stats.availPhys); duk_put_prop_string(ctx, -2, "available");
        duk_push_number(ctx, (double)stats.usedPhys); duk_put_prop_string(ctx, -2, "used");
        duk_push_int(ctx, stats.memoryLoad); duk_put_prop_string(ctx, -2, "percent");
        return 1;
    }

    duk_ret_t js_memory_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        MemoryMonitor* monitor = (MemoryMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) {
            delete monitor;
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, -3, "\xFF" "monitorPtr");
        }
        return 0;
    }

    // Network Monitor JS methods
    duk_ret_t js_network_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        NetworkMonitor* monitor = new NetworkMonitor();
        duk_push_this(ctx);
        duk_push_pointer(ctx, monitor);
        duk_put_prop_string(ctx, -2, "\xFF" "monitorPtr");
        return 0;
    }

    duk_ret_t js_network_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "monitorPtr");
        NetworkMonitor* monitor = (NetworkMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) delete monitor;
        return 0;
    }

    duk_ret_t js_network_stats(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        NetworkMonitor* monitor = (NetworkMonitor*)duk_get_pointer(ctx, -1);
        if (!monitor) return DUK_RET_ERROR;
        auto stats = monitor->GetStats();
        duk_push_object(ctx);
        duk_push_number(ctx, stats.inBytesPerSec); duk_put_prop_string(ctx, -2, "netIn");
        duk_push_number(ctx, stats.outBytesPerSec); duk_put_prop_string(ctx, -2, "netOut");
        duk_push_number(ctx, (double)stats.totalIn); duk_put_prop_string(ctx, -2, "totalIn");
        duk_push_number(ctx, (double)stats.totalOut); duk_put_prop_string(ctx, -2, "totalOut");
        return 1;
    }

    duk_ret_t js_network_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        NetworkMonitor* monitor = (NetworkMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) {
            delete monitor;
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, -3, "\xFF" "monitorPtr");
        }
        return 0;
    }

    // Mouse Monitor JS methods
    duk_ret_t js_mouse_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        MouseMonitor* monitor = new MouseMonitor();
        duk_push_this(ctx);
        duk_push_pointer(ctx, monitor);
        duk_put_prop_string(ctx, -2, "\xFF" "monitorPtr");
        return 0;
    }

    duk_ret_t js_mouse_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "monitorPtr");
        MouseMonitor* monitor = (MouseMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) delete monitor;
        return 0;
    }

    duk_ret_t js_mouse_position(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        MouseMonitor* monitor = (MouseMonitor*)duk_get_pointer(ctx, -1);
        if (!monitor) return DUK_RET_ERROR;
        auto pos = monitor->GetPosition();
        duk_push_object(ctx);
        duk_push_int(ctx, pos.x); duk_put_prop_string(ctx, -2, "x");
        duk_push_int(ctx, pos.y); duk_put_prop_string(ctx, -2, "y");
        return 1;
    }

    duk_ret_t js_mouse_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        MouseMonitor* monitor = (MouseMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) {
            delete monitor;
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, -3, "\xFF" "monitorPtr");
        }
        return 0;
    }

    // Disk Monitor JS methods
    duk_ret_t js_disk_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        
        std::wstring driveLetter = L"";  // Empty = all drives
        if (duk_get_top(ctx) > 0 && duk_is_object(ctx, 0)) {
            if (duk_get_prop_string(ctx, 0, "drive")) {
                if (!duk_is_null_or_undefined(ctx, -1)) {
                    driveLetter = Utils::ToWString(duk_get_string(ctx, -1));
                }
            }
            duk_pop(ctx);
        }
        
        DiskMonitor* monitor = new DiskMonitor(driveLetter);
        duk_push_this(ctx);
        duk_push_pointer(ctx, monitor);
        duk_put_prop_string(ctx, -2, "\xFF" "monitorPtr");
        return 0;
    }

    duk_ret_t js_disk_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "monitorPtr");
        DiskMonitor* monitor = (DiskMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) delete monitor;
        return 0;
    }

    duk_ret_t js_disk_stats(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        DiskMonitor* monitor = (DiskMonitor*)duk_get_pointer(ctx, -1);
        if (!monitor) return DUK_RET_ERROR;
        
        // Check if we should return all drives or single drive
        auto allDrives = monitor->GetAllDrives();
        
        if (allDrives.empty()) {
            // Single drive mode
            auto stats = monitor->GetStats();
            duk_push_object(ctx);
            duk_push_string(ctx, Utils::ToString(stats.driveLetter).c_str()); duk_put_prop_string(ctx, -2, "drive");
            duk_push_number(ctx, (double)stats.totalSpace); duk_put_prop_string(ctx, -2, "total");
            duk_push_number(ctx, (double)stats.freeSpace); duk_put_prop_string(ctx, -2, "free");
            duk_push_number(ctx, (double)stats.usedSpace); duk_put_prop_string(ctx, -2, "used");
            duk_push_int(ctx, stats.percentUsed); duk_put_prop_string(ctx, -2, "percent");
            return 1;
        }
        else {
            // All drives mode - return array
            duk_push_array(ctx);
            for (size_t i = 0; i < allDrives.size(); i++) {
                auto& stats = allDrives[i];
                duk_push_object(ctx);
                duk_push_string(ctx, Utils::ToString(stats.driveLetter).c_str()); duk_put_prop_string(ctx, -2, "drive");
                duk_push_number(ctx, (double)stats.totalSpace); duk_put_prop_string(ctx, -2, "total");
                duk_push_number(ctx, (double)stats.freeSpace); duk_put_prop_string(ctx, -2, "free");
                duk_push_number(ctx, (double)stats.usedSpace); duk_put_prop_string(ctx, -2, "used");
                duk_push_int(ctx, stats.percentUsed); duk_put_prop_string(ctx, -2, "percent");
                duk_put_prop_index(ctx, -2, (duk_uarridx_t)i);
            }
            return 1;
        }
    }

    duk_ret_t js_disk_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "monitorPtr");
        DiskMonitor* monitor = (DiskMonitor*)duk_get_pointer(ctx, -1);
        if (monitor) {
            delete monitor;
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, -3, "\xFF" "monitorPtr");
        }
        return 0;
    }

    duk_ret_t js_get_exe_path(duk_context* ctx) {
        std::wstring fullPath = PathUtils::GetExePath();
        std::wstring directory = PathUtils::GetExeDir();
        size_t lastBackslash = fullPath.find_last_of(L"\\");
        std::wstring filename = (lastBackslash != std::wstring::npos) ? fullPath.substr(lastBackslash + 1) : fullPath;
        
        duk_push_object(ctx);
        duk_push_string(ctx, Utils::ToString(fullPath).c_str());
        duk_put_prop_string(ctx, -2, "fullPath");
        duk_push_string(ctx, Utils::ToString(directory).c_str());
        duk_put_prop_string(ctx, -2, "directory");
        duk_push_string(ctx, Utils::ToString(filename).c_str());
        duk_put_prop_string(ctx, -2, "filename");
        return 1;
    }

    duk_ret_t js_get_env(duk_context* ctx) {
        if (duk_get_top(ctx) == 0 || duk_is_null_or_undefined(ctx, 0)) {
            // No arguments - return all environment variables
            LPWCH envStrings = GetEnvironmentStringsW();
            if (!envStrings) {
                duk_push_object(ctx);
                return 1;
            }
            
            duk_push_object(ctx);
            LPWCH current = envStrings;
            
            while (*current) {
                std::wstring entry = current;
                size_t pos = entry.find(L'=');
                
                // Skip entries that start with '=' (system variables)
                if (pos != std::wstring::npos && pos > 0) {
                    std::wstring key = entry.substr(0, pos);
                    std::wstring value = entry.substr(pos + 1);
                    
                    duk_push_string(ctx, Utils::ToString(value).c_str());
                    duk_put_prop_string(ctx, -2, Utils::ToString(key).c_str());
                }
                
                current += wcslen(current) + 1;
            }
            
            FreeEnvironmentStringsW(envStrings);
            return 1;
        }

        if (!duk_is_string(ctx, 0)) {
            return DUK_RET_TYPE_ERROR;
        }
        
        std::wstring varName = Utils::ToWString(duk_get_string(ctx, 0));
        wchar_t buffer[32767]; // Max environment variable size
        
        DWORD result = GetEnvironmentVariableW(varName.c_str(), buffer, 32767);
        if (result == 0) {
            duk_push_null(ctx);
        } else {
            duk_push_string(ctx, Utils::ToString(buffer).c_str());
        }
        return 1;
    }


    duk_ret_t js_register_hotkey(duk_context* ctx) {
        if (duk_get_top(ctx) < 2) return DUK_RET_TYPE_ERROR;

        std::wstring hotkeyStr = Utils::ToWString(duk_get_string(ctx, 0));
        int keyDownIdx = -1;
        int keyUpIdx = -1;
        
        int idForCallback = s_NextTempId++; 

        // Ensure JS-side storage exists
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__hotkeys")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__hotkeys");
            duk_get_prop_string(ctx, -1, "__hotkeys");
        }

        // Handle arguments: string/object
        if (duk_is_function(ctx, 1)) {
            keyDownIdx = idForCallback * 10;
            duk_push_int(ctx, keyDownIdx);
            duk_dup(ctx, 1);
            duk_put_prop(ctx, -3);
        } else if (duk_is_object(ctx, 1)) {
            if (duk_get_prop_string(ctx, 1, "onKeyDown") && duk_is_function(ctx, -1)) {
                keyDownIdx = idForCallback * 10;
                duk_push_int(ctx, keyDownIdx);
                duk_dup(ctx, -2);
                duk_put_prop(ctx, -4);
            }
            duk_pop(ctx);
            if (duk_get_prop_string(ctx, 1, "onKeyUp") && duk_is_function(ctx, -1)) {
                keyUpIdx = idForCallback * 10 + 1;
                duk_push_int(ctx, keyUpIdx);
                duk_dup(ctx, -2);
                duk_put_prop(ctx, -4);
            }
            duk_pop(ctx);
        }
        duk_pop_2(ctx); // Pop __hotkeys and novadesk

        int resultId = HotkeyManager::Register(hotkeyStr, keyDownIdx, keyUpIdx);
        if (resultId < 0) return DUK_RET_ERROR;

        duk_push_int(ctx, resultId);
        return 1;
    }

    duk_ret_t js_unregister_hotkey(duk_context* ctx) {
        if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int id = duk_get_int(ctx, 0);

        if (HotkeyManager::Unregister(id)) {
            // Success
        }

        return 0;
    }

    duk_ret_t js_system_get_display_metrics(duk_context* ctx) {
        const MultiMonitorInfo& info = System::GetMultiMonitorInfo();
        duk_push_object(ctx);

        // Indices
        const MonitorInfo* primary = (info.primaryIndex < info.monitors.size()) ? &info.monitors[info.primaryIndex] : (info.monitors.empty() ? nullptr : &info.monitors[0]);

        auto pushRect = [&](const RECT& r) {
            duk_push_object(ctx);
            duk_push_int(ctx, (int)r.left); duk_put_prop_string(ctx, -2, "x");
            duk_push_int(ctx, (int)r.top); duk_put_prop_string(ctx, -2, "y");
            duk_push_int(ctx, (int)(r.right - r.left)); duk_put_prop_string(ctx, -2, "width");
            duk_push_int(ctx, (int)(r.bottom - r.top)); duk_put_prop_string(ctx, -2, "height");
        };

        auto pushMonitorInfo = [&](const MonitorInfo& m) {
            duk_push_object(ctx);
            pushRect(m.work); duk_put_prop_string(ctx, -2, "workArea");
            pushRect(m.screen); duk_put_prop_string(ctx, -2, "screenArea");
        };

        // Primary monitor info
        if (primary) {
            pushMonitorInfo(*primary);
            duk_put_prop_string(ctx, -2, "primary");
        }

        // Virtual Screen info
        duk_push_object(ctx);
        duk_push_int(ctx, info.vsL); duk_put_prop_string(ctx, -2, "x");
        duk_push_int(ctx, info.vsT); duk_put_prop_string(ctx, -2, "y");
        duk_push_int(ctx, info.vsW); duk_put_prop_string(ctx, -2, "width");
        duk_push_int(ctx, info.vsH); duk_put_prop_string(ctx, -2, "height");
        duk_put_prop_string(ctx, -2, "virtualScreen");

        // All monitors array
        duk_push_array(ctx);
        for (size_t i = 0; i < info.monitors.size(); ++i) {
            pushMonitorInfo(info.monitors[i]);
            duk_push_int(ctx, (int)i + 1); duk_put_prop_string(ctx, -2, "id");
            duk_put_prop_index(ctx, -2, (duk_uarridx_t)i);
        }
        duk_put_prop_string(ctx, -2, "monitors");

        return 1;
    }

    // JS API: novadesk.include(filename)
    duk_ret_t js_include(duk_context* ctx) {
        std::wstring filename = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring fullPath = PathUtils::ResolvePath(filename, PathUtils::GetWidgetsDir());

        std::string content = FileUtils::ReadFileContent(fullPath);
        if (content.empty()) {
            return duk_error(ctx, DUK_ERR_ERROR, "Could not read file: %s", Utils::ToString(filename).c_str());
        }

        // Execute in global scope
        if (duk_peval_string(ctx, content.c_str()) != 0) {
            return duk_throw(ctx); // Re-throw error
        }

        duk_pop(ctx); // Pop result
        return 0;
    }

    /*
    ** JavaScript API: novadesk.log(...)
    ** Logs informational messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_log(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Info, L"%s", msg.c_str());
        return 0;
    }

    /*
    ** JavaScript API: novadesk.error(...)
    ** Logs error messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_error(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Error, L"%s", msg.c_str());
        return 0;
    }

    /*
    ** JavaScript API: novadesk.debug(...)
    ** Logs debug messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_debug(duk_context* ctx) {
        for (int i = 0; i < duk_get_top(ctx); i++) {
            Logging::Log(LogLevel::Debug, L"%S", duk_safe_to_string(ctx, i));
        }
        return 0;
    }

    /*
    ** JavaScript API: novadesk.onReady(callback)
    ** Registers a callback function to be called when the app is ready.
    ** The callback function should be a function that accepts no arguments.
    */
    duk_ret_t js_on_ready(duk_context* ctx) {
        if (!duk_is_function(ctx, 0)) {
            return DUK_RET_TYPE_ERROR;
        }

        // Store the callback
        duk_push_global_stash(ctx);
        duk_dup(ctx, 0); // Duplicate the callback function
        duk_put_prop_string(ctx, -2, "readyCallback");
        duk_pop(ctx);

        return 0;
    }

    /*
    ** JavaScript API: new widgetWindow(options)
    ** Creates a new widget window with the specified options.
    ** Options is an object that can contain various properties to customize the widget.
    ** Returns an object representing the created widget window.
    */
    duk_ret_t js_create_widget_window(duk_context* ctx) {
        if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        WidgetOptions options;
        options.x = CW_USEDEFAULT;
        options.y = CW_USEDEFAULT;
        options.width = 0;              // Default to auto
        options.height = 0;             // Default to auto
        options.m_WDefined = false;
        options.m_HDefined = false;
        options.backgroundColor = L"rgba(0,0,0,0)";
        options.zPos = ZPOSITION_ONDESKTOP;
        options.bgAlpha = 0;
        options.windowOpacity = 255;
        options.color = RGB(0,0,0);
        options.draggable = true;
        options.clickThrough = false;
        options.keepOnScreen = false;
        options.snapEdges = true;
 
        PropertyParser::ParseWidgetOptions(ctx, options);
 
        PropertyParser::ParseWidgetOptions(ctx, options);
 
        // Create new widget
        Widget* widget = new Widget(options);
        if (widget->Create()) {
            widget->Show();
            widgets.push_back(widget);
            Logging::Log(LogLevel::Info, L"Widget created and shown. HWND: 0x%p", widget->GetWindow());
        }
        else {
            Logging::Log(LogLevel::Error, L"Failed to create widget.");
            delete widget;
            return 0;
        }
        
        // Create JavaScript object to represent the widget
        duk_push_object(ctx);
        
        // Store widget pointer
        duk_push_pointer(ctx, widget);
        duk_put_prop_string(ctx, -2, "\xFF" "widgetPtr");
        
        // Add methods to the widget object
        duk_push_c_function(ctx, js_widget_add_image, 1);
        duk_put_prop_string(ctx, -2, "addImage");
        
        duk_push_c_function(ctx, js_widget_add_text, 1);
        duk_put_prop_string(ctx, -2, "addText");

        duk_push_c_function(ctx, js_widget_add_context_menu_item, 2);
        duk_put_prop_string(ctx, -2, "addContextMenuItem");

        duk_push_c_function(ctx, js_widget_clear_context_menu, 0);
        duk_put_prop_string(ctx, -2, "clearContextMenu");
        
        duk_push_c_function(ctx, js_widget_show_default_context_menu_items, 1);
        duk_put_prop_string(ctx, -2, "showDefaultContextMenuItems");
        
        duk_push_c_function(ctx, js_widget_set_element_properties, 2);
        duk_put_prop_string(ctx, -2, "setElementProperties");
        
        duk_push_c_function(ctx, js_widget_remove_elements, 1);
        duk_put_prop_string(ctx, -2, "removeElements");
        
        duk_push_c_function(ctx, js_widget_get_element_properties, 1);
        duk_put_prop_string(ctx, -2, "getElementProperties");

        duk_push_c_function(ctx, js_widget_set_properties, 1);
        duk_put_prop_string(ctx, -2, "setProperties");

        duk_push_c_function(ctx, js_widget_get_properties, 0);
        duk_put_prop_string(ctx, -2, "getProperties");

        duk_push_c_function(ctx, js_widget_close, 0);
        duk_put_prop_string(ctx, -2, "close");

        duk_push_c_function(ctx, js_novadesk_refresh, 0); // Full reload like Rainmeter
        duk_put_prop_string(ctx, -2, "refresh");
        
        return 1; // Return the object
    }

    void ParseElementOptions(duk_context* ctx, Element* element) {
        PropertyParser::ParseElementOptions(ctx, element);
    }

    duk_ret_t js_widget_add_image(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        duk_dup(ctx, 0); // Push options object to top
        PropertyParser::ImageOptions options;
        PropertyParser::ParseImageOptions(ctx, options);
        duk_pop(ctx); // Pop options object

        widget->AddImage(options);

        // Return 'this' for chaining
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_add_text(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        duk_dup(ctx, 0); // Push options object to top
        PropertyParser::TextOptions options;
        PropertyParser::ParseTextOptions(ctx, options);
        duk_pop(ctx); // Pop options object

        widget->AddText(options);

        // Return 'this' for chaining
        duk_push_this(ctx);
        return 1;
    }


    duk_ret_t js_widget_add_context_menu_item(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;

        if (duk_get_top(ctx) < 2) return DUK_RET_TYPE_ERROR;
        
        std::wstring label = Utils::ToWString(duk_safe_to_string(ctx, 0));
        std::wstring action = Utils::ToWString(duk_safe_to_string(ctx, 1));

        widget->AddContextMenuItem(label, action);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_clear_context_menu(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;

        widget->ClearContextMenuItems();

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_show_default_context_menu_items(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_ERROR;

        if (duk_get_top(ctx) > 0)
        {
            bool show = duk_get_boolean(ctx, 0);
            widget->SetShowDefaultContextMenuItems(show);
        }

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_set_element_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        if (!duk_is_object(ctx, 1)) return DUK_RET_TYPE_ERROR;

        duk_dup(ctx, 1); // Push properties object to top
        widget->SetElementProperties(id, ctx);
        duk_pop(ctx);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_remove_elements(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        if (duk_is_null_or_undefined(ctx, 0)) {
            widget->RemoveElements(); // Clear all
        } else if (duk_is_array(ctx, 0)) {
            std::vector<std::wstring> ids;
            int len = (int)duk_get_length(ctx, 0);
            for (int i = 0; i < len; i++) {
                duk_get_prop_index(ctx, 0, i);
                ids.push_back(Utils::ToWString(duk_get_string(ctx, -1)));
                duk_pop(ctx);
            }
            widget->RemoveElements(ids);
        } else {
            std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
            widget->RemoveElements(id);
        }

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_get_element_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        Element* element = widget->FindElementById(id);
        if (!element) {
            duk_push_null(ctx);
            return 1;
        }

        PropertyParser::PushElementProperties(ctx, element);
        return 1;
    }

    duk_ret_t js_widget_set_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
 
        PropertyParser::ApplyWidgetProperties(ctx, widget);
 
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_get_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;
 
        PropertyParser::PushWidgetProperties(ctx, widget);
 
        return 1;
    }

    duk_ret_t js_widget_close(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        delete widget;
        
        duk_push_this(ctx);
        duk_del_prop_string(ctx, -1, "\xFF" "widgetPtr");
        duk_pop(ctx);

        return 0;
    }

    duk_ret_t js_novadesk_refresh(duk_context* ctx) {
        ReloadScripts(ctx);
        return 0;
    }

    
    /*
    ** Execute a JavaScript string explicitly.
    ** Used for handling event callbacks.
    */
    
    void ExecuteScript(const std::wstring& script) {
        if (!s_JsContext) {
            Logging::Log(LogLevel::Error, L"ExecuteScript: Context not set");
            return;
        }

        std::string scriptStr = Utils::ToString(script);
        if (duk_peval_string(s_JsContext, scriptStr.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"JS Error: %S", duk_safe_to_string(s_JsContext, -1));
        }
        duk_pop(s_JsContext); // pop result/error
    }



    std::string ParseConfigDirectives(const std::string& script) {
        std::string result;
        std::istringstream stream(script);
        std::string line;

        while (std::getline(stream, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r");
            if (start == std::string::npos) {
                result += line + "\n";
                continue;
            }

            std::string trimmed = line.substr(start);

            // Check if line starts with !
            if (trimmed[0] == '!') {
                std::string directive = trimmed.substr(1);
                // Remove trailing semicolon if present
                if (!directive.empty() && directive.back() == ';') {
                    directive.pop_back();
                }

                // Process directives
                if (directive == "enableDebugging") {
                    Logging::SetLogLevel(LogLevel::Debug);
                    Logging::Log(LogLevel::Info, L"Directive: Debug logging enabled");
                }
                else if (directive == "disableLogging") {
                    Logging::SetConsoleLogging(false);
                    Logging::SetFileLogging(L""); // Also disable file logging
                    Logging::Log(LogLevel::Info, L"Directive: All logging disabled");
                }
                else if (directive == "logToFile") {
                    std::wstring logPath = PathUtils::GetExeDir() + L"novadesk.log";
                    Logging::SetFileLogging(logPath, true);
                    Logging::Log(LogLevel::Info, L"Directive: File logging enabled to %s", logPath.c_str());
                }
                else if (directive == "hideTrayIcon") {
                    ::HideTrayIconDynamic();
                }
                else {
                    Logging::Log(LogLevel::Error, L"Unknown directive: !%S", directive.c_str());
                }

                // Skip this line (don't add to result)
                continue;
            }

            result += line + "\n";
        }

        return result;
    }

    /*
    ** Initialize the JavaScript API and register all global functions.
    ** Registers the 'novadesk' object with log, error, and debug methods.
    ** Registers the 'widgetWindow' constructor for creating widget windows.
    */
    void InitializeJavaScriptAPI(duk_context* ctx) {
        s_JsContext = ctx; // Store context for callbacks

        // Register novadesk object and methods
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_log, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "log");
        duk_push_c_function(ctx, js_error, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "error");
        duk_push_c_function(ctx, js_debug, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "debug");
        duk_push_c_function(ctx, js_include, 1);
        duk_put_prop_string(ctx, -2, "include");
        duk_push_c_function(ctx, js_on_ready, 1);
        duk_put_prop_string(ctx, -2, "onReady");

        // Keep a reference to the novadesk object
        duk_put_global_string(ctx, "novadesk");

        // Register Managers
        HotkeyManager::SetCallbackHandler([](int idx) {
            CallHotkeyCallback(idx);
        });
        TimerManager::SetCallbackHandler([](int idx) {
            CallStoredCallback(idx);
        });

        // Register system object
        duk_push_object(ctx);

        duk_push_c_function(ctx, js_get_env, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "getEnv");
        duk_push_c_function(ctx, js_register_hotkey, 2);
        duk_put_prop_string(ctx, -2, "registerHotkey");
        duk_push_c_function(ctx, js_unregister_hotkey, 1);
        duk_put_prop_string(ctx, -2, "unregisterHotkey");
        duk_push_c_function(ctx, js_get_exe_path, 0);
        duk_put_prop_string(ctx, -2, "getExePath");
        duk_push_c_function(ctx, js_system_get_display_metrics, 0);
        duk_put_prop_string(ctx, -2, "getDisplayMetrics");

        // CPU Class
        duk_push_c_function(ctx, js_cpu_constructor, 1);
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_cpu_usage, 0); duk_put_prop_string(ctx, -2, "usage");
        duk_push_c_function(ctx, js_cpu_destroy, 0); duk_put_prop_string(ctx, -2, "destroy");
        duk_put_prop_string(ctx, -2, "prototype");
        duk_push_c_function(ctx, js_cpu_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "cpu");

        // Memory Class
        duk_push_c_function(ctx, js_memory_constructor, 0);
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_memory_stats, 0); duk_put_prop_string(ctx, -2, "stats");
        duk_push_c_function(ctx, js_memory_destroy, 0); duk_put_prop_string(ctx, -2, "destroy");
        duk_put_prop_string(ctx, -2, "prototype");
        duk_push_c_function(ctx, js_memory_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "memory");

        // Network Class
        duk_push_c_function(ctx, js_network_constructor, 0);
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_network_stats, 0); duk_put_prop_string(ctx, -2, "stats");
        duk_push_c_function(ctx, js_network_destroy, 0); duk_put_prop_string(ctx, -2, "destroy");
        duk_put_prop_string(ctx, -2, "prototype");
        duk_push_c_function(ctx, js_network_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "network");

        // Mouse Class
        duk_push_c_function(ctx, js_mouse_constructor, 0);
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_mouse_position, 0); duk_put_prop_string(ctx, -2, "position");
        duk_push_c_function(ctx, js_mouse_destroy, 0); duk_put_prop_string(ctx, -2, "destroy");
        duk_put_prop_string(ctx, -2, "prototype");
        duk_push_c_function(ctx, js_mouse_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "mouse");

        // Disk Class
        duk_push_c_function(ctx, js_disk_constructor, 1);
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_disk_stats, 0); duk_put_prop_string(ctx, -2, "stats");
        duk_push_c_function(ctx, js_disk_destroy, 0); duk_put_prop_string(ctx, -2, "destroy");
        duk_put_prop_string(ctx, -2, "prototype");
        duk_push_c_function(ctx, js_disk_finalizer, 1);
        duk_set_finalizer(ctx, -2);
        duk_put_prop_string(ctx, -2, "disk");

        duk_put_global_string(ctx, "system");

        // Register timer functions globally (not on novadesk)
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 1); // 1 = repeating
        duk_put_global_string(ctx, "setInterval");
        
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 0); // 0 = one-shot
        duk_put_global_string(ctx, "setTimeout");

        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearInterval");
        
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_global_string(ctx, "clearTimeout");

        duk_push_c_function(ctx, js_set_immediate, 1);
        duk_put_global_string(ctx, "setImmediate");




        // Register widgetWindow constructor
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    /*
    ** Load and execute the main JavaScript file.
    ** If scriptPath is empty, uses default Widgets\index.js.
    ** If scriptPath is provided, uses that path (supports relative and absolute).
    ** Calls onAppReady() function if defined in the script.
    ** Returns true on success, false on failure.
    */

    bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath) {
        // Reset logging state to defaults before each load
        Logging::SetConsoleLogging(true);
        Logging::SetFileLogging(L"");
        Logging::SetLogLevel(LogLevel::Info);

        // Determine which script to load
        std::wstring finalScriptPath;
        if (!scriptPath.empty())
        {
            // Custom path provided
            finalScriptPath = scriptPath;
            // Check if it's a relative path
            if (PathUtils::IsPathRelative(finalScriptPath))
            {
                // Make it relative to exe directory
                finalScriptPath = PathUtils::ResolvePath(finalScriptPath);
            }
            s_CurrentScriptPath = finalScriptPath; // Remember for reloads
        }
        else
        {
            // Use default or previously loaded path
            if (s_CurrentScriptPath.empty())
            {
                finalScriptPath = PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
                s_CurrentScriptPath = finalScriptPath;
            }
            else
            {
                finalScriptPath = s_CurrentScriptPath;
            }
        }

        Logging::Log(LogLevel::Info, L"Loading script from: %s", finalScriptPath.c_str());

        std::string content = FileUtils::ReadFileContent(finalScriptPath);
        if (content.empty()) {
            Logging::Log(LogLevel::Error, L"Failed to open script at %s", finalScriptPath.c_str());
            return false;
        }
        Logging::Log(LogLevel::Info, L"Script loaded, size: %zu", content.size());

        // Parse and apply configuration directives
        std::string processedScript = ParseConfigDirectives(content);

        // Execute the processed script
        if (duk_peval_string(ctx, processedScript.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
            duk_pop(ctx);
            return false;
        }

        Logging::Log(LogLevel::Info, L"Script execution successful. Calling ready callbacks...");

        // Call ready callback if registered
        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, "readyCallback")) {
            if (duk_is_function(ctx, -1)) {
                if (duk_pcall(ctx, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Ready callback failed: %S", duk_safe_to_string(ctx, -1));
                }
            }
        }
        duk_pop_2(ctx); // Pop callback and stash

        return true;
    }

    /*
    ** Reload all JavaScript scripts.
    ** Destroys all existing widgets and reloads the script.
    ** Uses the same script path that was initially loaded.
    ** Useful for development and testing without restarting the application.
    */

    void ReloadScripts(duk_context* ctx) {
        Logging::Log(LogLevel::Info, L"Reloading scripts...");
        
        // Unregister all hotkeys to prevent leaks and double hooks
        HotkeyManager::UnregisterAll();
        // Clear all timers
        TimerManager::ClearAll();
 
        // Delete ALL existing widgets
        for (auto w : widgets) {
            delete w;
        }
        widgets.clear();

        // Clear internal callbacks
        duk_get_global_string(ctx, "novadesk");
        duk_del_prop_string(ctx, -1, "__hotkeys");
        duk_del_prop_string(ctx, -1, "__timers");
        duk_pop(ctx);
 
        s_NextTempId = 1;
        s_NextTimerId = 1;
        s_NextImmId = 1;

        // Reload and execute script
        LoadAndExecuteScript(ctx, L"");
    }

    void Reload() {
        if (s_JsContext) {
            ReloadScripts(s_JsContext);
        }
    }

    void OnTimer(UINT_PTR id) {
        TimerManager::HandleTimer(id);
    }
 
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        TimerManager::HandleMessage(message, wParam, lParam);
    }
 
    void SetMessageWindow(HWND hWnd) {
        TimerManager::Initialize(hWnd);
    }

}
