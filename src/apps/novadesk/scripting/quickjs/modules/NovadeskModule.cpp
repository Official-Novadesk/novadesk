#include "NovadeskModule.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <windows.h>

#include "../../../Version.h"
#include "../../domain/Novadesk.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Settings.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "ModuleSystem.h"
#include "WidgetUiBindings.h"

namespace novadesk::scripting::quickjs
{
    namespace
    {
        using novadesk_context = void *;

        struct NovadeskHostAPI
        {
            void (*RegisterString)(novadesk_context ctx, const char *name, const char *value);
            void (*RegisterNumber)(novadesk_context ctx, const char *name, double value);
            void (*RegisterBool)(novadesk_context ctx, const char *name, int value);
            void (*RegisterObjectStart)(novadesk_context ctx, const char *name);
            void (*RegisterObjectEnd)(novadesk_context ctx, const char *name);
            void (*RegisterArrayString)(novadesk_context ctx, const char *name, const char **values, size_t count);
            void (*RegisterArrayNumber)(novadesk_context ctx, const char *name, const double *values, size_t count);
            void (*RegisterFunction)(novadesk_context ctx, const char *name, int (*func)(novadesk_context ctx), int nargs);
            void (*PushString)(novadesk_context ctx, const char *value);
            void (*PushNumber)(novadesk_context ctx, double value);
            void (*PushBool)(novadesk_context ctx, int value);
            void (*PushNull)(novadesk_context ctx);
            void (*PushObject)(novadesk_context ctx);
            void (*PushArray)(novadesk_context ctx);
            double (*GetNumber)(novadesk_context ctx, int index);
            const char *(*GetString)(novadesk_context ctx, int index);
            int (*GetBool)(novadesk_context ctx, int index);
            int (*IsNumber)(novadesk_context ctx, int index);
            int (*IsString)(novadesk_context ctx, int index);
            int (*IsBool)(novadesk_context ctx, int index);
            int (*IsObject)(novadesk_context ctx, int index);
            int (*IsFunction)(novadesk_context ctx, int index);
            int (*IsNull)(novadesk_context ctx, int index);
            int (*GetProperty)(novadesk_context ctx, int objIndex, const char *name);
            int (*GetTop)(novadesk_context ctx);
            void (*Pop)(novadesk_context ctx);
            void (*PopN)(novadesk_context ctx, int n);
            void (*ThrowError)(novadesk_context ctx, const char *message);
            void *(*JsGetFunctionPtr)(novadesk_context ctx, int index);
            void (*JsCallFunction)(novadesk_context ctx, void *funcPtr, int nargs);
            void (*JsCallFunctionNoArgs)(novadesk_context ctx, void *funcPtr);
            void (*ArrayPushObject)(novadesk_context ctx);
        };

        using NovadeskAddonInitFn = void (*)(novadesk_context ctx, HWND hMsgWnd, const NovadeskHostAPI *host);
        using NovadeskAddonUnloadFn = void (*)();

        bool g_moduleDebug = false;
        int g_nextTrayCommandId = 1;

        struct AddonInfo
        {
            int id = 0;
            HMODULE handle = nullptr;
            NovadeskAddonUnloadFn unloadFn = nullptr;
            std::vector<int> registeredFunctionIds;
            std::vector<void *> functionHandles;
            JSContext *exportCtx = nullptr;
            JSValue exportObject = JS_UNDEFINED;
        };

        std::map<std::wstring, AddonInfo> g_loadedAddons;
        std::unordered_map<int, std::wstring> g_addonPathById;
        int g_nextAddonId = 1;
        int g_nextAddonRegisteredFnId = 1;
        constexpr const char *kAddonIdKey = "__novadesk_addon_id";

        struct AddonRegisteredFunction
        {
            int (*fn)(novadesk_context) = nullptr;
            AddonInfo *addon = nullptr;
        };

        std::unordered_map<int, AddonRegisteredFunction> g_registeredAddonFunctions;

        struct JsFunctionHandle
        {
            JSContext *ctx = nullptr;
            JSValue fn = JS_UNDEFINED;
        };

        struct AddonCallContext
        {
            JSContext *ctx = nullptr;
            AddonInfo *addon = nullptr;
            std::vector<JSValue> args;
            std::vector<JSValue> stack;
            std::vector<std::string> tempStrings;
            std::string throwMessage;
            bool hasThrow = false;
        };

        static JSValue AddonRegisteredFunctionBridge(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic)
        {
            auto it = g_registeredAddonFunctions.find(magic);
            if (it == g_registeredAddonFunctions.end() || !it->second.fn)
            {
                return JS_UNDEFINED;
            }

            AddonCallContext call{};
            call.ctx = ctx;
            call.addon = it->second.addon;
            call.args.reserve(argc);
            for (int i = 0; i < argc; ++i)
            {
                call.args.push_back(JS_DupValue(ctx, argv[i]));
            }

            it->second.fn(reinterpret_cast<novadesk_context>(&call));

            for (JSValue &v : call.args)
            {
                JS_FreeValue(ctx, v);
            }

            if (call.hasThrow)
            {
                for (JSValue &v : call.stack)
                {
                    JS_FreeValue(ctx, v);
                }
                return JS_ThrowInternalError(ctx, "%s", call.throwMessage.c_str());
            }

            if (!call.stack.empty())
            {
                JSValue ret = call.stack.back();
                call.stack.pop_back();
                for (JSValue &v : call.stack)
                {
                    JS_FreeValue(ctx, v);
                }
                return ret;
            }

            return JS_UNDEFINED;
        }

