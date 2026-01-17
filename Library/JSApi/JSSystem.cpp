/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSSystem.h"
#include "../Logging.h"
#include "../PathUtils.h"
#include "../Utils.h"
#include "../Hotkey.h"
#include "novadesk_addon.h"
#include "JSApi.h"
#include "../System.h"
#include "../CPUMonitor.h"
#include "../MemoryMonitor.h"
#include "../NetworkMonitor.h"
#include "../MouseMonitor.h"
#include "../DiskMonitor.h"
#include "JSUtils.h"
#include <map>
#include <vector>

namespace JSApi {

    struct AddonInfo {
        HMODULE handle;
        NovadeskAddonUnloadFn unloadFn;
    };

    static std::map<std::wstring, AddonInfo> s_LoadedAddons;


    duk_ret_t js_get_env(duk_context* ctx) {
        if (duk_get_top(ctx) == 0 || duk_is_null_or_undefined(ctx, 0)) {
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

        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        
        std::wstring varName = Utils::ToWString(duk_get_string(ctx, 0));
        std::vector<wchar_t> buffer(32767);
        DWORD result = GetEnvironmentVariableW(varName.c_str(), buffer.data(), 32767);
        if (result == 0) duk_push_null(ctx);
        else duk_push_string(ctx, Utils::ToString(buffer.data()).c_str());
        return 1;
    }

    duk_ret_t js_register_hotkey(duk_context* ctx) {
        if (duk_get_top(ctx) < 2) return DUK_RET_TYPE_ERROR;
        std::wstring hotkeyStr = Utils::ToWString(duk_get_string(ctx, 0));
        int keyDownIdx = -1;
        int keyUpIdx = -1;
        int idForCallback = s_NextTempId++; 

        duk_push_global_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "__hotkeys")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__hotkeys");
            duk_get_prop_string(ctx, -1, "__hotkeys");
        }

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
        duk_pop_2(ctx);

