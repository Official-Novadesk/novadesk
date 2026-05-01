#include "SystemModule.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>

#include "../../shared/System.h"
#include "../../shared/Utils.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Logging.h"
#include "../engine/JSEngine.h"

namespace novadesk::scripting::quickjs
{
    namespace
    {
        shared::system::NetworkStats g_cachedNetworkStats{};
        std::chrono::steady_clock::time_point g_cachedNetworkAt = std::chrono::steady_clock::time_point::min();
        shared::system::DiskIoStats g_cachedDiskIoStats{};
        std::chrono::steady_clock::time_point g_cachedDiskIoAt = std::chrono::steady_clock::time_point::min();
        uint64_t g_nextWebFetchId = 1;

        struct WebFetchRequest
        {
            uint64_t id = 0;
            JSContext *ctx = nullptr;
            JSValue resolve = JS_UNDEFINED;
            JSValue reject = JS_UNDEFINED;
            bool ok = false;
            std::string data;
            std::string error;
        };

        std::mutex g_webFetchMutex;
        std::unordered_map<uint64_t, std::unique_ptr<WebFetchRequest>> g_webFetchRequests;

        void DispatchWebFetchResult(void *payload)
        {
            std::unique_ptr<uint64_t> requestId(static_cast<uint64_t *>(payload));
            if (!requestId)
            {
                return;
            }

            std::unique_ptr<WebFetchRequest> req;
            {
                std::lock_guard<std::mutex> lock(g_webFetchMutex);
                auto it = g_webFetchRequests.find(*requestId);
                if (it == g_webFetchRequests.end())
                {
                    return;
                }
                req = std::move(it->second);
                g_webFetchRequests.erase(it);
            }

            if (!req || !req->ctx)
            {
                return;
            }

            JSValue arg = req->ok
                              ? JS_NewStringLen(req->ctx, req->data.data(), req->data.size())
                              : JS_NewString(req->ctx, req->error.empty() ? "webFetch failed" : req->error.c_str());
            JSValue fn = req->ok ? req->resolve : req->reject;
            JSValue ret = JS_Call(req->ctx, fn, JS_UNDEFINED, 1, &arg);
            JS_FreeValue(req->ctx, arg);
            if (JS_IsException(ret))
            {
                JS_FreeValue(req->ctx, JS_GetException(req->ctx));
            }
            else
            {
                JS_FreeValue(req->ctx, ret);
            }

            JS_FreeValue(req->ctx, req->resolve);
            JS_FreeValue(req->ctx, req->reject);

            JSRuntime *runtime = JS_GetRuntime(req->ctx);
            JSContext *jobCtx = nullptr;
            while (runtime && JS_IsJobPending(runtime))
            {
                int err = JS_ExecutePendingJob(runtime, &jobCtx);
                if (err < 0)
                {
                    if (jobCtx)
                    {
                        JS_FreeValue(jobCtx, JS_GetException(jobCtx));
                    }
                    break;
                }
            }
        }

        bool ReadNetworkCached(shared::system::NetworkStats &out)
        {
            const auto now = std::chrono::steady_clock::now();
            constexpr auto kMinResample = std::chrono::milliseconds(400);

            if (g_cachedNetworkAt == std::chrono::steady_clock::time_point::min() ||
                (now - g_cachedNetworkAt) >= kMinResample)
            {
                if (!shared::system::GetNetworkStats(g_cachedNetworkStats))
                {
                    return false;
                }
                g_cachedNetworkAt = now;
            }

            out = g_cachedNetworkStats;
            return true;
        }

        bool ReadDiskIoCached(shared::system::DiskIoStats &out)
        {
            const auto now = std::chrono::steady_clock::now();
            constexpr auto kMinResample = std::chrono::milliseconds(400);

            if (g_cachedDiskIoAt == std::chrono::steady_clock::time_point::min() ||
                (now - g_cachedDiskIoAt) >= kMinResample)
            {
                if (!shared::system::GetDiskIoStats(g_cachedDiskIoStats))
                {
                    return false;
                }
                g_cachedDiskIoAt = now;
            }

            out = g_cachedDiskIoStats;
            return true;
        }