        static JSValue *ResolveByIndex(AddonCallContext *call, int index)
        {
            if (!call)
                return nullptr;
            if (index >= 0)
            {
                if (index < static_cast<int>(call->args.size()))
                {
                    return &call->args[static_cast<size_t>(index)];
                }
                return nullptr;
            }

            const int pos = static_cast<int>(call->stack.size()) + index;
            if (pos >= 0 && pos < static_cast<int>(call->stack.size()))
            {
                return &call->stack[static_cast<size_t>(pos)];
            }
            return nullptr;
        }

        static void host_RegisterString(novadesk_context c, const char *name, const char *value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name)
                return;
            JSValue v = value ? JS_NewString(call->ctx, value) : JS_NULL;
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, v);
        }

        static void host_RegisterNumber(novadesk_context c, const char *name, double value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name)
                return;
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, JS_NewFloat64(call->ctx, value));
        }

        static void host_RegisterBool(novadesk_context c, const char *name, int value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name)
                return;
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, JS_NewBool(call->ctx, value ? 1 : 0));
        }

        static void host_RegisterObjectStart(novadesk_context c, const char *)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewObject(call->ctx));
        }

        static void host_RegisterObjectEnd(novadesk_context c, const char *name)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.size() < 2 || !name)
                return;
            JSValue child = call->stack.back();
            call->stack.pop_back();
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, child);
        }

        static void host_RegisterArrayString(novadesk_context c, const char *name, const char **values, size_t count)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name)
                return;
            JSValue arr = JS_NewArray(call->ctx);
            for (uint32_t i = 0; i < static_cast<uint32_t>(count); ++i)
            {
                JS_SetPropertyUint32(call->ctx, arr, i, JS_NewString(call->ctx, values[i] ? values[i] : ""));
            }
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, arr);
        }

        static void host_RegisterArrayNumber(novadesk_context c, const char *name, const double *values, size_t count)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name)
                return;
            JSValue arr = JS_NewArray(call->ctx);
            for (uint32_t i = 0; i < static_cast<uint32_t>(count); ++i)
            {
                JS_SetPropertyUint32(call->ctx, arr, i, JS_NewFloat64(call->ctx, values[i]));
            }
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, arr);
        }

        static void host_RegisterFunction(novadesk_context c, const char *name, int (*func)(novadesk_context), int nargs)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty() || !name || !func)
                return;
            const int id = g_nextAddonRegisteredFnId++;
            g_registeredAddonFunctions[id] = AddonRegisteredFunction{func, call->addon};
            if (call->addon)
            {
                call->addon->registeredFunctionIds.push_back(id);
            }
            JSValue fn = JS_NewCFunctionMagic(call->ctx, AddonRegisteredFunctionBridge, name, nargs, JS_CFUNC_generic_magic, id);
            JS_SetPropertyStr(call->ctx, call->stack.back(), name, fn);
        }

        static void host_PushString(novadesk_context c, const char *value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewString(call->ctx, value ? value : ""));
        }

        static void host_PushNumber(novadesk_context c, double value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewFloat64(call->ctx, value));
        }

        static void host_PushBool(novadesk_context c, int value)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewBool(call->ctx, value ? 1 : 0));
        }

        static void host_PushNull(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NULL);
        }

        static void host_PushObject(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewObject(call->ctx));
        }

        static void host_PushArray(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->stack.push_back(JS_NewArray(call->ctx));
        }

        static double host_GetNumber(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            if (!v)
                return 0.0;
            double n = 0.0;
            JS_ToFloat64(call->ctx, &n, *v);
            return n;
        }

        static const char *host_GetString(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            if (!call || !v)
                return nullptr;
            const char *s = JS_ToCString(call->ctx, *v);
            if (!s)
                return nullptr;
            call->tempStrings.emplace_back(s);
            JS_FreeCString(call->ctx, s);
            return call->tempStrings.back().c_str();
        }

        static int host_GetBool(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            if (!v)
                return 0;
            return JS_ToBool(call->ctx, *v) == 1 ? 1 : 0;
        }

        static int host_IsNumber(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && JS_IsNumber(*v);
        }

        static int host_IsString(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && JS_IsString(*v);
        }

        static int host_IsBool(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && JS_IsBool(*v);
        }

        static int host_IsObject(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && JS_IsObject(*v);
        }

        static int host_IsFunction(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && JS_IsFunction(call->ctx, *v);
        }

        static int host_IsNull(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            return v && (JS_IsNull(*v) || JS_IsUndefined(*v));
        }

        static int host_GetProperty(novadesk_context c, int objIndex, const char *name)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || !name)
                return 0;
            JSValue *obj = ResolveByIndex(call, objIndex);
            if (!obj || !JS_IsObject(*obj))
                return 0;

            JSValue value = JS_GetPropertyStr(call->ctx, *obj, name);
            if (JS_IsException(value))
            {
                call->hasThrow = true;
                call->throwMessage = "GetProperty failed";
                return 0;
            }

            call->stack.push_back(value);
            return JS_IsUndefined(value) ? 0 : 1;
        }

        static int host_GetTop(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return 0;
            return static_cast<int>(call->args.size() + call->stack.size());
        }

        static void host_Pop(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty())
                return;
            JS_FreeValue(call->ctx, call->stack.back());
            call->stack.pop_back();
        }

        static void host_PopN(novadesk_context c, int n)
        {
            for (int i = 0; i < n; ++i)
                host_Pop(c);
        }

        static void host_ThrowError(novadesk_context c, const char *message)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call)
                return;
            call->hasThrow = true;
            call->throwMessage = message ? message : "Addon error";
        }

        static void *host_JsGetFunctionPtr(novadesk_context c, int index)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            JSValue *v = ResolveByIndex(call, index);
            if (!call || !v || !JS_IsFunction(call->ctx, *v))
                return nullptr;
            auto *handle = new JsFunctionHandle{};
            handle->ctx = call->ctx;
            handle->fn = JS_DupValue(call->ctx, *v);
            if (call->addon)
            {
                call->addon->functionHandles.push_back(handle);
            }
            return handle;
        }

        static void host_JsCallFunction(novadesk_context c, void *funcPtr, int nargs)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            auto *handle = reinterpret_cast<JsFunctionHandle *>(funcPtr);
            if (!call || !handle || !JS_IsFunction(handle->ctx, handle->fn))
                return;

            if (nargs < 0 || nargs > static_cast<int>(call->stack.size()))
            {
                return;
            }

            std::vector<JSValue> argv(static_cast<size_t>(nargs));
            const int base = static_cast<int>(call->stack.size()) - nargs;
            for (int i = 0; i < nargs; ++i)
            {
                argv[static_cast<size_t>(i)] = JS_DupValue(call->ctx, call->stack[static_cast<size_t>(base + i)]);
            }

            JSValue ret = JS_Call(call->ctx, handle->fn, JS_UNDEFINED, nargs, argv.data());
            for (JSValue &a : argv)
                JS_FreeValue(call->ctx, a);
            for (int i = 0; i < nargs; ++i)
            {
                JS_FreeValue(call->ctx, call->stack.back());
                call->stack.pop_back();
            }

            if (JS_IsException(ret))
            {
                call->hasThrow = true;
                call->throwMessage = "JsCallFunction failed";
                return;
            }

            call->stack.push_back(ret);
        }

        static void host_JsCallFunctionNoArgs(novadesk_context, void *funcPtr)
        {
            auto *handle = reinterpret_cast<JsFunctionHandle *>(funcPtr);
            if (!handle || !JS_IsFunction(handle->ctx, handle->fn))
                return;

            JSValue ret = JS_Call(handle->ctx, handle->fn, JS_UNDEFINED, 0, nullptr);
            if (!JS_IsException(ret))
            {
                JS_FreeValue(handle->ctx, ret);
            }
            else
            {
                JSValue exc = JS_GetException(handle->ctx);
                JS_FreeValue(handle->ctx, exc);
            }
        }

        static void host_ArrayPushObject(novadesk_context c)
        {
            auto *call = reinterpret_cast<AddonCallContext *>(c);
            if (!call || call->stack.empty())
                return;
            JSValue *arr = &call->stack.back();
            if (!JS_IsArray(*arr))
                return;

            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(call->ctx, *arr, "length");
            JS_ToUint32(call->ctx, &len, lenV);
            JS_FreeValue(call->ctx, lenV);

            JSValue obj = JS_NewObject(call->ctx);
            JS_SetPropertyUint32(call->ctx, *arr, len, JS_DupValue(call->ctx, obj));
            call->stack.push_back(obj);
        }

        const NovadeskHostAPI g_hostApi = {
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
            host_PushArray,
            host_GetNumber,
            host_GetString,
            host_GetBool,
            host_IsNumber,
            host_IsString,
            host_IsBool,
            host_IsObject,
            host_IsFunction,
            host_IsNull,
            host_GetProperty,
            host_GetTop,
            host_Pop,
            host_PopN,
            host_ThrowError,
            host_JsGetFunctionPtr,
            host_JsCallFunction,
            host_JsCallFunctionNoArgs,
            host_ArrayPushObject};

        bool UnloadAddonById(int addonId)
        {
            auto pit = g_addonPathById.find(addonId);
            if (pit == g_addonPathById.end())
            {
                return false;
            }
            auto it = g_loadedAddons.find(pit->second);
            if (it == g_loadedAddons.end())
            {
                return false;
            }

            if (it->second.unloadFn)
            {
                try
                {
                    it->second.unloadFn();
                }
                catch (...)
                {
                    Logging::Log(LogLevel::Error, L"Crash in NovadeskAddonUnload");
                }
            }

            for (int id : it->second.registeredFunctionIds)
            {
                g_registeredAddonFunctions.erase(id);
            }
            for (void *p : it->second.functionHandles)
            {
                auto *h = reinterpret_cast<JsFunctionHandle *>(p);
                if (h)
                {
                    if (!JS_IsUndefined(h->fn))
                        JS_FreeValue(h->ctx, h->fn);
                    delete h;
                }
            }
            if (!JS_IsUndefined(it->second.exportObject) && it->second.exportCtx)
            {
                JS_FreeValue(it->second.exportCtx, it->second.exportObject);
                it->second.exportObject = JS_UNDEFINED;
            }

            g_addonPathById.erase(it->second.id);
            FreeLibrary(it->second.handle);
            g_loadedAddons.erase(it);
            return true;
        }

        void UnloadAllAddonsInternal()
        {
            std::vector<int> addonIds;
            addonIds.reserve(g_addonPathById.size());
            for (const auto &kv : g_addonPathById)
            {
                addonIds.push_back(kv.first);
            }
            for (int id : addonIds)
            {
                UnloadAddonById(id);
            }
        }

        std::wstring GetVersionProperty(const std::wstring &propertyName)
        {
            std::wstring exePath = PathUtils::GetExePath();
            DWORD handle = 0;
            DWORD size = GetFileVersionInfoSizeW(exePath.c_str(), &handle);
            if (size == 0)
                return L"";

            std::vector<BYTE> buffer(size);
            if (!GetFileVersionInfoW(exePath.c_str(), handle, size, buffer.data()))
            {
                return L"";
            }

            struct LANGANDCODEPAGE
            {
                WORD wLanguage;
                WORD wCodePage;
            } *translate = nullptr;
            UINT cbTranslate = 0;
            if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<LPVOID *>(&translate), &cbTranslate) ||
                cbTranslate < sizeof(LANGANDCODEPAGE))
            {
                return L"";
            }

            wchar_t subBlock[128];
            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\%s",
                       translate[0].wLanguage, translate[0].wCodePage, propertyName.c_str());

            LPWSTR value = nullptr;
            UINT sizeOut = 0;
            if (VerQueryValueW(buffer.data(), subBlock, reinterpret_cast<LPVOID *>(&value), &sizeOut) && value)
            {
                return value;
            }
            return L"";
        }

        bool ParseTrayMenuItems(JSContext *ctx, int trayId, JSValueConst arr, std::vector<MenuItem> &out)
        {
            if (!JS_IsArray(arr))
                return false;
            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
            if (JS_ToUint32(ctx, &len, lenV) != 0)
            {
                JS_FreeValue(ctx, lenV);
                return false;
            }
            JS_FreeValue(ctx, lenV);

            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue itemV = JS_GetPropertyUint32(ctx, arr, i);
                if (!JS_IsObject(itemV))
                {
                    JS_FreeValue(ctx, itemV);
                    continue;
                }

                MenuItem item{};
                item.id = 0;

                JSValue typeV = JS_GetPropertyStr(ctx, itemV, "type");
                const char *typeS = JS_ToCString(ctx, typeV);
                if (typeS && std::string(typeS) == "separator")
                    item.isSeparator = true;
                if (typeS)
                    JS_FreeCString(ctx, typeS);
                JS_FreeValue(ctx, typeV);

                if (!item.isSeparator)
                {
                    JSValue textV = JS_GetPropertyStr(ctx, itemV, "text");
                    const char *textS = JS_ToCString(ctx, textV);
                    if (textS)
                    {
                        item.text = Utils::ToWString(textS);
                        JS_FreeCString(ctx, textS);
                    }
                    JS_FreeValue(ctx, textV);

                    JSValue checkedV = JS_GetPropertyStr(ctx, itemV, "checked");
                    int checked = JS_ToBool(ctx, checkedV);
                    if (checked >= 0)
                        item.checked = (checked != 0);
                    JS_FreeValue(ctx, checkedV);

                    JSValue actionV = JS_GetPropertyStr(ctx, itemV, "action");
                    if (JS_IsFunction(ctx, actionV))
                    {
                        item.id = 2000 + g_nextTrayCommandId++;
                        JSEngine::RegisterTrayCommandCallback(ctx, trayId, item.id, actionV);
                    }
                    JS_FreeValue(ctx, actionV);

                    JSValue childV = JS_GetPropertyStr(ctx, itemV, "items");
                    if (JS_IsArray(childV))
                    {
                        ParseTrayMenuItems(ctx, trayId, childV, item.children);
                    }
                    JS_FreeValue(ctx, childV);
                }

                out.push_back(std::move(item));
                JS_FreeValue(ctx, itemV);
            }

            return true;
        }

        JSValue JsAddonUnload(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv);

        JSValue JsAppReload(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            (void)ctx;
            JSEngine::Reload();
            return JS_UNDEFINED;
        }

        JSValue JsAppRefresh(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            (void)ctx;
            JSEngine::Reload();
            return JS_UNDEFINED;
        }

        JSValue JsAppExit(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            (void)ctx;
            PostQuitMessage(0);
            return JS_UNDEFINED;
        }

        JSValue JsAppRequestSingleInstanceLock(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, RequestSingleInstanceLock() ? 1 : 0);
        }

        JSValue JsAppReleaseSingleInstanceLock(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            ReleaseSingleInstanceLock();
            return JS_NewBool(ctx, 1);
        }

        JSValue JsAppSaveLogToFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "app.saveLogToFile(enable) requires boolean");
            }
            int b = JS_ToBool(ctx, argv[0]);
            if (b < 0)
            {
                return JS_ThrowTypeError(ctx, "app.saveLogToFile(enable) expects boolean");
            }

            const bool enable = (b != 0);
            Settings::SetGlobalBool("saveLogToFile", enable);
            if (enable)
            {
                std::wstring logPath = PathUtils::GetAppDataPath() + L"logs.log";
                Logging::SetFileLogging(logPath, false);
            }
            else
            {
                Logging::SetFileLogging(L"");
            }
            return JS_NewBool(ctx, 1);
        }

        JSValue JsAppDisableLogging(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "app.disableLogging(disable) requires boolean");
            }
            int b = JS_ToBool(ctx, argv[0]);
            if (b < 0)
            {
                return JS_ThrowTypeError(ctx, "app.disableLogging(disable) expects boolean");
            }

            const bool disable = (b != 0);
            Settings::SetGlobalBool("disableLogging", disable);
            Logging::SetConsoleLogging(!disable);
            if (disable)
            {
                Logging::SetFileLogging(L"");
            }
            else if (Settings::GetGlobalBool("saveLogToFile", false))
            {
                std::wstring logPath = PathUtils::GetAppDataPath() + L"logs.log";
                Logging::SetFileLogging(logPath, false);
            }
            return JS_NewBool(ctx, 1);
        }

        JSValue JsAppUseHardwareAcceleration(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "app.useHardwareAcceleration(enable) requires boolean");
            }
            int b = JS_ToBool(ctx, argv[0]);
            if (b < 0)
            {
                return JS_ThrowTypeError(ctx, "app.useHardwareAcceleration(enable) expects boolean");
            }

            Settings::SetGlobalBool("useHardwareAcceleration", (b != 0));
            return JS_NewBool(ctx, 1);
        }

        JSValue JsAppGetProductVersion(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, Utils::ToString(GetVersionProperty(L"ProductVersion")).c_str());
        }

        JSValue JsAppGetFileVersion(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, Utils::ToString(GetVersionProperty(L"FileVersion")).c_str());
        }

        JSValue JsAppGetNovadeskVersion(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, NOVADESK_VERSION);
        }

        JSValue JsAppGetAppDataPath(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, Utils::ToString(PathUtils::GetAppDataPath()).c_str());
        }

        JSValue JsAppGetSettingsFilePath(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, Utils::ToString(Settings::GetSettingsPath()).c_str());
        }

        JSValue JsAppGetLogPath(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewString(ctx, Utils::ToString(Settings::GetLogPath()).c_str());
        }

        JSValue JsAppIsPortable(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, PathUtils::IsPortableEnvironment() ? 1 : 0);
        }

        JSValue JsAppIsFirstRun(JSContext *ctx, JSValueConst, int, JSValueConst *)
        {
            return JS_NewBool(ctx, Settings::IsFirstRun() ? 1 : 0);
        }

        static bool IsTrayEventName(const std::string &name)
        {
            static const std::unordered_set<std::string> kNames = {
                "click",
                "right-click",
            };
            return kNames.find(name) != kNames.end();
        }

        static const char *kTrayIdKey = "__trayId";

        bool GetTrayId(JSContext *ctx, JSValueConst thisVal, int &outId)
        {
            if (!JS_IsObject(thisVal))
                return false;
            JSValue idV = JS_GetPropertyStr(ctx, thisVal, kTrayIdKey);
            if (JS_IsUndefined(idV) || JS_IsNull(idV))
            {
                JS_FreeValue(ctx, idV);
                return false;
            }
            int32_t id = 0;
            const bool ok = (JS_ToInt32(ctx, &id, idV) == 0);
            JS_FreeValue(ctx, idV);
            if (!ok || id <= 0)
                return false;
            outId = id;
            return true;
        }

        std::wstring ResolveTrayImagePath(const std::wstring &inputPath)
        {
            if (inputPath.empty())
                return inputPath;

            if (PathUtils::IsPathRelative(inputPath))
            {
                std::wstring baseDir = PathUtils::GetParentDir(JSEngine::GetCurrentScriptPath());
                if (baseDir.empty())
                    baseDir = JSEngine::GetEntryScriptDir();
                if (baseDir.empty())
                    baseDir = PathUtils::GetWidgetsDir();
                return PathUtils::ResolvePath(inputPath, baseDir);
            }

            return PathUtils::NormalizePath(inputPath);
        }

        JSValue JsTrayOn(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            if (argc < 2 || !JS_IsString(argv[0]) || !JS_IsFunction(ctx, argv[1]))
            {
                return JS_ThrowTypeError(ctx, "tray.on(event, handler) requires event string and function");
            }
            int trayId = 0;
            if (!GetTrayId(ctx, thisVal, trayId))
            {
                return JS_ThrowTypeError(ctx, "tray.on called on invalid tray instance");
            }
            const char *nameC = JS_ToCString(ctx, argv[0]);
            if (!nameC)
            {
                return JS_EXCEPTION;
            }
            std::string name = nameC;
            JS_FreeCString(ctx, nameC);
            if (!IsTrayEventName(name))
            {
                return JS_ThrowTypeError(ctx, "tray.on: unknown event");
            }
            JSEngine::RegisterTrayEventCallback(ctx, trayId, name, argv[1]);
            return JS_UNDEFINED;
        }

        JSValue JsTrayDestroy(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            (void)ctx;
            int trayId = 0;
            if (!GetTrayId(ctx, thisVal, trayId))
            {
                return JS_ThrowTypeError(ctx, "tray.destroy called on invalid tray instance");
            }
            TrayDestroy(trayId);
            JSEngine::ClearTrayEventCallbacks(trayId);
            JSEngine::ClearTrayCommandCallbacks(trayId);
            JSEngine::UnregisterTrayOwner(trayId);
            return JS_UNDEFINED;
        }

        JSValue JsTraySetImage(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            if (argc < 1 || !JS_IsString(argv[0]))
            {
                return JS_ThrowTypeError(ctx, "tray.setImage(path) requires image path");
            }
            int trayId = 0;
            if (!GetTrayId(ctx, thisVal, trayId))
            {
                return JS_ThrowTypeError(ctx, "tray.setImage called on invalid tray instance");
            }
            const char *pathC = JS_ToCString(ctx, argv[0]);
            if (!pathC)
            {
                return JS_EXCEPTION;
            }
            std::wstring path = Utils::ToWString(pathC);
            JS_FreeCString(ctx, pathC);
            TraySetImage(trayId, ResolveTrayImagePath(path));
            return JS_UNDEFINED;
        }

        JSValue JsTraySetToolTip(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            if (argc < 1 || !JS_IsString(argv[0]))
            {
                return JS_ThrowTypeError(ctx, "tray.setToolTip(text) requires text");
            }
            int trayId = 0;
            if (!GetTrayId(ctx, thisVal, trayId))
            {
                return JS_ThrowTypeError(ctx, "tray.setToolTip called on invalid tray instance");
            }
            const char *textC = JS_ToCString(ctx, argv[0]);
            if (!textC)
            {
                return JS_EXCEPTION;
            }
            std::wstring text = Utils::ToWString(textC);
            JS_FreeCString(ctx, textC);
            TraySetToolTip(trayId, text);
            return JS_UNDEFINED;
        }

        JSValue JsTraySetContextMenu(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            if (argc < 1 || !JS_IsArray(argv[0]))
            {
                return JS_ThrowTypeError(ctx, "tray.setContextMenu: expected items array");
            }
            int trayId = 0;
            if (!GetTrayId(ctx, thisVal, trayId))
            {
                return JS_ThrowTypeError(ctx, "tray.setContextMenu called on invalid tray instance");
            }
            JSEngine::ClearTrayCommandCallbacks(trayId);
            std::vector<MenuItem> menu;
            if (!ParseTrayMenuItems(ctx, trayId, argv[0], menu))
            {
                return JS_ThrowTypeError(ctx, "tray.setContextMenu: invalid items");
            }
            TraySetContextMenu(trayId, menu);
            return JS_UNDEFINED;
        }

        JSValue JsTrayCtor(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            std::wstring imagePath;
            if (argc > 0 && !JS_IsNull(argv[0]) && !JS_IsUndefined(argv[0]))
            {
                if (!JS_IsString(argv[0]))
                {
                    return JS_ThrowTypeError(ctx, "new tray(image) expects image path or null");
                }
                const char *pathC = JS_ToCString(ctx, argv[0]);
                if (!pathC)
                {
                    return JS_EXCEPTION;
                }
                imagePath = Utils::ToWString(pathC);
                JS_FreeCString(ctx, pathC);
                imagePath = ResolveTrayImagePath(imagePath);
            }

            const int trayId = TrayCreate(imagePath);
            JSValue tray = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, tray, kTrayIdKey, JS_NewInt32(ctx, trayId));
            JS_SetPropertyStr(ctx, tray, "setImage", JS_NewCFunction(ctx, JsTraySetImage, "setImage", 1));
            JS_SetPropertyStr(ctx, tray, "setToolTip", JS_NewCFunction(ctx, JsTraySetToolTip, "setToolTip", 1));
            JS_SetPropertyStr(ctx, tray, "setContextMenu", JS_NewCFunction(ctx, JsTraySetContextMenu, "setContextMenu", 1));
            JS_SetPropertyStr(ctx, tray, "on", JS_NewCFunction(ctx, JsTrayOn, "on", 2));
            JS_SetPropertyStr(ctx, tray, "destroy", JS_NewCFunction(ctx, JsTrayDestroy, "destroy", 0));
            JSEngine::RegisterTrayOwner(trayId, JSEngine::GetCurrentScriptPath());
            return tray;
        }

        JSValue JsAppEnableDebugging(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "app.enableDebugging(enable) requires boolean");
            }

            int b = JS_ToBool(ctx, argv[0]);
            if (b < 0)
            {
                return JS_ThrowTypeError(ctx, "app.enableDebugging(enable) expects boolean");
            }

            const bool enable = (b != 0);
            Settings::SetGlobalBool("enableDebugging", enable);
            Logging::SetLogLevel(enable ? LogLevel::Debug : LogLevel::Info);
            SetModuleDebug(enable);
            SetModuleSystemDebug(enable);
            return JS_NewBool(ctx, 1);
        }

        JSValue JsAddonLoad(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "addon.load(path) requires path");
            }

            const char *pathC = JS_ToCString(ctx, argv[0]);
            if (!pathC)
            {
                return JS_EXCEPTION;
            }

            std::wstring addonPath = Utils::ToWString(pathC);
            JS_FreeCString(ctx, pathC);

            if (PathUtils::IsPathRelative(addonPath))
            {
                addonPath = PathUtils::ResolvePath(addonPath, JSEngine::GetEntryScriptDir());
            }
            else
            {
                addonPath = PathUtils::NormalizePath(addonPath);
            }

            auto it = g_loadedAddons.find(addonPath);
            if (it != g_loadedAddons.end())
            {
                if (it->second.exportCtx == ctx && !JS_IsUndefined(it->second.exportObject))
                {
                    return JS_DupValue(ctx, it->second.exportObject);
                }
                return JS_NewInt32(ctx, it->second.id);
            }

            HMODULE module = LoadLibraryW(addonPath.c_str());
            if (!module)
            {
                Logging::Log(LogLevel::Error, L"Failed to load addon: %s (Error: %d)", addonPath.c_str(), GetLastError());
                return JS_NULL;
            }

            auto initFn = reinterpret_cast<NovadeskAddonInitFn>(GetProcAddress(module, "NovadeskAddonInit"));
            if (!initFn)
            {
                Logging::Log(LogLevel::Error, L"Addon %s is missing NovadeskAddonInit export", addonPath.c_str());
                FreeLibrary(module);
                return JS_NULL;
            }

            AddonInfo info{};
            info.id = g_nextAddonId++;
            info.handle = module;
            info.unloadFn = reinterpret_cast<NovadeskAddonUnloadFn>(GetProcAddress(module, "NovadeskAddonUnload"));
            info.exportCtx = ctx;
            auto [insIt, _ok] = g_loadedAddons.emplace(addonPath, std::move(info));
            AddonInfo &stored = insIt->second;
            g_addonPathById[stored.id] = addonPath;

            AddonCallContext call{};
            call.ctx = ctx;
            call.addon = &stored;
            JSValue rootExports = JS_NewObject(ctx);
            call.stack.push_back(JS_DupValue(ctx, rootExports));

            initFn(reinterpret_cast<novadesk_context>(&call), JSEngine::GetMessageWindow(), &g_hostApi);

            if (call.hasThrow)
            {
                for (JSValue &v : call.args)
                    JS_FreeValue(ctx, v);
                for (JSValue &v : call.stack)
                    JS_FreeValue(ctx, v);
                if (stored.unloadFn)
                {
                    try
                    {
                        stored.unloadFn();
                    }
                    catch (...)
                    {
                    }
                }
                FreeLibrary(stored.handle);
                g_addonPathById.erase(stored.id);
                g_loadedAddons.erase(insIt);
                return JS_ThrowInternalError(ctx, "%s", call.throwMessage.c_str());
            }

            JSValue addonHandle = JS_UNDEFINED;
            if (!call.stack.empty() && JS_IsObject(call.stack.back()))
            {
                addonHandle = JS_DupValue(ctx, call.stack.back());
            }
            else if (!call.stack.empty())
            {
                addonHandle = JS_DupValue(ctx, rootExports);
                JS_SetPropertyStr(ctx, addonHandle, "value", JS_DupValue(ctx, call.stack.back()));
            }
            else
            {
                addonHandle = JS_DupValue(ctx, rootExports);
            }

            JS_SetPropertyStr(ctx, addonHandle, kAddonIdKey, JS_NewInt32(ctx, stored.id));
            JSValue unloadFn = JS_NewCFunction(ctx, JsAddonUnload, "unload", 0);
            JS_SetPropertyStr(ctx, addonHandle, "unload", unloadFn);
            stored.exportObject = JS_DupValue(ctx, addonHandle);

            for (size_t i = 0; i < call.stack.size(); ++i)
            {
                JS_FreeValue(ctx, call.stack[i]);
            }
            JS_FreeValue(ctx, rootExports);
            return addonHandle;
        }

        JSValue JsAddonUnload(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            JSValueConst target = JS_UNDEFINED;
            if (argc >= 1)
            {
                target = argv[0];
            }
            else
            {
                // Support handle.unload() with no args.
                target = thisVal;
            }

            int32_t addonId = 0;
            if (JS_IsObject(target))
            {
                JSValue idV = JS_GetPropertyStr(ctx, target, kAddonIdKey);
                const bool ok = (JS_ToInt32(ctx, &addonId, idV) == 0);
                JS_FreeValue(ctx, idV);
                if (!ok)
                {
                    return JS_ThrowTypeError(ctx, "addon.unload(addonObject): invalid addon object");
                }
            }
            else
            {
                if (JS_ToInt32(ctx, &addonId, target) != 0)
                {
                    return JS_ThrowTypeError(ctx, "addon.unload(addonObject|addonId) expects object or number");
                }
            }

            return JS_NewBool(ctx, UnloadAddonById(static_cast<int>(addonId)) ? 1 : 0);
        }

        int InitAppExport(JSContext *ctx, JSModuleDef *m)
        {
            JSValue app = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, app, "reload", JS_NewCFunction(ctx, JsAppReload, "reload", 0));
            JS_SetPropertyStr(ctx, app, "refresh", JS_NewCFunction(ctx, JsAppRefresh, "refresh", 0));
            JS_SetPropertyStr(ctx, app, "exit", JS_NewCFunction(ctx, JsAppExit, "exit", 0));
            JS_SetPropertyStr(ctx, app, "requestSingleInstanceLock", JS_NewCFunction(ctx, JsAppRequestSingleInstanceLock, "requestSingleInstanceLock", 0));
            JS_SetPropertyStr(ctx, app, "releaseSingleInstanceLock", JS_NewCFunction(ctx, JsAppReleaseSingleInstanceLock, "releaseSingleInstanceLock", 0));
            JS_SetPropertyStr(ctx, app, "saveLogToFile", JS_NewCFunction(ctx, JsAppSaveLogToFile, "saveLogToFile", 1));
            JS_SetPropertyStr(ctx, app, "disableLogging", JS_NewCFunction(ctx, JsAppDisableLogging, "disableLogging", 1));
            JS_SetPropertyStr(ctx, app, "useHardwareAcceleration", JS_NewCFunction(ctx, JsAppUseHardwareAcceleration, "useHardwareAcceleration", 1));
            JS_SetPropertyStr(ctx, app, "getProductVersion", JS_NewCFunction(ctx, JsAppGetProductVersion, "getProductVersion", 0));
            JS_SetPropertyStr(ctx, app, "getFileVersion", JS_NewCFunction(ctx, JsAppGetFileVersion, "getFileVersion", 0));
            JS_SetPropertyStr(ctx, app, "getNovadeskVersion", JS_NewCFunction(ctx, JsAppGetNovadeskVersion, "getNovadeskVersion", 0));
            JS_SetPropertyStr(ctx, app, "getAppDataPath", JS_NewCFunction(ctx, JsAppGetAppDataPath, "getAppDataPath", 0));
            JS_SetPropertyStr(ctx, app, "getSettingsFilePath", JS_NewCFunction(ctx, JsAppGetSettingsFilePath, "getSettingsFilePath", 0));
            JS_SetPropertyStr(ctx, app, "getLogPath", JS_NewCFunction(ctx, JsAppGetLogPath, "getLogPath", 0));
            JS_SetPropertyStr(ctx, app, "isPortable", JS_NewCFunction(ctx, JsAppIsPortable, "isPortable", 0));
            JS_SetPropertyStr(ctx, app, "isFirstRun", JS_NewCFunction(ctx, JsAppIsFirstRun, "isFirstRun", 0));
            JS_SetPropertyStr(ctx, app, "enableDebugging", JS_NewCFunction(ctx, JsAppEnableDebugging, "enableDebugging", 1));
            JS_SetModuleExport(ctx, m, "app", app);

            JSValue addon = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, addon, "load", JS_NewCFunction(ctx, JsAddonLoad, "load", 1));
            JS_SetPropertyStr(ctx, addon, "unload", JS_NewCFunction(ctx, JsAddonUnload, "unload", 1));
            JS_SetModuleExport(ctx, m, "addon", addon);

            return 0;
        }

        int NovadeskModuleInit(JSContext *ctx, JSModuleDef *m)
        {
            EnsureWidgetWindowClass(ctx);
            JSValue ctor = JS_NewCFunction2(ctx, JsWidgetWindowCtor, "widgetWindow", 1, JS_CFUNC_constructor, 0);
            JSValue proto = JS_GetClassProto(ctx, EnsureWidgetWindowClass(ctx));
            JS_SetConstructor(ctx, ctor, proto);
            JS_SetModuleExport(ctx, m, "widgetWindow", ctor);
            JS_FreeValue(ctx, proto);
            InitAppExport(ctx, m);
            JSValue trayCtor = JS_NewCFunction2(ctx, JsTrayCtor, "tray", 1, JS_CFUNC_constructor, 0);
            JS_SetModuleExport(ctx, m, "tray", trayCtor);
            return 0;
        }
    } // namespace

    void SetModuleDebug(bool debug)
    {
        g_moduleDebug = debug;
        SetWidgetUiDebug(debug);
    }

    JSModuleDef *EnsureNovadeskModule(JSContext *ctx, const char *moduleName)
    {
        JSModuleDef *m = JS_NewCModule(ctx, moduleName, NovadeskModuleInit);
        if (!m)
        {
            return nullptr;
        }
        if (JS_AddModuleExport(ctx, m, "widgetWindow") < 0)
        {
            return nullptr;
        }
        if (JS_AddModuleExport(ctx, m, "app") < 0)
        {
            return nullptr;
        }
        if (JS_AddModuleExport(ctx, m, "tray") < 0)
        {
            return nullptr;
        }
        if (JS_AddModuleExport(ctx, m, "addon") < 0)
        {
            return nullptr;
        }
        return m;
    }

    void UnloadAllAddons()
    {
        UnloadAllAddonsInternal();
    }
} // namespace novadesk::scripting::quickjs
