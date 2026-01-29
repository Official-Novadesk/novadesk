/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSPath.h"
#include "../Utils.h"
#include "../PathUtils.h"
#include <Windows.h>
#include <shlwapi.h>
#include <vector>

#pragma comment(lib, "shlwapi.lib")

namespace JSApi {

    duk_ret_t js_path_join(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        if (n == 0) {
            duk_push_string(ctx, ".");
            return 1;
        }

        std::wstring result;
        for (duk_idx_t i = 0; i < n; i++) {
            std::wstring segment = Utils::ToWString(duk_safe_to_string(ctx, i));
            if (segment.empty()) continue;

            if (result.empty()) {
                result = segment;
            } else {
                wchar_t combined[MAX_PATH];
                // Ensure result doesn't have a trailing slash for PathCombine
                if (result.back() == L'\\' || result.back() == L'/') {
                    result.pop_back();
                }
                if (PathCombineW(combined, result.c_str(), segment.c_str())) {
                    result = combined;
                } else {
                    // Fallback simpler join if PathCombine fails (e.g. too long)
                    if (result.back() != L'\\' && segment.front() != L'\\' && segment.front() != L'/') {
                        result += L'\\';
                    }
                    result += segment;
                }
            }
        }

        duk_push_string(ctx, Utils::ToString(PathUtils::NormalizePath(result)).c_str());
        return 1;
    }

    duk_ret_t js_path_resolve(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring resolvedPath = PathUtils::GetExeDir(); // Default starting point

        for (duk_idx_t i = 0; i < n; i++) {
            std::wstring segment = Utils::ToWString(duk_safe_to_string(ctx, i));
            resolvedPath = PathUtils::ResolvePath(segment, resolvedPath);
        }

        duk_push_string(ctx, Utils::ToString(resolvedPath).c_str());
        return 1;
    }

    duk_ret_t js_path_dirname(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring dir = PathUtils::GetParentDir(path);
        if (dir.empty()) {
            duk_push_string(ctx, ".");
        } else {
            // Remove trailing slash if exists (unless it's root)
            if (dir.length() > 3 && dir.back() == L'\\') dir.pop_back();
            duk_push_string(ctx, Utils::ToString(dir).c_str());
        }
        return 1;
    }

    duk_ret_t js_path_basename(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring ext;
        if (duk_get_top(ctx) > 1) {
            ext = Utils::ToWString(duk_require_string(ctx, 1));
        }

        wchar_t pathBuf[MAX_PATH];
        wcscpy_s(pathBuf, path.c_str());
        wchar_t* fileName = PathFindFileNameW(pathBuf);
        std::wstring res = fileName;

        if (!ext.empty() && res.length() >= ext.length()) {
            if (res.substr(res.length() - ext.length()) == ext) {
                res = res.substr(0, res.length() - ext.length());
            }
        }

        duk_push_string(ctx, Utils::ToString(res).c_str());
        return 1;
    }

    duk_ret_t js_path_extname(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        wchar_t* ext = PathFindExtensionW(path.c_str());
        duk_push_string(ctx, Utils::ToString(ext).c_str());
        return 1;
    }

    duk_ret_t js_path_is_absolute(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        duk_push_boolean(ctx, !PathIsRelativeW(path.c_str()));
        return 1;
    }

    duk_ret_t js_path_normalize(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        duk_push_string(ctx, Utils::ToString(PathUtils::NormalizePath(path)).c_str());
        return 1;
    }

