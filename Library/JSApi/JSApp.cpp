/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApp.h"
#include "../Utils.h"
#include "../PathUtils.h"
#include <Windows.h>
#include <shlobj.h>
#include <KnownFolders.h>

namespace JSApi {

    static std::wstring GetShellFolderPath(REFKNOWNFOLDERID rfid) {
        PWSTR path = NULL;
        HRESULT hr = SHGetKnownFolderPath(rfid, 0, NULL, &path);
        if (SUCCEEDED(hr)) {
            std::wstring res(path);
            CoTaskMemFree(path);
            if (res.back() != L'\\') res += L'\\';
            return res;
        }
        return L"";
    }

    duk_ret_t js_app_get_path(duk_context* ctx) {
        std::string name = duk_require_string(ctx, 0);
        std::wstring path;

        if (name == "desktop") path = GetShellFolderPath(FOLDERID_Desktop);
        else if (name == "documents") path = GetShellFolderPath(FOLDERID_Documents);
        else if (name == "downloads") path = GetShellFolderPath(FOLDERID_Downloads);
        else if (name == "music") path = GetShellFolderPath(FOLDERID_Music);
        else if (name == "pictures") path = GetShellFolderPath(FOLDERID_Pictures);
        else if (name == "videos") path = GetShellFolderPath(FOLDERID_Videos);
        else if (name == "home") path = GetShellFolderPath(FOLDERID_Profile);
        else if (name == "appData") path = GetShellFolderPath(FOLDERID_RoamingAppData);
        else if (name == "userData") {
            path = GetShellFolderPath(FOLDERID_RoamingAppData) + L"Novadesk\\";
        }
        else if (name == "temp") {
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            path = tempPath;
        }
        else if (name == "exe") path = PathUtils::GetExePath();
        else if (name == "appPath") path = PathUtils::GetExeDir();
        else return duk_error(ctx, DUK_ERR_ERROR, "Unknown path name: %s", name.c_str());

        duk_push_string(ctx, Utils::ToString(path).c_str());
        return 1;
    }

    duk_ret_t js_app_get_version(duk_context* ctx) {
        duk_push_string(ctx, "1.0.0"); // Fixed for now
        return 1;
    }

    duk_ret_t js_app_get_name(duk_context* ctx) {
        duk_push_string(ctx, "Novadesk");
        return 1;
    }

    duk_ret_t js_process_cwd(duk_context* ctx) {
        wchar_t cwd[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, cwd);
        duk_push_string(ctx, Utils::ToString(cwd).c_str());
        return 1;
    }

    void BindAppMethods(duk_context* ctx) {
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_app_get_path, 1);
        duk_put_prop_string(ctx, -2, "getPath");
        duk_push_c_function(ctx, js_app_get_version, 0);
        duk_put_prop_string(ctx, -2, "getVersion");
        duk_push_c_function(ctx, js_app_get_name, 0);
        duk_put_prop_string(ctx, -2, "getName");
        duk_put_global_string(ctx, "app");
    }

    void BindProcessMethods(duk_context* ctx) {
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_process_cwd, 0);
        duk_put_prop_string(ctx, -2, "cwd");
        duk_push_string(ctx, "win32");
        duk_put_prop_string(ctx, -2, "platform");
        duk_put_global_string(ctx, "process");
    }
}
