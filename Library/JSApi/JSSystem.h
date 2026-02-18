/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSCommon.h"

namespace JSApi {
    // Environment/Path API
    duk_ret_t js_get_env(duk_context* ctx);

    // Hotkey API
    duk_ret_t js_register_hotkey(duk_context* ctx);
    duk_ret_t js_unregister_hotkey(duk_context* ctx);

    // System API
    duk_ret_t js_system_execute(duk_context* ctx);
    duk_ret_t js_system_get_display_metrics(duk_context* ctx);
    duk_ret_t js_system_load_addon(duk_context* ctx);
    duk_ret_t js_system_set_wallpaper(duk_context* ctx);
    duk_ret_t js_system_get_current_wallpaper_path(duk_context* ctx);
    duk_ret_t js_system_get_brightness(duk_context* ctx);
    duk_ret_t js_system_set_brightness(duk_context* ctx);
    duk_ret_t js_system_extract_file_icon(duk_context* ctx);
    duk_ret_t js_system_list_app_volumes(duk_context* ctx);
    duk_ret_t js_system_get_app_volume(duk_context* ctx);
    duk_ret_t js_system_set_app_volume(duk_context* ctx);
    duk_ret_t js_system_get_app_peak(duk_context* ctx);
    duk_ret_t js_system_get_app_mute(duk_context* ctx);
    duk_ret_t js_system_set_app_mute(duk_context* ctx);

    // Monitor Constructors/Methods
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

    duk_ret_t js_now_playing_constructor(duk_context* ctx);
    duk_ret_t js_now_playing_stats(duk_context* ctx);
    duk_ret_t js_now_playing_play(duk_context* ctx);
    duk_ret_t js_now_playing_pause(duk_context* ctx);
    duk_ret_t js_now_playing_play_pause(duk_context* ctx);
    duk_ret_t js_now_playing_stop(duk_context* ctx);
    duk_ret_t js_now_playing_next(duk_context* ctx);
    duk_ret_t js_now_playing_previous(duk_context* ctx);
    duk_ret_t js_now_playing_set_position(duk_context* ctx);
    duk_ret_t js_now_playing_set_shuffle(duk_context* ctx);
    duk_ret_t js_now_playing_toggle_shuffle(duk_context* ctx);
    duk_ret_t js_now_playing_set_repeat(duk_context* ctx);
    duk_ret_t js_now_playing_destroy(duk_context* ctx);
    duk_ret_t js_now_playing_finalizer(duk_context* ctx);

    // Binding functions
    void BindSystemBaseMethods(duk_context* ctx);
    void BindSystemMonitors(duk_context* ctx);
    void CleanupAddons();
}