    duk_ret_t js_path_parse(duk_context* ctx) {
        std::wstring path = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring norm = PathUtils::NormalizePath(path);
        
        duk_push_object(ctx);
        
        // Root
        std::wstring root;
        if (norm.length() >= 2 && norm[1] == L':') {
            root = norm.substr(0, 3); // e.g. C:\\ 
        } else if (norm.length() >= 2 && norm[0] == L'\\' && norm[1] == L'\\') {
            size_t next = norm.find(L'\\', 2);
            if (next != std::wstring::npos) {
                size_t next2 = norm.find(L'\\', next + 1);
                if (next2 != std::wstring::npos) root = norm.substr(0, next2 + 1);
                else root = norm + L'\\';
            }
        }
        duk_push_string(ctx, Utils::ToString(root).c_str());
        duk_put_prop_string(ctx, -2, "root");

        // Dir
        std::wstring dir = PathUtils::GetParentDir(norm);
        if (dir.length() > 3 && dir.back() == L'\\') dir.pop_back();
        duk_push_string(ctx, Utils::ToString(dir).c_str());
        duk_put_prop_string(ctx, -2, "dir");

        // Base
        wchar_t pathBuf[MAX_PATH];
        wcscpy_s(pathBuf, norm.c_str());
        std::wstring base = PathFindFileNameW(pathBuf);
        duk_push_string(ctx, Utils::ToString(base).c_str());
        duk_put_prop_string(ctx, -2, "base");

        // Ext
        std::wstring ext = PathFindExtensionW(norm.c_str());
        duk_push_string(ctx, Utils::ToString(ext).c_str());
        duk_put_prop_string(ctx, -2, "ext");

        // Name
        std::wstring name = base;
        if (!ext.empty() && name.length() >= ext.length()) {
            name = name.substr(0, name.length() - ext.length());
        }
        duk_push_string(ctx, Utils::ToString(name).c_str());
        duk_put_prop_string(ctx, -2, "name");

        return 1;
    }

    duk_ret_t js_path_format(duk_context* ctx) {
        if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
        
        std::wstring dir, base, name, ext;
        if (duk_get_prop_string(ctx, 0, "dir")) dir = Utils::ToWString(duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "base")) base = Utils::ToWString(duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "name")) name = Utils::ToWString(duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "ext")) ext = Utils::ToWString(duk_safe_to_string(ctx, -1));
        duk_pop(ctx);

        std::wstring result;
        if (!dir.empty()) {
            result = dir;
            if (result.back() != L'\\' && result.back() != L'/') result += L'\\';
        }

        if (!base.empty()) {
            result += base;
        } else {
            result += name + ext;
        }

        duk_push_string(ctx, Utils::ToString(PathUtils::NormalizePath(result)).c_str());
        return 1;
    }

    duk_ret_t js_path_relative(duk_context* ctx) {
        std::wstring from = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring to = Utils::ToWString(duk_require_string(ctx, 1));
        
        wchar_t relative[MAX_PATH];
        if (PathRelativePathToW(relative, from.c_str(), FILE_ATTRIBUTE_DIRECTORY, to.c_str(), FILE_ATTRIBUTE_NORMAL)) {
            duk_push_string(ctx, Utils::ToString(relative).c_str());
        } else {
            duk_push_string(ctx, Utils::ToString(to).c_str()); // Fallback
        }
        return 1;
    }

    void BindPathMethods(duk_context* ctx) {
        duk_push_object(ctx);
        
        duk_push_c_function(ctx, js_path_join, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "join");
        duk_push_c_function(ctx, js_path_resolve, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "resolve");
        duk_push_c_function(ctx, js_path_dirname, 1);
        duk_put_prop_string(ctx, -2, "dirname");
        duk_push_c_function(ctx, js_path_basename, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "basename");
        duk_push_c_function(ctx, js_path_extname, 1);
        duk_put_prop_string(ctx, -2, "extname");
        duk_push_c_function(ctx, js_path_is_absolute, 1);
        duk_put_prop_string(ctx, -2, "isAbsolute");
        duk_push_c_function(ctx, js_path_normalize, 1);
        duk_put_prop_string(ctx, -2, "normalize");
        duk_push_c_function(ctx, js_path_parse, 1);
        duk_put_prop_string(ctx, -2, "parse");
        duk_push_c_function(ctx, js_path_format, 1);
        duk_put_prop_string(ctx, -2, "format");
        duk_push_c_function(ctx, js_path_relative, 2);
        duk_put_prop_string(ctx, -2, "relative");

        duk_put_global_string(ctx, "path");
    }
}