        int resultId = HotkeyManager::Register(hotkeyStr, keyDownIdx, keyUpIdx);
        if (resultId < 0) return DUK_RET_ERROR;
        duk_push_int(ctx, resultId);
        return 1;
    }

    duk_ret_t js_unregister_hotkey(duk_context* ctx) {
        if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int id = duk_get_int(ctx, 0);
        HotkeyManager::Unregister(id);
        return 0;
    }

    duk_ret_t js_system_get_display_metrics(duk_context* ctx) {
        const MultiMonitorInfo& info = System::GetMultiMonitorInfo();
        duk_push_object(ctx);
        const MonitorInfo* primary = (info.primaryIndex < (int)info.monitors.size()) ? &info.monitors[info.primaryIndex] : (info.monitors.empty() ? nullptr : &info.monitors[0]);

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

        if (primary) {
            pushMonitorInfo(*primary);
            duk_put_prop_string(ctx, -2, "primary");
        }

        duk_push_object(ctx);
        duk_push_int(ctx, info.vsL); duk_put_prop_string(ctx, -2, "x");
        duk_push_int(ctx, info.vsT); duk_put_prop_string(ctx, -2, "y");
        duk_push_int(ctx, info.vsW); duk_put_prop_string(ctx, -2, "width");
        duk_push_int(ctx, info.vsH); duk_put_prop_string(ctx, -2, "height");
        duk_put_prop_string(ctx, -2, "virtualScreen");

        duk_push_array(ctx);
        for (size_t i = 0; i < info.monitors.size(); ++i) {
            pushMonitorInfo(info.monitors[i]);
            duk_push_int(ctx, (int)i + 1); duk_put_prop_string(ctx, -2, "id");
            duk_put_prop_index(ctx, -2, (duk_uarridx_t)i);
        }
        duk_put_prop_string(ctx, -2, "monitors");
        return 1;
    }
    
    duk_ret_t js_system_execute(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;

        std::wstring target = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring parameters = L"";
        std::wstring workingDir = L"";
        int show = SW_SHOWNORMAL;

        if (duk_get_top(ctx) > 1 && !duk_is_null_or_undefined(ctx, 1)) {
            parameters = Utils::ToWString(duk_get_string(ctx, 1));
        }

        if (duk_get_top(ctx) > 2 && !duk_is_null_or_undefined(ctx, 2)) {
            workingDir = Utils::ToWString(duk_get_string(ctx, 2));
        }

        if (duk_get_top(ctx) > 3 && !duk_is_null_or_undefined(ctx, 3)) {
            if (duk_is_number(ctx, 3)) {
                show = duk_get_int(ctx, 3);
            } else if (duk_is_string(ctx, 3)) {
                std::string s = duk_get_string(ctx, 3);
                if (s == "hide") show = SW_HIDE;
                else if (s == "normal") show = SW_SHOWNORMAL;
                else if (s == "minimized") show = SW_SHOWMINIMIZED;
                else if (s == "maximized") show = SW_SHOWMAXIMIZED;
            }
        }

        bool success = System::Execute(target, parameters, workingDir, show);
        
        duk_push_boolean(ctx, success);
        return 1;
    }

    duk_ret_t js_system_load_addon(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        std::wstring addonPath = Utils::ToWString(duk_get_string(ctx, 0));

        if (PathUtils::IsPathRelative(addonPath)) {
            addonPath = PathUtils::ResolvePath(addonPath, PathUtils::GetParentDir(s_CurrentScriptPath));
        }

        auto it = s_LoadedAddons.find(addonPath);
        if (it != s_LoadedAddons.end()) {
            // Addon already loaded. For now, we don't re-initialize.
            // If the addon pushed something to the stack, we might need a way to track that.
            // But usually, an addon registers things globally.
            return 0;
        }

        HMODULE hModule = LoadLibraryW(addonPath.c_str());
        if (!hModule) {
            Logging::Log(LogLevel::Error, L"Failed to load addon: %s (Error: %d)", addonPath.c_str(), GetLastError());
            duk_push_null(ctx);
            return 1;
        }

        NovadeskAddonInitFn initFn = (NovadeskAddonInitFn)GetProcAddress(hModule, "NovadeskAddonInit");
        if (!initFn) {
            Logging::Log(LogLevel::Error, L"Addon %s is missing NovadeskAddonInit export", addonPath.c_str());
            FreeLibrary(hModule);
            duk_push_null(ctx);
            return 1;
        }

        AddonInfo info;
        info.handle = hModule;
        info.unloadFn = (void(*)())GetProcAddress(hModule, "NovadeskAddonUnload");
        
        s_LoadedAddons[addonPath] = info;

        // Call initialization. The addon may push a return value.
        int topBefore = duk_get_top(ctx);
        initFn(ctx, JSApi::GetMessageWindow(), JSApi::GetHostAPI());
        int topAfter = duk_get_top(ctx);

        if (topAfter > topBefore) {
            // Addon pushed one or more values. Return the top one.
            return 1;
        }

        return 0;
    }

    duk_ret_t js_system_unload_addon(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        std::wstring addonPath = Utils::ToWString(duk_get_string(ctx, 0));

        if (PathUtils::IsPathRelative(addonPath)) {
            addonPath = PathUtils::ResolvePath(addonPath, PathUtils::GetParentDir(s_CurrentScriptPath));
        }

        auto it = s_LoadedAddons.find(addonPath);
        if (it != s_LoadedAddons.end()) {
            if (it->second.unloadFn) {
                try {
                    it->second.unloadFn();
                } catch (...) {
                    Logging::Log(LogLevel::Error, L"Crash in NovadeskAddonUnload for %s", addonPath.c_str());
                }
            }
            FreeLibrary(it->second.handle);
            s_LoadedAddons.erase(it);
            duk_push_boolean(ctx, true);
        } else {
            duk_push_boolean(ctx, false);
        }
        return 1;
    }

    void CleanupAddons() {
        for (auto const& [path, info] : s_LoadedAddons) {
            if (info.unloadFn) {
                try {
                    info.unloadFn();
                } catch (...) {
                    Logging::Log(LogLevel::Error, L"Crash in NovadeskAddonUnload for %s", path.c_str());
                }
            }
            FreeLibrary(info.handle);
        }
        s_LoadedAddons.clear();
    }

    // Monitor Implementations
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
        double usage = monitor->GetUsage();
        usage = floor(usage + 0.5);
        duk_push_number(ctx, usage);
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

    duk_ret_t js_disk_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;
        std::wstring driveLetter = L"";
        if (duk_get_top(ctx) > 0 && duk_is_object(ctx, 0)) {
            if (duk_get_prop_string(ctx, 0, "drive")) {
                if (!duk_is_null_or_undefined(ctx, -1)) driveLetter = Utils::ToWString(duk_get_string(ctx, -1));
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
        auto allDrives = monitor->GetAllDrives();
        if (allDrives.empty()) {
            auto stats = monitor->GetStats();
            duk_push_object(ctx);
            duk_push_string(ctx, Utils::ToString(stats.driveLetter).c_str()); duk_put_prop_string(ctx, -2, "drive");
            duk_push_number(ctx, (double)stats.totalSpace); duk_put_prop_string(ctx, -2, "total");
            duk_push_number(ctx, (double)stats.freeSpace); duk_put_prop_string(ctx, -2, "free");
            duk_push_number(ctx, (double)stats.usedSpace); duk_put_prop_string(ctx, -2, "used");
            duk_push_int(ctx, stats.percentUsed); duk_put_prop_string(ctx, -2, "percent");
            return 1;
        } else {
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

    void BindSystemBaseMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_get_env, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "getEnv");
        duk_push_c_function(ctx, js_register_hotkey, 2);
        duk_put_prop_string(ctx, -2, "registerHotkey");
        duk_push_c_function(ctx, js_unregister_hotkey, 1);
        duk_put_prop_string(ctx, -2, "unregisterHotkey");
        duk_push_c_function(ctx, js_system_execute, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "execute");
        duk_push_c_function(ctx, js_system_get_display_metrics, 0);
        duk_put_prop_string(ctx, -2, "getDisplayMetrics");
        duk_push_c_function(ctx, js_system_load_addon, 1);
        duk_put_prop_string(ctx, -2, "loadAddon");
        duk_push_c_function(ctx, js_system_unload_addon, 1);
        duk_put_prop_string(ctx, -2, "unloadAddon");
    }

    void BindSystemMonitors(duk_context* ctx) {
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
    }
}