        JSValue JsClipboardSetText(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "clipboard.setText(text) requires text");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            std::wstring text = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            return JS_NewBool(ctx, shared::system::ClipboardSetText(text) ? 1 : 0);
        }

        JSValue JsClipboardGetText(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            std::wstring text;
            if (!shared::system::ClipboardGetText(text))
            {
                return JS_NewString(ctx, "");
            }
            return JS_NewString(ctx, Utils::ToString(text).c_str());
        }

        JSValue JsWallpaperSet(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "wallpaper.set(path[, style]) requires path");
            const char *p = JS_ToCString(ctx, argv[0]);
            if (!p)
                return JS_EXCEPTION;
            std::wstring path = Utils::ToWString(p);
            JS_FreeCString(ctx, p);
            std::wstring style = L"fill";
            if (argc > 1)
            {
                const char *st = JS_ToCString(ctx, argv[1]);
                if (st)
                {
                    style = Utils::ToWString(st);
                    JS_FreeCString(ctx, st);
                }
            }
            return JS_NewBool(ctx, shared::system::SetWallpaper(path, style) ? 1 : 0);
        }

        JSValue JsWallpaperGet(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            std::wstring p;
            if (!shared::system::GetCurrentWallpaperPath(p))
                return JS_NewString(ctx, "");
            return JS_NewString(ctx, Utils::ToString(p).c_str());
        }

        JSValue JsPowerGetStatus(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::PowerStatus status;
            if (!shared::system::GetPowerStatus(status))
                return JS_NULL;
            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "acline", JS_NewInt32(ctx, status.acline));
            JS_SetPropertyStr(ctx, out, "status", JS_NewInt32(ctx, status.status));
            JS_SetPropertyStr(ctx, out, "status2", JS_NewInt32(ctx, status.status2));
            JS_SetPropertyStr(ctx, out, "lifetime", JS_NewFloat64(ctx, status.lifetime));
            JS_SetPropertyStr(ctx, out, "percent", JS_NewInt32(ctx, status.percent));
            JS_SetPropertyStr(ctx, out, "mhz", JS_NewFloat64(ctx, status.mhz));
            JS_SetPropertyStr(ctx, out, "hz", JS_NewFloat64(ctx, status.hz));
            return out;
        }

        JSValue JsCpuUsage(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::CpuStats stats;
            if (!shared::system::GetCpuStats(stats))
            {
                return JS_NewFloat64(ctx, 0.0);
            }
            return JS_NewFloat64(ctx, stats.usage);
        }

        JSValue JsMemoryTotalBytes(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::MemoryStats stats;
            if (!shared::system::GetMemoryStats(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.total);
        }

        JSValue JsMemoryAvailableBytes(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::MemoryStats stats;
            if (!shared::system::GetMemoryStats(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.available);
        }

        JSValue JsMemoryUsedBytes(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::MemoryStats stats;
            if (!shared::system::GetMemoryStats(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.used);
        }

        JSValue JsMemoryUsagePercent(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::MemoryStats stats;
            if (!shared::system::GetMemoryStats(stats))
                return JS_NewInt32(ctx, 0);
            return JS_NewInt32(ctx, stats.percent);
        }

        JSValue JsNetworkRxSpeed(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::NetworkStats stats;
            if (!ReadNetworkCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.netIn);
        }

        JSValue JsNetworkTxSpeed(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::NetworkStats stats;
            if (!ReadNetworkCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.netOut);
        }

        JSValue JsNetworkBytesReceived(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::NetworkStats stats;
            if (!ReadNetworkCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.totalIn);
        }

        JSValue JsNetworkBytesSent(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::NetworkStats stats;
            if (!ReadNetworkCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.totalOut);
        }

        std::wstring ReadOptionalPathArg(JSContext *ctx, int argc, JSValueConst *argv)
        {
            if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
            {
                return L"";
            }
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return L"";
            std::wstring out = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            return out;
        }

        JSValue JsDiskTotalBytes(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            shared::system::DiskStats stats;
            if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.total);
        }

        JSValue JsDiskAvailableBytes(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            shared::system::DiskStats stats;
            if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.available);
        }

        JSValue JsDiskUsedBytes(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            shared::system::DiskStats stats;
            if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.used);
        }

        JSValue JsDiskUsagePercent(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            shared::system::DiskStats stats;
            if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats))
                return JS_NewInt32(ctx, 0);
            return JS_NewInt32(ctx, stats.percent);
        }

        JSValue JsDiskReadSpeed(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::DiskIoStats stats;
            if (!ReadDiskIoCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.readSpeed);
        }

        JSValue JsDiskWriteSpeed(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::DiskIoStats stats;
            if (!ReadDiskIoCached(stats))
                return JS_NewFloat64(ctx, 0.0);
            return JS_NewFloat64(ctx, stats.writeSpeed);
        }

        JSValue JsRecycleBinOpenBin(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, shared::system::OpenRecycleBin() ? 1 : 0);
        }

        JSValue JsRecycleBinEmptyBin(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, shared::system::EmptyRecycleBin(false) ? 1 : 0);
        }

        JSValue JsRecycleBinEmptyBinSilent(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, shared::system::EmptyRecycleBin(true) ? 1 : 0);
        }

        JSValue JsRecycleBinGetStats(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::RecycleBinStats stats;
            if (!shared::system::GetRecycleBinStats(stats))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "count", JS_NewFloat64(ctx, stats.count));
            JS_SetPropertyStr(ctx, out, "size", JS_NewFloat64(ctx, stats.size));
            return out;
        }


        JSValue JsFileIconExtractIcon(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fileIcon.extractIcon(filePath, outIcoPath[, size])");
            const char *p1 = JS_ToCString(ctx, argv[0]);
            const char *p2 = JS_ToCString(ctx, argv[1]);
            if (!p1 || !p2)
            {
                if (p1)
                    JS_FreeCString(ctx, p1);
                if (p2)
                    JS_FreeCString(ctx, p2);
                return JS_EXCEPTION;
            }
            std::wstring filePath = Utils::ToWString(p1);
            std::wstring outPath = Utils::ToWString(p2);
            JS_FreeCString(ctx, p1);
            JS_FreeCString(ctx, p2);
            int32_t size = 48;
            if (argc > 2)
                JS_ToInt32(ctx, &size, argv[2]);
            return JS_NewBool(ctx, Utils::ExtractFileIconToIco(filePath, outPath, static_cast<int>(size)) ? 1 : 0);
        }

        JSValue JsDisplayMetricsGetMetrics(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            const auto &mm = shared::system::GetDisplayMetrics();
            JSValue out = JS_NewObject(ctx);

            auto makeArea = [&](int left, int top, int right, int bottom) -> JSValue
            {
                JSValue area = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, area, "x", JS_NewInt32(ctx, left));
                JS_SetPropertyStr(ctx, area, "y", JS_NewInt32(ctx, top));
                JS_SetPropertyStr(ctx, area, "width", JS_NewInt32(ctx, right - left));
                JS_SetPropertyStr(ctx, area, "height", JS_NewInt32(ctx, bottom - top));
                return area;
            };

            JSValue virtualScreen = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, virtualScreen, "x", JS_NewInt32(ctx, mm.virtualLeft));
            JS_SetPropertyStr(ctx, virtualScreen, "y", JS_NewInt32(ctx, mm.virtualTop));
            JS_SetPropertyStr(ctx, virtualScreen, "width", JS_NewInt32(ctx, mm.virtualWidth));
            JS_SetPropertyStr(ctx, virtualScreen, "height", JS_NewInt32(ctx, mm.virtualHeight));
            JS_SetPropertyStr(ctx, out, "virtualScreen", virtualScreen);

            JSValue arr = JS_NewArray(ctx);
            uint32_t i = 0;
            for (const auto &m : mm.monitors)
            {
                JSValue mo = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, mo, "id", JS_NewInt32(ctx, m.id));
                JS_SetPropertyStr(ctx, mo, "workArea", makeArea(m.work.left, m.work.top, m.work.right, m.work.bottom));
                JS_SetPropertyStr(ctx, mo, "screenArea", makeArea(m.screen.left, m.screen.top, m.screen.right, m.screen.bottom));
                JS_SetPropertyUint32(ctx, arr, i++, mo);
            }
            JS_SetPropertyStr(ctx, out, "monitors", arr);

            JSValue primary = JS_NewObject(ctx);
            if (mm.primaryIndex >= 0 && mm.primaryIndex < static_cast<int>(mm.monitors.size()))
            {
                const auto &pm = mm.monitors[mm.primaryIndex];
                JS_SetPropertyStr(ctx, primary, "workArea", makeArea(pm.work.left, pm.work.top, pm.work.right, pm.work.bottom));
                JS_SetPropertyStr(ctx, primary, "screenArea", makeArea(pm.screen.left, pm.screen.top, pm.screen.right, pm.screen.bottom));
            }
            else
            {
                JS_SetPropertyStr(ctx, primary, "workArea", makeArea(0, 0, 0, 0));
                JS_SetPropertyStr(ctx, primary, "screenArea", makeArea(0, 0, 0, 0));
            }
            JS_SetPropertyStr(ctx, out, "primary", primary);

            return out;
        }

        JSValue JsAudioSetVolume(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "audio.setVolume(value) requires value");
            int32_t value = 0;
            JS_ToInt32(ctx, &value, argv[0]);
            return JS_NewBool(ctx, shared::system::AudioSetVolume(static_cast<int>(value)) ? 1 : 0);
        }

        JSValue JsAudioGetVolume(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewInt32(ctx, shared::system::AudioGetVolume());
        }

        JSValue JsAudioPlaySound(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "audio.playSound(path[, loop]) requires path");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            std::wstring path = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            int loop = 0;
            if (argc > 1)
            {
                loop = JS_ToBool(ctx, argv[1]);
            }
            return JS_NewBool(ctx, shared::system::AudioPlaySound(path, loop != 0) ? 1 : 0);
        }

        JSValue JsAudioStopSound(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            shared::system::AudioStopSound();
            return JS_NewBool(ctx, 1);
        }


        JSValue JsRegistryReadData(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "registry.readData(path, valueName)");
            const char *p = JS_ToCString(ctx, argv[0]);
            const char *v = JS_ToCString(ctx, argv[1]);
            if (!p || !v)
            {
                if (p)
                    JS_FreeCString(ctx, p);
                if (v)
                    JS_FreeCString(ctx, v);
                return JS_EXCEPTION;
            }

            std::wstring fullPath = Utils::ToWString(p);
            std::wstring valueName = Utils::ToWString(v);
            JS_FreeCString(ctx, p);
            JS_FreeCString(ctx, v);

            shared::system::RegistryValue out;
            if (!shared::system::RegistryReadData(fullPath, valueName, out))
            {
                return JS_NULL;
            }

            if (out.type == shared::system::RegistryValueType::String)
            {
                return JS_NewString(ctx, Utils::ToString(out.stringValue).c_str());
            }
            if (out.type == shared::system::RegistryValueType::Number)
            {
                return JS_NewFloat64(ctx, out.numberValue);
            }
            return JS_NULL;
        }

        JSValue JsRegistryWriteData(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 3)
                return JS_ThrowTypeError(ctx, "registry.writeData(path, valueName, value)");
            const char *p = JS_ToCString(ctx, argv[0]);
            const char *v = JS_ToCString(ctx, argv[1]);
            if (!p || !v)
            {
                if (p)
                    JS_FreeCString(ctx, p);
                if (v)
                    JS_FreeCString(ctx, v);
                return JS_EXCEPTION;
            }

            std::wstring fullPath = Utils::ToWString(p);
            std::wstring valueName = Utils::ToWString(v);
            JS_FreeCString(ctx, p);
            JS_FreeCString(ctx, v);

            bool ok = false;
            if (JS_IsString(argv[2]))
            {
                const char *s = JS_ToCString(ctx, argv[2]);
                if (!s)
                    return JS_EXCEPTION;
                ok = shared::system::RegistryWriteString(fullPath, valueName, Utils::ToWString(s));
                JS_FreeCString(ctx, s);
            }
            else
            {
                double n = 0;
                if (JS_ToFloat64(ctx, &n, argv[2]) == 0)
                {
                    ok = shared::system::RegistryWriteNumber(fullPath, valueName, n);
                }
            }

            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        std::wstring ResolveModulePath(JSContext *ctx, JSValueConst v)
        {
            const char *s = JS_ToCString(ctx, v);
            if (!s)
                return L"";
            std::wstring path = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            if (!PathUtils::IsPathRelative(path))
                return PathUtils::NormalizePath(path);
            std::wstring base = JSEngine::GetCurrentScriptDir();
            if (base.empty())
            {
                base = JSEngine::GetEntryScriptDir();
            }
            if (base.empty())
            {
                base = PathUtils::GetWidgetsDir();
            }
            return PathUtils::ResolvePath(path, base);
        }

        JSValue JsJsonParse(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "json.parse(text)");
            const char *text = JS_ToCString(ctx, argv[0]);
            if (!text)
                return JS_EXCEPTION;
            size_t len = std::strlen(text);
            JSValue out = JS_ParseJSON(ctx, text, len, "<json.parse>");
            JS_FreeCString(ctx, text);
            return out;
        }

        JSValue JsJsonStringify(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "json.stringify(value[, space])");
            JSValue space = JS_UNDEFINED;
            if (argc > 1)
                space = JS_DupValue(ctx, argv[1]);
            JSValue s = JS_JSONStringify(ctx, argv[0], JS_UNDEFINED, space);
            if (!JS_IsUndefined(space))
                JS_FreeValue(ctx, space);
            return s;
        }

        JSValue JsJsonRead(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "json.read(path)");
            const std::wstring path = ResolveModulePath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "json.read invalid path");

            std::string text;
            if (!shared::system::JsonReadTextFile(path, text))
            {
                return JS_NULL;
            }
            if (text.find_first_not_of(" \t\r\n") == std::string::npos)
            {
                return JS_NewObject(ctx);
            }
            return JS_ParseJSON(ctx, text.c_str(), text.size(), Utils::ToString(path).c_str());
        }

        JSValue JsJsonWrite(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "json.write(path, value[, merge])");
            const std::wstring path = ResolveModulePath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "json.write invalid path");

            int merge = 0;
            if (argc > 2)
                merge = JS_ToBool(ctx, argv[2]);

            JSValue indent = JS_NewInt32(ctx, 4);
            JSValue s = JS_JSONStringify(ctx, argv[1], JS_UNDEFINED, indent);
            JS_FreeValue(ctx, indent);
            if (JS_IsException(s))
                return s;
            const char *text = JS_ToCString(ctx, s);
            if (!text)
            {
                JS_FreeValue(ctx, s);
                return JS_EXCEPTION;
            }

            bool ok = false;
            if (merge != 0)
                ok = shared::system::JsonMergePatchFile(path, text);
            else
                ok = shared::system::JsonWriteTextFile(path, text);

            JS_FreeCString(ctx, text);
            JS_FreeValue(ctx, s);
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsGetEnv(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
            {
                JSValue all = JS_NewObject(ctx);
                const auto vars = shared::system::GetAllEnv();
                for (const auto &kv : vars)
                {
                    const std::string key = Utils::ToString(kv.first);
                    const std::string val = Utils::ToString(kv.second);
                    JS_SetPropertyStr(ctx, all, key.c_str(), JS_NewString(ctx, val.c_str()));
                }
                return all;
            }

            const char *n = JS_ToCString(ctx, argv[0]);
            if (!n)
                return JS_EXCEPTION;
            std::wstring name = Utils::ToWString(n);
            JS_FreeCString(ctx, n);

            std::wstring value = shared::system::GetEnv(name);
            if (value.empty() && argc > 1 && !JS_IsUndefined(argv[1]) && !JS_IsNull(argv[1]))
            {
                const char *d = JS_ToCString(ctx, argv[1]);
                if (d)
                {
                    value = Utils::ToWString(d);
                    JS_FreeCString(ctx, d);
                }
            }

            return JS_NewString(ctx, Utils::ToString(value).c_str());
        }

        JSValue JsExecute(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "execute(target[, parameters, workingDir, show])");

            const char *t = JS_ToCString(ctx, argv[0]);
            if (!t)
                return JS_EXCEPTION;
            std::wstring target = Utils::ToWString(t);
            JS_FreeCString(ctx, t);

            std::wstring parameters;
            std::wstring workingDir;
            int32_t show = SW_SHOWNORMAL;

            if (argc > 1 && !JS_IsUndefined(argv[1]) && !JS_IsNull(argv[1]))
            {
                const char *p = JS_ToCString(ctx, argv[1]);
                if (p)
                {
                    parameters = Utils::ToWString(p);
                    JS_FreeCString(ctx, p);
                }
            }
            if (argc > 2 && !JS_IsUndefined(argv[2]) && !JS_IsNull(argv[2]))
            {
                const char *w = JS_ToCString(ctx, argv[2]);
                if (w)
                {
                    workingDir = Utils::ToWString(w);
                    JS_FreeCString(ctx, w);
                }
            }
            if (argc > 3 && !JS_IsUndefined(argv[3]) && !JS_IsNull(argv[3]))
            {
                JS_ToInt32(ctx, &show, argv[3]);
            }

            bool ok = shared::system::Execute(target, parameters, workingDir, static_cast<int>(show));
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsWebFetch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "webFetch(urlOrPath)");

            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            std::wstring pathOrUrl = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            if (pathOrUrl.empty())
                return JS_ThrowTypeError(ctx, "webFetch invalid url/path");
            if (PathUtils::IsPathRelative(pathOrUrl) &&
                pathOrUrl.rfind(L"http://", 0) != 0 &&
                pathOrUrl.rfind(L"https://", 0) != 0 &&
                pathOrUrl.rfind(L"file://", 0) != 0)
            {
                pathOrUrl = PathUtils::ResolvePath(pathOrUrl, JSEngine::GetEntryScriptDir());
            }

            JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
            JSValue promise = JS_NewPromiseCapability(ctx, funcs);
            if (JS_IsException(promise))
            {
                if (!JS_IsUndefined(funcs[0]))
                    JS_FreeValue(ctx, funcs[0]);
                if (!JS_IsUndefined(funcs[1]))
                    JS_FreeValue(ctx, funcs[1]);
                return JS_EXCEPTION;
            }

            auto req = std::make_unique<WebFetchRequest>();
            req->id = g_nextWebFetchId++;
            req->ctx = ctx;
            req->resolve = JS_DupValue(ctx, funcs[0]);
            req->reject = JS_DupValue(ctx, funcs[1]);

            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);

            const uint64_t requestId = req->id;
            {
                std::lock_guard<std::mutex> lock(g_webFetchMutex);
                g_webFetchRequests[requestId] = std::move(req);
            }

            std::thread([requestId, pathOrUrl]()
                        {
                            bool ok = false;
                            std::string data;
                            std::string error;

                            ok = shared::system::WebFetch(pathOrUrl, data);
                            if (!ok)
                            {
                                error = "webFetch failed";
                            }

                            {
                                std::lock_guard<std::mutex> lock(g_webFetchMutex);
                                auto it = g_webFetchRequests.find(requestId);
                                if (it == g_webFetchRequests.end() || !it->second)
                                {
                                    return;
                                }
                                it->second->ok = ok;
                                if (ok)
                                {
                                    it->second->data = std::move(data);
                                }
                                else
                                {
                                    it->second->error = std::move(error);
                                }
                            }

                            HWND hwnd = JSEngine::GetMessageWindow();
                            if (!hwnd)
                            {
                                return;
                            }

                            auto *payload = new uint64_t(requestId);
                            if (!PostMessageW(
                                    hwnd,
                                    JSEngine::WM_NOVADESK_DISPATCH,
                                    reinterpret_cast<WPARAM>(&DispatchWebFetchResult),
                                    reinterpret_cast<LPARAM>(payload)))
                            {
                                delete payload;
                                std::lock_guard<std::mutex> lock(g_webFetchMutex);
                                g_webFetchRequests.erase(requestId);
                            } })
                .detach();

            return promise;
        }

        int SystemModuleInit(JSContext *ctx, JSModuleDef *m)
        {
            JSValue clipboard = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, clipboard, "setText", JS_NewCFunction(ctx, JsClipboardSetText, "setText", 1));
            JS_SetPropertyStr(ctx, clipboard, "getText", JS_NewCFunction(ctx, JsClipboardGetText, "getText", 0));
            JS_SetModuleExport(ctx, m, "clipboard", clipboard);

            JSValue wallpaper = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, wallpaper, "set", JS_NewCFunction(ctx, JsWallpaperSet, "set", 2));
            JS_SetPropertyStr(ctx, wallpaper, "getCurrentPath", JS_NewCFunction(ctx, JsWallpaperGet, "getCurrentPath", 0));
            JS_SetModuleExport(ctx, m, "wallpaper", wallpaper);

            JSValue power = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, power, "getStatus", JS_NewCFunction(ctx, JsPowerGetStatus, "getStatus", 0));
            JS_SetModuleExport(ctx, m, "power", power);

            JSValue cpu = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, cpu, "usage", JS_NewCFunction(ctx, JsCpuUsage, "usage", 0));
            JS_SetModuleExport(ctx, m, "cpu", cpu);

            JSValue memory = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, memory, "totalBytes", JS_NewCFunction(ctx, JsMemoryTotalBytes, "totalBytes", 0));
            JS_SetPropertyStr(ctx, memory, "availableBytes", JS_NewCFunction(ctx, JsMemoryAvailableBytes, "availableBytes", 0));
            JS_SetPropertyStr(ctx, memory, "usedBytes", JS_NewCFunction(ctx, JsMemoryUsedBytes, "usedBytes", 0));
            JS_SetPropertyStr(ctx, memory, "usagePercent", JS_NewCFunction(ctx, JsMemoryUsagePercent, "usagePercent", 0));
            JS_SetModuleExport(ctx, m, "memory", memory);

            JSValue network = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, network, "rxSpeed", JS_NewCFunction(ctx, JsNetworkRxSpeed, "rxSpeed", 0));
            JS_SetPropertyStr(ctx, network, "txSpeed", JS_NewCFunction(ctx, JsNetworkTxSpeed, "txSpeed", 0));
            JS_SetPropertyStr(ctx, network, "bytesReceived", JS_NewCFunction(ctx, JsNetworkBytesReceived, "bytesReceived", 0));
            JS_SetPropertyStr(ctx, network, "bytesSent", JS_NewCFunction(ctx, JsNetworkBytesSent, "bytesSent", 0));
            JS_SetModuleExport(ctx, m, "network", network);

            JSValue disk = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, disk, "totalBytes", JS_NewCFunction(ctx, JsDiskTotalBytes, "totalBytes", 1));
            JS_SetPropertyStr(ctx, disk, "availableBytes", JS_NewCFunction(ctx, JsDiskAvailableBytes, "availableBytes", 1));
            JS_SetPropertyStr(ctx, disk, "usedBytes", JS_NewCFunction(ctx, JsDiskUsedBytes, "usedBytes", 1));
            JS_SetPropertyStr(ctx, disk, "usagePercent", JS_NewCFunction(ctx, JsDiskUsagePercent, "usagePercent", 1));
            JS_SetPropertyStr(ctx, disk, "readSpeed", JS_NewCFunction(ctx, JsDiskReadSpeed, "readSpeed", 0));
            JS_SetPropertyStr(ctx, disk, "writeSpeed", JS_NewCFunction(ctx, JsDiskWriteSpeed, "writeSpeed", 0));
            JS_SetModuleExport(ctx, m, "disk", disk);

            JSValue recycleBin = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, recycleBin, "openBin", JS_NewCFunction(ctx, JsRecycleBinOpenBin, "openBin", 0));
            JS_SetPropertyStr(ctx, recycleBin, "emptyBin", JS_NewCFunction(ctx, JsRecycleBinEmptyBin, "emptyBin", 0));
            JS_SetPropertyStr(ctx, recycleBin, "emptyBinSilent", JS_NewCFunction(ctx, JsRecycleBinEmptyBinSilent, "emptyBinSilent", 0));
            JS_SetPropertyStr(ctx, recycleBin, "getStats", JS_NewCFunction(ctx, JsRecycleBinGetStats, "getStats", 0));
            JS_SetModuleExport(ctx, m, "recycleBin", recycleBin);

            JSValue audio = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, audio, "setVolume", JS_NewCFunction(ctx, JsAudioSetVolume, "setVolume", 1));
            JS_SetPropertyStr(ctx, audio, "getVolume", JS_NewCFunction(ctx, JsAudioGetVolume, "getVolume", 0));
            JS_SetPropertyStr(ctx, audio, "playSound", JS_NewCFunction(ctx, JsAudioPlaySound, "playSound", 2));
            JS_SetPropertyStr(ctx, audio, "stopSound", JS_NewCFunction(ctx, JsAudioStopSound, "stopSound", 0));
            JS_SetModuleExport(ctx, m, "audio", audio);

            JSValue fileIcon = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, fileIcon, "extractIcon", JS_NewCFunction(ctx, JsFileIconExtractIcon, "extractIcon", 3));
            JS_SetPropertyStr(ctx, fileIcon, "extractFileIcon", JS_NewCFunction(ctx, JsFileIconExtractIcon, "extractFileIcon", 3));
            JS_SetModuleExport(ctx, m, "fileIcon", fileIcon);

            JSValue displayMetrics = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, displayMetrics, "getMetrics", JS_NewCFunction(ctx, JsDisplayMetricsGetMetrics, "getMetrics", 0));
            JS_SetPropertyStr(ctx, displayMetrics, "get", JS_NewCFunction(ctx, JsDisplayMetricsGetMetrics, "get", 0));
            JS_SetModuleExport(ctx, m, "displayMetrics", displayMetrics);

            JSValue registry = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, registry, "readData", JS_NewCFunction(ctx, JsRegistryReadData, "readData", 2));
            JS_SetPropertyStr(ctx, registry, "writeData", JS_NewCFunction(ctx, JsRegistryWriteData, "writeData", 3));
            JS_SetModuleExport(ctx, m, "registry", registry);

            JSValue json = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, json, "parse", JS_NewCFunction(ctx, JsJsonParse, "parse", 1));
            JS_SetPropertyStr(ctx, json, "stringify", JS_NewCFunction(ctx, JsJsonStringify, "stringify", 2));
            JS_SetPropertyStr(ctx, json, "read", JS_NewCFunction(ctx, JsJsonRead, "read", 1));
            JS_SetPropertyStr(ctx, json, "write", JS_NewCFunction(ctx, JsJsonWrite, "write", 3));
            JS_SetModuleExport(ctx, m, "json", json);

            JS_SetModuleExport(ctx, m, "getEnv", JS_NewCFunction(ctx, JsGetEnv, "getEnv", 2));
            JS_SetModuleExport(ctx, m, "execute", JS_NewCFunction(ctx, JsExecute, "execute", 4));
            JS_SetModuleExport(ctx, m, "webFetch", JS_NewCFunction(ctx, JsWebFetch, "webFetch", 1));

            return 0;
        }
    } // namespace

    JSModuleDef *EnsureSystemModule(JSContext *ctx, const char *moduleName)
    {
        JSModuleDef *m = JS_NewCModule(ctx, moduleName, SystemModuleInit);
        if (!m)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "clipboard") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "wallpaper") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "power") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "cpu") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "memory") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "network") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "disk") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "recycleBin") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "audio") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "fileIcon") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "displayMetrics") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "registry") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "json") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "getEnv") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "execute") < 0)
            return nullptr;
        if (JS_AddModuleExport(ctx, m, "webFetch") < 0)
            return nullptr;
        return m;
    }
} // namespace novadesk::scripting::quickjs
