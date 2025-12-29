/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSSystem.h"
#include "../PathUtils.h"
#include "../Utils.h"
#include "../Hotkey.h"
#include "../System.h"
#include "../CPUMonitor.h"
#include "../MemoryMonitor.h"
#include "../NetworkMonitor.h"
#include "../MouseMonitor.h"
#include "../DiskMonitor.h"

namespace JSApi {

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

        duk_get_global_string(ctx, "novadesk");
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
}
