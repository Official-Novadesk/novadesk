/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "FSModule.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <string>

#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"

namespace novadesk::scripting::quickjs
{
    namespace
    {
        namespace fs = std::filesystem;

        std::wstring ResolveFsPath(JSContext *ctx, JSValueConst v)
        {
            const char *s = JS_ToCString(ctx, v);
            if (!s)
                return L"";
            std::wstring p = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            if (p.empty())
                return L"";
            if (!PathUtils::IsPathRelative(p))
                return PathUtils::NormalizePath(p);
            return PathUtils::ResolvePath(p, JSEngine::GetEntryScriptDir());
        }

        JSValue JsFsReadFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.readFile(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::ifstream in(fs::path(path), std::ios::binary);
            if (!in.is_open())
                return JS_NULL;
            std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            return JS_NewStringLen(ctx, data.data(), data.size());
        }

        JSValue JsFsWriteFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.writeFile(path, data[, append])");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            const char *data = JS_ToCString(ctx, argv[1]);
            if (!data)
                return JS_EXCEPTION;
            const bool append = (argc > 2) ? (JS_ToBool(ctx, argv[2]) != 0) : false;
            std::ofstream out(fs::path(path), std::ios::binary | (append ? std::ios::app : std::ios::trunc));
            bool ok = out.is_open();
            if (ok)
                out.write(data, static_cast<std::streamsize>(std::strlen(data)));
            JS_FreeCString(ctx, data);
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsFsExists(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.exists(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_NewBool(ctx, 0);
            std::error_code ec;
            return JS_NewBool(ctx, fs::exists(fs::path(path), ec) && !ec ? 1 : 0);
        }

        JSValue JsFsMkdir(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.mkdir(path[, recursive])");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            const bool recursive = (argc > 1) ? (JS_ToBool(ctx, argv[1]) != 0) : true;
            std::error_code ec;
            bool ok = recursive ? fs::create_directories(fs::path(path), ec) : fs::create_directory(fs::path(path), ec);
            if (!ok && fs::exists(fs::path(path), ec))
                ok = true;
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsReaddir(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.readdir(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            JSValue arr = JS_NewArray(ctx);
            std::error_code ec;
            uint32_t i = 0;
            for (const auto &e : fs::directory_iterator(fs::path(path), ec))
            {
                if (ec)
                    break;
                const std::string name = Utils::ToString(e.path().filename().wstring());
                JS_SetPropertyUint32(ctx, arr, i++, JS_NewString(ctx, name.c_str()));
            }
            return arr;
        }

        JSValue JsFsUnlink(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.unlink(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::error_code ec;
            bool ok = fs::remove(fs::path(path), ec);
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsRename(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.rename(from, to)");
            const std::wstring from = ResolveFsPath(ctx, argv[0]);
            const std::wstring to = ResolveFsPath(ctx, argv[1]);
            if (from.empty() || to.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::error_code ec;
            fs::rename(fs::path(from), fs::path(to), ec);
            return JS_NewBool(ctx, !ec ? 1 : 0);
        }

        JSValue JsFsCopyFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.copyFile(from, to[, overwrite])");
            const std::wstring from = ResolveFsPath(ctx, argv[0]);
            const std::wstring to = ResolveFsPath(ctx, argv[1]);
            if (from.empty() || to.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            const bool overwrite = (argc > 2) ? (JS_ToBool(ctx, argv[2]) != 0) : true;
            std::error_code ec;
            const bool ok = fs::copy_file(
                fs::path(from),
                fs::path(to),
                overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none,
                ec);
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsStat(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.stat(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_NULL;
            std::error_code ec;
            const fs::path p(path);
            const fs::file_status st = fs::symlink_status(p, ec);
            if (ec || !fs::exists(st))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "isFile", JS_NewBool(ctx, fs::is_regular_file(st) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "isDirectory", JS_NewBool(ctx, fs::is_directory(st) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "isSymlink", JS_NewBool(ctx, fs::is_symlink(st) ? 1 : 0));
            uintmax_t size = 0;
            if (fs::is_regular_file(st))
            {
                size = fs::file_size(p, ec);
                if (ec)
                    size = 0;
            }
            JS_SetPropertyStr(ctx, out, "size", JS_NewFloat64(ctx, static_cast<double>(size)));
            JS_SetPropertyStr(ctx, out, "mode", JS_NewInt32(ctx, static_cast<int32_t>(st.permissions())));
            return out;
        }
    } // namespace

    static int FsModuleInit(JSContext *ctx, JSModuleDef *m)
    {
        JS_SetModuleExport(ctx, m, "readFile", JS_NewCFunction(ctx, JsFsReadFile, "readFile", 1));
        JS_SetModuleExport(ctx, m, "writeFile", JS_NewCFunction(ctx, JsFsWriteFile, "writeFile", 3));
        JS_SetModuleExport(ctx, m, "exists", JS_NewCFunction(ctx, JsFsExists, "exists", 1));
        JS_SetModuleExport(ctx, m, "mkdir", JS_NewCFunction(ctx, JsFsMkdir, "mkdir", 2));
        JS_SetModuleExport(ctx, m, "readdir", JS_NewCFunction(ctx, JsFsReaddir, "readdir", 1));
        JS_SetModuleExport(ctx, m, "unlink", JS_NewCFunction(ctx, JsFsUnlink, "unlink", 1));
        JS_SetModuleExport(ctx, m, "rename", JS_NewCFunction(ctx, JsFsRename, "rename", 2));
        JS_SetModuleExport(ctx, m, "copyFile", JS_NewCFunction(ctx, JsFsCopyFile, "copyFile", 3));
        JS_SetModuleExport(ctx, m, "stat", JS_NewCFunction(ctx, JsFsStat, "stat", 1));
        return 0;
    }

    JSModuleDef *EnsureFsModule(JSContext *ctx, const char *moduleName)
    {
        JSModuleDef *m = JS_NewCModule(ctx, moduleName, FsModuleInit);
        if (!m)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "readFile") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "writeFile") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "exists") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "mkdir") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "readdir") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "unlink") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "rename") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "copyFile") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "stat") < 0)
            return nullptr;
        return m;
    }
} // namespace novadesk::scripting::quickjs
