#include "JSEngine.h"

#include "quickjs.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "../../domain/Widget.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/System.h"
#include "../../shared/Utils.h"
#include "../../domain/Novadesk.h"
#include "../modules/ModuleSystem.h"
#include "../modules/WidgetUiBindings.h"

namespace JSEngine
{
    namespace
    {
        struct IpcListener;
        struct IpcHandler;
        void LogQuickJsException(JSContext *ctx);
        JSValue CreateMainIpcObject(JSContext *ctx);
        void ClearWidgetEventListeners();
        void ClearAllWidgetContextMenuCallbacks();
        void ClearAllTrayCommandCallbacksInternal();
        void ClearAllTrayEventCallbacksInternal();
        void ClearAllTimers();
        void ClearCallbacks(std::vector<IpcListener> &list);
        void ClearChannelMap(std::unordered_map<std::string, std::vector<IpcListener>> &map);
        void ClearHandlerMap(std::unordered_map<std::string, IpcHandler> &map);
        HWND g_messageWindow = nullptr;
        JSRuntime *g_runtime = nullptr;
        JSContext *g_context = nullptr;
        std::wstring g_lastScriptPath;
        std::wstring g_mainScriptPath;
        std::wstring g_currentScriptDir;
        std::wstring g_currentScriptPath;
        std::vector<std::wstring> g_loadedScriptPaths;
        std::vector<JSValue> g_eventCallbacks;
        std::unordered_map<Widget *, std::unordered_map<std::string, std::vector<int>>> g_widgetEventListeners;
        std::unordered_map<std::wstring, std::unordered_map<int, JSValue>> g_widgetContextMenuCallbacks;
        struct TrayCommandCallback
        {
            int trayId = 0;
            JSValue callback = JS_UNDEFINED;
        };
        std::unordered_map<int, TrayCommandCallback> g_trayCommandCallbacks;
        std::unordered_map<int, std::unordered_map<std::string, std::vector<JSValue>>> g_trayEventCallbacks;
        struct IpcListener
        {
            JSValue callback = JS_UNDEFINED;
            std::wstring owner;
        };
        struct IpcHandler
        {
            JSValue callback = JS_UNDEFINED;
            std::wstring owner;
        };
        struct TimerEntry
        {
            JSValue callback = JS_UNDEFINED;
            std::vector<JSValue> args;
            bool repeat = false;
            std::wstring owner;
        };
        std::unordered_map<UINT_PTR, TimerEntry> g_timers;
        UINT_PTR g_nextTimerId = 50000;
        std::vector<IpcListener> g_mainIpcListeners;
        std::vector<IpcListener> g_uiIpcListeners;
        std::unordered_map<std::string, std::vector<IpcListener>> g_mainIpcChannelListeners;
        std::unordered_map<std::string, std::vector<IpcListener>> g_uiIpcChannelListeners;
        std::unordered_map<std::string, IpcHandler> g_mainIpcHandlers;
        std::unordered_map<Widget *, std::wstring> g_widgetOwners;
        std::unordered_map<int, std::wstring> g_trayOwners;
        std::unordered_set<std::wstring> g_staleScripts;
        std::unordered_map<std::wstring, int> g_scriptEvalRevisions;

        class ScriptExecutionScope
        {
        public:
            explicit ScriptExecutionScope(const std::wstring &ownerScriptPath)
                : m_PreviousScriptPath(g_currentScriptPath), m_PreviousScriptDir(g_currentScriptDir)
            {
                if (!ownerScriptPath.empty())
                {
                    g_currentScriptPath = ownerScriptPath;
                    g_currentScriptDir = PathUtils::GetParentDir(ownerScriptPath);
                }
            }

            ~ScriptExecutionScope()
            {
                g_currentScriptPath = m_PreviousScriptPath;
                g_currentScriptDir = m_PreviousScriptDir;
            }

        private:
            std::wstring m_PreviousScriptPath;
            std::wstring m_PreviousScriptDir;
        };

        std::wstring GetWidgetOwnerScriptPath(Widget *widget)
        {
            if (!widget)
            {
                return L"";
            }

            auto it = g_widgetOwners.find(widget);
            if (it == g_widgetOwners.end())
            {
                return L"";
            }
            return it->second;
        }

        std::wstring GetWidgetOwnerScriptPathById(const std::wstring &widgetId)
        {
            if (widgetId.empty())
            {
                return L"";
            }

            for (const auto &entry : g_widgetOwners)
            {
                Widget *widget = entry.first;
                if (!widget)
                {
                    continue;
                }
                if (widget->GetOptions().id == widgetId)
                {
                    return entry.second;
                }
            }
            return L"";
        }

        void DestroyAllWidgets()
        {
            std::vector<Widget *> copy = Widget::GetAllWidgets();
            Widget::ClearAllWidgets();
            for (auto w : copy)
            {
                delete w;
            }
            g_widgetOwners.clear();
            g_staleScripts.clear();
        }

        void DestroyAllTrays()
        {
            std::vector<int> ids;
            ids.reserve(g_trayOwners.size());
            for (auto &kv : g_trayOwners)
            {
                ids.push_back(kv.first);
            }
            for (int trayId : ids)
            {
                TrayDestroy(trayId);
                ClearTrayEventCallbacks(trayId);
                ClearTrayCommandCallbacks(trayId);
            }
            g_trayOwners.clear();
        }

        void ResetRuntime()
        {
            ClearAllTimers();
            for (size_t i = 1; i < g_eventCallbacks.size(); ++i)
            {
                JS_FreeValue(g_context, g_eventCallbacks[i]);
            }
            g_eventCallbacks.clear();
            g_eventCallbacks.push_back(JS_UNDEFINED);
            ClearWidgetEventListeners();
            ClearAllWidgetContextMenuCallbacks();
            ClearAllTrayCommandCallbacksInternal();
            ClearAllTrayEventCallbacksInternal();
            ClearCallbacks(g_mainIpcListeners);
            ClearCallbacks(g_uiIpcListeners);
            ClearChannelMap(g_mainIpcChannelListeners);
            ClearChannelMap(g_uiIpcChannelListeners);
            ClearHandlerMap(g_mainIpcHandlers);
            g_widgetOwners.clear();
            g_trayOwners.clear();

            if (g_context)
            {
                JS_FreeContext(g_context);
                g_context = nullptr;
            }
            if (g_runtime)
            {
                JS_FreeRuntime(g_runtime);
                g_runtime = nullptr;
            }
        }

        void DestroyWidgetsForScript(const std::wstring &scriptPath)
        {
            std::vector<Widget *> toDelete;
            auto &all = Widget::GetAllWidgets();
            for (auto *w : all)
            {
                auto it = g_widgetOwners.find(w);
                if (it != g_widgetOwners.end() && it->second == scriptPath)
                {
                    toDelete.push_back(w);
                }
            }
            if (!toDelete.empty())
            {
                auto &allWidgets = Widget::GetAllWidgets();
                std::unordered_set<Widget *> toDeleteSet;
                toDeleteSet.reserve(toDelete.size());
                for (auto *w : toDelete)
                    toDeleteSet.insert(w);
                allWidgets.erase(std::remove_if(allWidgets.begin(), allWidgets.end(),
                                                [&](Widget *w)
                                                { return toDeleteSet.find(w) != toDeleteSet.end(); }),
                                 allWidgets.end());
                for (auto *w : toDelete)
                {
                    g_widgetOwners.erase(w);
                    delete w;
                }
            }
        }

        void ClearTimersForScript(const std::wstring &scriptPath)
        {
            if (!g_messageWindow)
                return;
            for (auto it = g_timers.begin(); it != g_timers.end();)
            {
                if (it->second.owner == scriptPath)
                {
                    KillTimer(g_messageWindow, it->first);
                    JS_FreeValue(g_context, it->second.callback);
                    for (JSValue &a : it->second.args)
                    {
                        JS_FreeValue(g_context, a);
                    }
                    it = g_timers.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void ClearIpcListenersForScript(std::vector<IpcListener> &list, const std::wstring &scriptPath)
        {
            for (auto it = list.begin(); it != list.end();)
            {
                if (it->owner == scriptPath)
                {
                    JS_FreeValue(g_context, it->callback);
                    it = list.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void ClearIpcChannelListenersForScript(std::unordered_map<std::string, std::vector<IpcListener>> &map, const std::wstring &scriptPath)
        {
            for (auto mit = map.begin(); mit != map.end();)
            {
                auto &vec = mit->second;
                for (auto it = vec.begin(); it != vec.end();)
                {
                    if (it->owner == scriptPath)
                    {
                        JS_FreeValue(g_context, it->callback);
                        it = vec.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                if (vec.empty())
                {
                    mit = map.erase(mit);
                }
                else
                {
                    ++mit;
                }
            }
        }

        void ClearIpcHandlersForScript(std::unordered_map<std::string, IpcHandler> &map, const std::wstring &scriptPath)
        {
            for (auto it = map.begin(); it != map.end();)
            {
                if (it->second.owner == scriptPath)
                {
                    JS_FreeValue(g_context, it->second.callback);
                    it = map.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void ClearTraysForScript(const std::wstring &scriptPath)
        {
            std::vector<int> toRemove;
            for (auto &kv : g_trayOwners)
            {
                if (kv.second == scriptPath)
                {
                    toRemove.push_back(kv.first);
                }
            }
            for (int trayId : toRemove)
            {
                TrayDestroy(trayId);
                ClearTrayEventCallbacks(trayId);
                ClearTrayCommandCallbacks(trayId);
                g_trayOwners.erase(trayId);
            }
        }

        bool ExecuteScriptFile(const std::wstring &finalScriptPath)
        {
            const std::string script = FileUtils::ReadFileContent(finalScriptPath);
            if (script.empty())
            {
                Logging::Log(LogLevel::Error, L"Failed to load script: %s", finalScriptPath.c_str());
                return false;
            }

            if (!g_context)
            {
                Logging::Log(LogLevel::Error, L"QuickJS context not initialized");
                return false;
            }

            const std::string fileName = Utils::ToString(finalScriptPath);
            const int revision = ++g_scriptEvalRevisions[finalScriptPath];
            const std::string evalModuleName = fileName + "#rev=" + std::to_string(revision);
            const std::wstring scriptDir = PathUtils::GetParentDir(finalScriptPath);
            const std::string dirName = Utils::ToString(scriptDir);
            g_currentScriptDir = scriptDir;
            g_currentScriptPath = finalScriptPath;

            JSValue global = JS_GetGlobalObject(g_context);
            JSValue mainIpc = CreateMainIpcObject(g_context);
            JS_SetPropertyStr(g_context, global, "ipcMain", JS_DupValue(g_context, mainIpc));
            JS_SetPropertyStr(g_context, global, "__filename", JS_NewString(g_context, fileName.c_str()));
            JS_SetPropertyStr(g_context, global, "__dirname", JS_NewString(g_context, dirName.c_str()));
            const std::wstring effectiveMainScriptPath = g_mainScriptPath.empty() ? finalScriptPath : g_mainScriptPath;
            const std::wstring effectiveMainScriptDirPath = PathUtils::GetParentDir(effectiveMainScriptPath);
            JS_SetPropertyStr(g_context, global, "__mainScriptDirPath", JS_NewString(g_context, Utils::ToString(effectiveMainScriptDirPath).c_str()));
            JS_SetPropertyStr(g_context, global, "__widgetDir", JS_NewString(g_context, Utils::ToString(PathUtils::GetWidgetsDir()).c_str()));
            JS_SetPropertyStr(g_context, global, "__addonsPath", JS_NewString(g_context, Utils::ToString(PathUtils::GetAddonsDir()).c_str()));
            JS_FreeValue(g_context, mainIpc);
            JS_FreeValue(g_context, global);

            const std::string modulePrelude =
                "const ipcMain = globalThis.ipcMain;\n"
                "const __filename = globalThis.__filename;\n"
                "const __dirname = globalThis.__dirname;\n"
                "const __mainScriptDirPath = globalThis.__mainScriptDirPath;\n"
                "const __widgetDir = globalThis.__widgetDir;\n"
                "const __addonsPath = globalThis.__addonsPath;\n"
                "const path = globalThis.path;\n";
            const std::string moduleSource = modulePrelude + script;

            JSValue result = JS_Eval(
                g_context,
                moduleSource.c_str(),
                moduleSource.size(),
                evalModuleName.c_str(),
                JS_EVAL_TYPE_MODULE);

            if (JS_IsException(result))
            {
                LogQuickJsException(g_context);
                JS_FreeValue(g_context, result);
                g_currentScriptDir.clear();
                g_currentScriptPath.clear();
                return false;
            }
            JS_FreeValue(g_context, result);

            JSContext *ctx1 = nullptr;
            int err = 0;
            while (JS_IsJobPending(g_runtime))
            {
                err = JS_ExecutePendingJob(g_runtime, &ctx1);
                if (err < 0)
                {
                    LogQuickJsException(ctx1 ? ctx1 : g_context);
                    g_currentScriptDir.clear();
                    g_currentScriptPath.clear();
                    return false;
                }
            }

            g_currentScriptDir.clear();
            g_currentScriptPath.clear();
            return true;
        }

        enum class ConsoleLevel
        {
            Log = 0,
            Info = 1,
            Warn = 2,
            Error = 3,
            Debug = 4
        };

        std::wstring JsValueToWString(JSContext *ctx, JSValueConst value)
        {
            const char *s = JS_ToCString(ctx, value);
            if (!s)
            {
                return L"<unprintable>";
            }
            std::wstring out = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            return out;
        }

        JSValue JsConsoleWrite(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic)
        {
            std::wstring msg;
            for (int i = 0; i < argc; ++i)
            {
                if (!msg.empty())
                    msg += L" ";
                msg += JsValueToWString(ctx, argv[i]);
            }

            LogLevel lvl = LogLevel::Info;
            switch (static_cast<ConsoleLevel>(magic))
            {
            case ConsoleLevel::Log:
                lvl = LogLevel::Info;
                break;
            case ConsoleLevel::Info:
                lvl = LogLevel::Info;
                break;
            case ConsoleLevel::Warn:
                lvl = LogLevel::Warn;
                break;
            case ConsoleLevel::Error:
                lvl = LogLevel::Error;
                break;
            case ConsoleLevel::Debug:
                lvl = LogLevel::Debug;
                break;
            }
            Logging::Log(lvl, L"%s", msg.c_str());
            return JS_UNDEFINED;
        }

        JSValue JsSetTimerImpl(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic)
        {
            if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
            {
                return JS_ThrowTypeError(ctx, "timer requires callback");
            }
            if (!g_messageWindow)
            {
                return JS_ThrowInternalError(ctx, "Message window is not initialized");
            }

            int32_t delay = 0;
            if (argc > 1)
                JS_ToInt32(ctx, &delay, argv[1]);
            if (delay < 0)
                delay = 0;

            UINT_PTR id = g_nextTimerId++;
            if (SetTimer(g_messageWindow, id, static_cast<UINT>(delay), nullptr) == 0)
            {
                return JS_ThrowInternalError(ctx, "SetTimer failed");
            }

            TimerEntry entry{};
            entry.callback = JS_DupValue(ctx, argv[0]);
            entry.repeat = (magic != 0);
            entry.owner = g_currentScriptPath;
            for (int i = 2; i < argc; ++i)
            {
                entry.args.push_back(JS_DupValue(ctx, argv[i]));
            }
            g_timers[id] = std::move(entry);
            return JS_NewInt64(ctx, static_cast<int64_t>(id));
        }

        JSValue JsClearTimerImpl(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_UNDEFINED;
            int64_t id64 = 0;
            if (JS_ToInt64(ctx, &id64, argv[0]) != 0)
                return JS_UNDEFINED;
            UINT_PTR id = static_cast<UINT_PTR>(id64);
            auto it = g_timers.find(id);
            if (it == g_timers.end())
                return JS_UNDEFINED;

            if (g_messageWindow)
                KillTimer(g_messageWindow, id);
            JS_FreeValue(ctx, it->second.callback);
            for (JSValue &a : it->second.args)
                JS_FreeValue(ctx, a);
            g_timers.erase(it);
            return JS_UNDEFINED;
        }

        JSValue JsPathJoin(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            std::vector<std::wstring> parts;
            parts.reserve(argc);
            for (int i = 0; i < argc; ++i)
            {
                const char *s = JS_ToCString(ctx, argv[i]);
                if (!s)
                    return JS_EXCEPTION;
                parts.push_back(Utils::ToWString(s));
                JS_FreeCString(ctx, s);
            }
            const std::wstring out = PathUtils::Join(parts);
            const std::string utf8 = Utils::ToString(out);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathBasename(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_NewString(ctx, "");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            std::wstring input = Utils::ToWString(s);
            std::wstring ext;
            if (argc > 1)
            {
                const char *extArg = JS_ToCString(ctx, argv[1]);
                if (extArg)
                {
                    ext = Utils::ToWString(extArg);
                    JS_FreeCString(ctx, extArg);
                }
            }
            JS_FreeCString(ctx, s);
            const std::wstring base = PathUtils::Basename(input, ext);
            const std::string utf8 = Utils::ToString(base);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathDirname(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_NewString(ctx, ".");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            const std::wstring dir = PathUtils::Dirname(Utils::ToWString(s));
            JS_FreeCString(ctx, s);
            const std::string utf8 = Utils::ToString(dir);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathExtname(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_NewString(ctx, "");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            const std::wstring ext = PathUtils::Extname(Utils::ToWString(s));
            JS_FreeCString(ctx, s);
            const std::string utf8 = Utils::ToString(ext);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathIsAbsolute(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_NewBool(ctx, 0);
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            bool abs = PathUtils::IsAbsolute(Utils::ToWString(s));
            JS_FreeCString(ctx, s);
            return JS_NewBool(ctx, abs ? 1 : 0);
        }

        JSValue JsPathNormalize(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_NewString(ctx, ".");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            const std::wstring n = PathUtils::NormalizePath(Utils::ToWString(s));
            JS_FreeCString(ctx, s);
            const std::string utf8 = Utils::ToString(n);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathRelative(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_NewString(ctx, "");
            const char *from = JS_ToCString(ctx, argv[0]);
            const char *to = JS_ToCString(ctx, argv[1]);
            if (!from || !to)
            {
                if (from)
                    JS_FreeCString(ctx, from);
                if (to)
                    JS_FreeCString(ctx, to);
                return JS_EXCEPTION;
            }
            const std::wstring rel = PathUtils::Relative(Utils::ToWString(from), Utils::ToWString(to));
            JS_FreeCString(ctx, from);
            JS_FreeCString(ctx, to);
            const std::string utf8 = Utils::ToString(rel);
            return JS_NewString(ctx, utf8.c_str());
        }

        JSValue JsPathParse(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "path.parse(path) requires path");
            const char *s = JS_ToCString(ctx, argv[0]);
            if (!s)
                return JS_EXCEPTION;
            const auto parts = PathUtils::Parse(Utils::ToWString(s));
            JS_FreeCString(ctx, s);

            JSValue obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "root", JS_NewString(ctx, Utils::ToString(parts.root).c_str()));
            JS_SetPropertyStr(ctx, obj, "dir", JS_NewString(ctx, Utils::ToString(parts.dir).c_str()));
            JS_SetPropertyStr(ctx, obj, "base", JS_NewString(ctx, Utils::ToString(parts.base).c_str()));
            JS_SetPropertyStr(ctx, obj, "ext", JS_NewString(ctx, Utils::ToString(parts.ext).c_str()));
            JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, Utils::ToString(parts.name).c_str()));
            return obj;
        }

        JSValue JsPathFormat(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1 || !JS_IsObject(argv[0]))
            {
                return JS_ThrowTypeError(ctx, "path.format(obj) requires object");
            }
            PathUtils::PathParts parts;

            JSValue v = JS_GetPropertyStr(ctx, argv[0], "dir");
            if (!JS_IsUndefined(v) && !JS_IsNull(v))
            {
                const char *s = JS_ToCString(ctx, v);
                if (s)
                {
                    parts.dir = Utils::ToWString(s);
                    JS_FreeCString(ctx, s);
                }
            }
            JS_FreeValue(ctx, v);

            v = JS_GetPropertyStr(ctx, argv[0], "base");
            if (!JS_IsUndefined(v) && !JS_IsNull(v))
            {
                const char *s = JS_ToCString(ctx, v);
                if (s)
                {
                    parts.base = Utils::ToWString(s);
                    JS_FreeCString(ctx, s);
                }
            }
            JS_FreeValue(ctx, v);

            if (parts.base.empty())
            {
                v = JS_GetPropertyStr(ctx, argv[0], "name");
                if (!JS_IsUndefined(v) && !JS_IsNull(v))
                {
                    const char *s = JS_ToCString(ctx, v);
                    if (s)
                    {
                        parts.name = Utils::ToWString(s);
                        JS_FreeCString(ctx, s);
                    }
                }
                JS_FreeValue(ctx, v);

                v = JS_GetPropertyStr(ctx, argv[0], "ext");
                if (!JS_IsUndefined(v) && !JS_IsNull(v))
                {
                    const char *s = JS_ToCString(ctx, v);
                    if (s)
                    {
                        parts.ext = Utils::ToWString(s);
                        JS_FreeCString(ctx, s);
                    }
                }
                JS_FreeValue(ctx, v);
            }

            const std::wstring out = PathUtils::Format(parts);
            const std::string utf8 = Utils::ToString(out);
            return JS_NewString(ctx, utf8.c_str());
        }

        void RegisterConsoleBindings(JSContext *ctx)
        {
            JSValue global = JS_GetGlobalObject(ctx);

            JSValue printFn = JS_NewCFunction(ctx, [](JSContext *c, JSValueConst thisVal, int argc, JSValueConst *argv) -> JSValue
                                              { return JsConsoleWrite(c, thisVal, argc, argv, static_cast<int>(ConsoleLevel::Log)); }, "print", 1);
            JS_SetPropertyStr(ctx, global, "print", printFn);

            JSValue consoleObj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, consoleObj, "log",
                              JS_NewCFunctionMagic(ctx, JsConsoleWrite, "log", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Log)));
            JS_SetPropertyStr(ctx, consoleObj, "info",
                              JS_NewCFunctionMagic(ctx, JsConsoleWrite, "info", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Info)));
            JS_SetPropertyStr(ctx, consoleObj, "warn",
                              JS_NewCFunctionMagic(ctx, JsConsoleWrite, "warn", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Warn)));
            JS_SetPropertyStr(ctx, consoleObj, "error",
                              JS_NewCFunctionMagic(ctx, JsConsoleWrite, "error", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Error)));
            JS_SetPropertyStr(ctx, consoleObj, "debug",
                              JS_NewCFunctionMagic(ctx, JsConsoleWrite, "debug", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Debug)));
            JS_SetPropertyStr(ctx, global, "console", consoleObj);
            JS_SetPropertyStr(ctx, global, "setTimeout",
                              JS_NewCFunctionMagic(ctx, JsSetTimerImpl, "setTimeout", 2, JS_CFUNC_generic_magic, 0));
            JS_SetPropertyStr(ctx, global, "setInterval",
                              JS_NewCFunctionMagic(ctx, JsSetTimerImpl, "setInterval", 2, JS_CFUNC_generic_magic, 1));
            JS_SetPropertyStr(ctx, global, "clearTimeout",
                              JS_NewCFunction(ctx, JsClearTimerImpl, "clearTimeout", 1));
            JS_SetPropertyStr(ctx, global, "clearInterval",
                              JS_NewCFunction(ctx, JsClearTimerImpl, "clearInterval", 1));

            JSValue pathObj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, pathObj, "join", JS_NewCFunction(ctx, JsPathJoin, "join", 1));
            JS_SetPropertyStr(ctx, pathObj, "basename", JS_NewCFunction(ctx, JsPathBasename, "basename", 2));
            JS_SetPropertyStr(ctx, pathObj, "dirname", JS_NewCFunction(ctx, JsPathDirname, "dirname", 1));
            JS_SetPropertyStr(ctx, pathObj, "extname", JS_NewCFunction(ctx, JsPathExtname, "extname", 1));
            JS_SetPropertyStr(ctx, pathObj, "isAbsolute", JS_NewCFunction(ctx, JsPathIsAbsolute, "isAbsolute", 1));
            JS_SetPropertyStr(ctx, pathObj, "normalize", JS_NewCFunction(ctx, JsPathNormalize, "normalize", 1));
            JS_SetPropertyStr(ctx, pathObj, "relative", JS_NewCFunction(ctx, JsPathRelative, "relative", 2));
            JS_SetPropertyStr(ctx, pathObj, "parse", JS_NewCFunction(ctx, JsPathParse, "parse", 1));
            JS_SetPropertyStr(ctx, pathObj, "format", JS_NewCFunction(ctx, JsPathFormat, "format", 1));
            JS_SetPropertyStr(ctx, global, "path", pathObj);

            JS_FreeValue(ctx, global);
        }

        void LogQuickJsException(JSContext *ctx)
        {
            JSValue ex = JS_GetException(ctx);

            // Prefer Error.message when available; fall back to exception string.
            JSValue msgV = JS_GetPropertyStr(ctx, ex, "message");
            const char *msg = nullptr;
            if (!JS_IsException(msgV) && !JS_IsUndefined(msgV) && !JS_IsNull(msgV))
            {
                msg = JS_ToCString(ctx, msgV);
            }
            if (!msg)
            {
                msg = JS_ToCString(ctx, ex);
            }

            if (msg)
            {
                Logging::Log(LogLevel::Error, L"QuickJS uncaught exception: %S", msg);
                JS_FreeCString(ctx, msg);
            }
            else
            {
                Logging::Log(LogLevel::Error, L"QuickJS uncaught exception (unknown)");
            }
            JS_FreeValue(ctx, msgV);

            // Log stack trace when present (works for Error objects).
            JSValue stackV = JS_GetPropertyStr(ctx, ex, "stack");
            if (!JS_IsException(stackV) && !JS_IsUndefined(stackV) && !JS_IsNull(stackV))
            {
                const char *stack = JS_ToCString(ctx, stackV);
                if (stack && stack[0] != '\0')
                {
                    Logging::Log(LogLevel::Error, L"QuickJS stack:\n%S", stack);
                    JS_FreeCString(ctx, stack);
                }
            }
            JS_FreeValue(ctx, stackV);

            JS_FreeValue(ctx, ex);
        }

        void LogQuickJsValue(JSContext *ctx, JSValueConst value, const wchar_t *prefix)
        {
            JSValue msgV = JS_GetPropertyStr(ctx, value, "message");
            const char *msg = nullptr;
            if (!JS_IsException(msgV) && !JS_IsUndefined(msgV) && !JS_IsNull(msgV))
            {
                msg = JS_ToCString(ctx, msgV);
            }
            if (!msg)
            {
                msg = JS_ToCString(ctx, value);
            }

            if (msg)
            {
                Logging::Log(LogLevel::Error, L"%s: %S", prefix, msg);
                JS_FreeCString(ctx, msg);
            }
            else
            {
                Logging::Log(LogLevel::Error, L"%s", prefix);
            }
            JS_FreeValue(ctx, msgV);

            JSValue stackV = JS_GetPropertyStr(ctx, value, "stack");
            if (!JS_IsException(stackV) && !JS_IsUndefined(stackV) && !JS_IsNull(stackV))
            {
                const char *stack = JS_ToCString(ctx, stackV);
                if (stack && stack[0] != '\0')
                {
                    Logging::Log(LogLevel::Error, L"QuickJS stack:\n%S", stack);
                    JS_FreeCString(ctx, stack);
                }
            }
            JS_FreeValue(ctx, stackV);
        }

        void HostPromiseRejectionTracker(JSContext *ctx, JSValueConst, JSValueConst reason, bool is_handled, void *)
        {
            if (is_handled)
            {
                return;
            }
            LogQuickJsValue(ctx, reason, L"QuickJS unhandled promise rejection");
        }

        void ClearCallbacks(std::vector<IpcListener> &list)
        {
            for (auto &v : list)
            {
                JS_FreeValue(g_context, v.callback);
            }
            list.clear();
        }

        void ClearChannelMap(std::unordered_map<std::string, std::vector<IpcListener>> &map)
        {
            for (auto &kv : map)
            {
                for (auto &v : kv.second)
                {
                    JS_FreeValue(g_context, v.callback);
                }
            }
            map.clear();
        }

        void ClearHandlerMap(std::unordered_map<std::string, IpcHandler> &map)
        {
            for (auto &kv : map)
            {
                JS_FreeValue(g_context, kv.second.callback);
            }
            map.clear();
        }

        void ClearWidgetEventListeners()
        {
            g_widgetEventListeners.clear();
        }

        void ClearAllWidgetContextMenuCallbacks()
        {
            if (!g_context)
                return;
            for (auto &wkv : g_widgetContextMenuCallbacks)
            {
                for (auto &ckv : wkv.second)
                {
                    JS_FreeValue(g_context, ckv.second);
                }
            }
            g_widgetContextMenuCallbacks.clear();
        }

        void ClearAllTrayCommandCallbacksInternal()
        {
            if (!g_context)
                return;
            for (auto &kv : g_trayCommandCallbacks)
            {
                JS_FreeValue(g_context, kv.second.callback);
            }
            g_trayCommandCallbacks.clear();
        }

        void ClearAllTrayEventCallbacksInternal()
        {
            if (!g_context)
                return;
            for (auto &kv : g_trayEventCallbacks)
            {
                for (auto &ekv : kv.second)
                {
                    for (JSValue &cb : ekv.second)
                    {
                        JS_FreeValue(g_context, cb);
                    }
                }
            }
            g_trayEventCallbacks.clear();
        }

        void ClearAllTimers()
        {
            if (g_messageWindow)
            {
                for (const auto &kv : g_timers)
                {
                    KillTimer(g_messageWindow, kv.first);
                }
            }
            for (auto &kv : g_timers)
            {
                JS_FreeValue(g_context, kv.second.callback);
                for (JSValue &a : kv.second.args)
                {
                    JS_FreeValue(g_context, a);
                }
            }
            g_timers.clear();
        }

        int RegisterCallback(std::vector<JSValue> &list, JSContext *ctx, JSValueConst fn)
        {
            if (!ctx || !JS_IsFunction(ctx, fn) || ctx != g_context)
            {
                return -1;
            }
            list.push_back(JS_DupValue(g_context, fn));
            return static_cast<int>(list.size() - 1);
        }

        bool GetChannelArg(JSContext *ctx, JSValueConst v, std::string &out)
        {
            const char *s = JS_ToCString(ctx, v);
            if (!s || !*s)
            {
                if (s)
                    JS_FreeCString(ctx, s);
                return false;
            }
            out = s;
            JS_FreeCString(ctx, s);
            return true;
        }

        void RegisterChannelListener(std::unordered_map<std::string, std::vector<IpcListener>> &map, JSContext *ctx, const std::string &channel, JSValueConst fn)
        {
            IpcListener entry{};
            entry.callback = JS_DupValue(ctx, fn);
            entry.owner = g_currentScriptPath;
            map[channel].push_back(std::move(entry));
        }

        JSValue BuildIpcMessage(JSContext *ctx, JSValueConst typeVal, JSValueConst payloadVal, const char *from, const char *to, const char *channel)
        {
            JSValue msg = JS_NewObject(ctx);

            std::string type = "message";
            const char *t = JS_ToCString(ctx, typeVal);
            if (t && *t)
            {
                type = t;
            }
            if (t)
                JS_FreeCString(ctx, t);

            JS_SetPropertyStr(ctx, msg, "type", JS_NewString(ctx, type.c_str()));
            JS_SetPropertyStr(ctx, msg, "payload", JS_DupValue(ctx, payloadVal));
            JS_SetPropertyStr(ctx, msg, "from", JS_NewString(ctx, from));
            JS_SetPropertyStr(ctx, msg, "to", JS_NewString(ctx, to));
            JS_SetPropertyStr(ctx, msg, "channel", JS_NewString(ctx, channel ? channel : type.c_str()));
            return msg;
        }

        void DispatchIpc(std::vector<IpcListener> &listeners, JSValueConst message)
        {
            for (auto &cb : listeners)
            {
                JSValue argv[1] = {JS_DupValue(g_context, message)};
                JSValue ret = JS_Call(g_context, cb.callback, JS_UNDEFINED, 1, argv);
                JS_FreeValue(g_context, argv[0]);
                if (JS_IsException(ret))
                {
                    LogQuickJsException(g_context);
                }
                else
                {
                    JS_FreeValue(g_context, ret);
                }
            }
        }

        void DispatchChannelIpc(std::unordered_map<std::string, std::vector<IpcListener>> &listeners, const std::string &channel, const char *from, const char *to, JSValueConst payload, bool payloadFirst = false)
        {
            auto it = listeners.find(channel);
            if (it == listeners.end())
                return;

            JSValue channelVal = JS_NewString(g_context, channel.c_str());
            JSValue eventObj = BuildIpcMessage(g_context, channelVal, payload, from, to, channel.c_str());
            JS_FreeValue(g_context, channelVal);

            for (auto &cb : it->second)
            {
                JSValue argv[2] = {JS_UNDEFINED, JS_UNDEFINED};
                if (payloadFirst)
                {
                    // Backward compatibility for legacy UI scripts: callback(payload, event)
                    argv[0] = JS_DupValue(g_context, payload);
                    argv[1] = JS_DupValue(g_context, eventObj);
                }
                else
                {
                    // Default callback(event, payload)
                    argv[0] = JS_DupValue(g_context, eventObj);
                    argv[1] = JS_DupValue(g_context, payload);
                }
                JSValue ret = JS_Call(g_context, cb.callback, JS_UNDEFINED, 2, argv);
                JS_FreeValue(g_context, argv[0]);
                JS_FreeValue(g_context, argv[1]);
                if (JS_IsException(ret))
                {
                    LogQuickJsException(g_context);
                }
                else
                {
                    JS_FreeValue(g_context, ret);
                }
            }

            JS_FreeValue(g_context, eventObj);
        }

        JSValue JsMainIpcOn(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2 || !JS_IsFunction(ctx, argv[1]))
            {
                return JS_ThrowTypeError(ctx, "ipcMain.on requires (channel, listener)");
            }
            std::string channel;
            if (!GetChannelArg(ctx, argv[0], channel))
            {
                return JS_ThrowTypeError(ctx, "ipcMain.on channel must be non-empty string");
            }
            RegisterChannelListener(g_mainIpcChannelListeners, ctx, channel, argv[1]);
            return JS_UNDEFINED;
        }

        JSValue JsMainIpcHandle(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2 || !JS_IsFunction(ctx, argv[1]))
            {
                return JS_ThrowTypeError(ctx, "ipcMain.handle requires (channel, handler)");
            }
            std::string channel;
            if (!GetChannelArg(ctx, argv[0], channel))
            {
                return JS_ThrowTypeError(ctx, "ipcMain.handle channel must be non-empty string");
            }
            auto it = g_mainIpcHandlers.find(channel);
            if (it != g_mainIpcHandlers.end())
            {
                JS_FreeValue(ctx, it->second.callback);
            }
            IpcHandler handler{};
            handler.callback = JS_DupValue(ctx, argv[1]);
            handler.owner = g_currentScriptPath;
            g_mainIpcHandlers[channel] = std::move(handler);
            return JS_UNDEFINED;
        }

        JSValue JsMainIpcSend(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "ipc.sendToUi requires at least a type");
            }
            JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
            JSValue msg = BuildIpcMessage(ctx, argv[0], payload, "main", "ui", nullptr);
            DispatchIpc(g_uiIpcListeners, msg);
            JS_FreeValue(ctx, msg);
            std::string channel;
            if (GetChannelArg(ctx, argv[0], channel))
            {
                DispatchChannelIpc(g_uiIpcChannelListeners, channel, "main", "ui", payload);
            }
            return JS_UNDEFINED;
        }

        JSValue JsUiIpcOn(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2 || !JS_IsFunction(ctx, argv[1]))
            {
                return JS_ThrowTypeError(ctx, "ipcRenderer.on requires (channel, listener)");
            }
            std::string channel;
            if (!GetChannelArg(ctx, argv[0], channel))
            {
                return JS_ThrowTypeError(ctx, "ipcRenderer.on channel must be non-empty string");
            }
            RegisterChannelListener(g_uiIpcChannelListeners, ctx, channel, argv[1]);
            return JS_UNDEFINED;
        }

        JSValue JsUiIpcSend(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "ipc.sendToMain requires at least a type");
            }
            JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
            JSValue msg = BuildIpcMessage(ctx, argv[0], payload, "ui", "main", nullptr);
            DispatchIpc(g_mainIpcListeners, msg);
            JS_FreeValue(ctx, msg);
            std::string channel;
            if (GetChannelArg(ctx, argv[0], channel))
            {
                DispatchChannelIpc(g_mainIpcChannelListeners, channel, "ui", "main", payload);
            }
            return JS_UNDEFINED;
        }

        JSValue JsUiIpcInvoke(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
            {
                return JS_ThrowTypeError(ctx, "ipcRenderer.invoke requires at least a channel");
            }
            std::string channel;
            if (!GetChannelArg(ctx, argv[0], channel))
            {
                return JS_ThrowTypeError(ctx, "ipcRenderer.invoke channel must be non-empty string");
            }
            auto it = g_mainIpcHandlers.find(channel);
            if (it == g_mainIpcHandlers.end())
            {
                return JS_ThrowReferenceError(ctx, "No ipcMain handler for channel: %s", channel.c_str());
            }

            JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
            JSValue channelVal = JS_NewString(ctx, channel.c_str());
            JSValue eventObj = BuildIpcMessage(ctx, channelVal, payload, "ui", "main", channel.c_str());
            JS_FreeValue(ctx, channelVal);

            JSValue callArgs[2] = {eventObj, JS_DupValue(ctx, payload)};
            JSValue ret = JS_Call(ctx, it->second.callback, JS_UNDEFINED, 2, callArgs);
            JS_FreeValue(ctx, callArgs[0]);
            JS_FreeValue(ctx, callArgs[1]);
            return ret;
        }

        JSValue CreateMainIpcObject(JSContext *ctx)
        {
            JSValue ipc = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, ipc, "on", JS_NewCFunction(ctx, JsMainIpcOn, "on", 2));
            JS_SetPropertyStr(ctx, ipc, "handle", JS_NewCFunction(ctx, JsMainIpcHandle, "handle", 2));
            JS_SetPropertyStr(ctx, ipc, "send", JS_NewCFunction(ctx, JsMainIpcSend, "send", 2));
            return ipc;
        }

        JSValue CreateUiIpcObjectImpl(JSContext *ctx)
        {
            JSValue ipc = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, ipc, "on", JS_NewCFunction(ctx, JsUiIpcOn, "on", 2));
            JS_SetPropertyStr(ctx, ipc, "send", JS_NewCFunction(ctx, JsUiIpcSend, "send", 2));
            JS_SetPropertyStr(ctx, ipc, "invoke", JS_NewCFunction(ctx, JsUiIpcInvoke, "invoke", 2));
            return ipc;
        }

        bool EnsureRuntime()
        {
            if (g_runtime && g_context)
            {
                return true;
            }

            g_runtime = JS_NewRuntime();
            if (!g_runtime)
            {
                Logging::Log(LogLevel::Error, L"Failed to create QuickJS runtime");
                return false;
            }

            g_context = JS_NewContext(g_runtime);
            if (!g_context)
            {
                Logging::Log(LogLevel::Error, L"Failed to create QuickJS context");
                JS_FreeRuntime(g_runtime);
                g_runtime = nullptr;
                return false;
            }

            JS_SetModuleLoaderFunc(g_runtime, novadesk::scripting::quickjs::ModuleNormalizeName, novadesk::scripting::quickjs::ModuleLoader, nullptr);
            JS_SetHostPromiseRejectionTracker(g_runtime, HostPromiseRejectionTracker, nullptr);
            novadesk::scripting::quickjs::SetModuleSystemDebug(false);
            RegisterConsoleBindings(g_context);
            g_eventCallbacks.clear();
            g_eventCallbacks.push_back(JS_UNDEFINED); // callback id 0 is invalid
            ClearCallbacks(g_mainIpcListeners);
            ClearCallbacks(g_uiIpcListeners);
            ClearChannelMap(g_mainIpcChannelListeners);
            ClearChannelMap(g_uiIpcChannelListeners);
            ClearHandlerMap(g_mainIpcHandlers);
            return true;
        }

        std::wstring ResolveEntryScript(const std::wstring &scriptPath)
        {
            if (scriptPath.empty())
            {
                const std::wstring defaultEntry = PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
                std::error_code ec;
                if (!std::filesystem::exists(std::filesystem::path(defaultEntry), ec))
                {
                    return L"";
                }
                return defaultEntry;
            }

            // Respect explicit custom script paths first.
            if (!PathUtils::IsPathRelative(scriptPath))
            {
                return PathUtils::NormalizePath(scriptPath);
            }

            // For relative custom paths, resolve from current working directory first.
            std::error_code ec;
            const std::filesystem::path cwd = std::filesystem::current_path(ec);
            if (!ec)
            {
                const std::wstring fromCwd = PathUtils::ResolvePath(scriptPath, cwd.wstring());
                if (std::filesystem::exists(std::filesystem::path(fromCwd), ec))
                {
                    return fromCwd;
                }
            }

            // Fallback: resolve relative to Widgets folder.
            return PathUtils::ResolvePath(scriptPath, PathUtils::GetWidgetsDir());
        }
    } // namespace

    void ClearAllTrayCommandCallbacks()
    {
        ClearAllTrayCommandCallbacksInternal();
    }

    void ClearAllTrayEventCallbacks()
    {
        ClearAllTrayEventCallbacksInternal();
    }

    void InitializeJavaScriptAPI(duk_context *ctx)
    {
        (void)ctx;
        EnsureRuntime();
    }

    bool LoadAndExecuteScript(duk_context *ctx, const std::wstring &scriptPath)
    {
        std::vector<std::wstring> list;
        list.push_back(scriptPath);
        return LoadAndExecuteScripts(ctx, list);
    }

    bool LoadAndExecuteScripts(duk_context *ctx, const std::vector<std::wstring> &scriptPaths)
    {
        (void)ctx;
        const bool isDefaultLoad = scriptPaths.empty();
        if (!g_staleScripts.empty())
        {
            DestroyAllWidgets();
            DestroyAllTrays();
            ResetRuntime();
        }
        if (!EnsureRuntime())
        {
            return false;
        }

        DestroyAllWidgets();
        DestroyAllTrays();

        std::vector<std::wstring> resolved;
        if (scriptPaths.empty())
        {
            const std::wstring defaultEntry = ResolveEntryScript(L"");
            if (!defaultEntry.empty())
            {
                resolved.push_back(defaultEntry);
            }
        }
        else
        {
            for (const auto &p : scriptPaths)
            {
                resolved.push_back(ResolveEntryScript(p));
            }
        }

        if (resolved.empty())
        {
            Logging::Log(LogLevel::Warn, L"No scripts to load.");
        }
        else
        {
            Logging::Log(LogLevel::Info, L"Loading %zu script(s).", resolved.size());
            for (const auto &p : resolved)
            {
                Logging::Log(LogLevel::Info, L"Script: %s", p.c_str());
            }
        }

        std::vector<std::wstring> loadedPaths;
        g_mainScriptPath = resolved.empty() ? L"" : resolved.front();

        for (size_t i = 1; i < g_eventCallbacks.size(); ++i)
        {
            JS_FreeValue(g_context, g_eventCallbacks[i]);
        }
        g_eventCallbacks.clear();
        g_eventCallbacks.push_back(JS_UNDEFINED);
        ClearCallbacks(g_mainIpcListeners);
        ClearCallbacks(g_uiIpcListeners);
        ClearChannelMap(g_mainIpcChannelListeners);
        ClearChannelMap(g_uiIpcChannelListeners);
        ClearHandlerMap(g_mainIpcHandlers);
        ClearWidgetEventListeners();
        ClearAllWidgetContextMenuCallbacks();
        ClearAllTrayCommandCallbacksInternal();
        ClearAllTrayEventCallbacksInternal();
        ClearAllTimers();

        const std::string widgetDirName = Utils::ToString(PathUtils::GetWidgetsDir());

        for (const auto &finalScriptPath : resolved)
        {
            Logging::Log(LogLevel::Info, L"Executing script: %s", finalScriptPath.c_str());
            if (!ExecuteScriptFile(finalScriptPath))
            {
                continue;
            }
            Logging::Log(LogLevel::Info, L"Script loaded: %s", finalScriptPath.c_str());
            loadedPaths.push_back(finalScriptPath);
        }

        g_loadedScriptPaths = loadedPaths;
        g_lastScriptPath = loadedPaths.empty() ? L"" : loadedPaths.front();
        g_staleScripts.clear();
        if (loadedPaths.empty())
        {
            if (isDefaultLoad)
            {
                Logging::Log(LogLevel::Info, L"No default Widgets\\index.js found. Starting without scripts.");
                return true;
            }
            Logging::Log(LogLevel::Error, L"No scripts loaded successfully.");
            return false;
        }
        return true;
    }

    std::wstring GetEntryScriptDir()
    {
        if (g_lastScriptPath.empty())
        {
            return PathUtils::GetWidgetsDir();
        }
        return PathUtils::GetParentDir(g_lastScriptPath);
    }

    std::wstring GetCurrentScriptDir()
    {
        return g_currentScriptDir;
    }

    std::wstring GetCurrentScriptPath()
    {
        return g_currentScriptPath;
    }

    void RegisterWidgetOwner(Widget *widget, const std::wstring &scriptPath)
    {
        if (!widget)
            return;
        g_widgetOwners[widget] = scriptPath;
    }

    void RegisterTrayOwner(int trayId, const std::wstring &scriptPath)
    {
        if (trayId <= 0)
            return;
        g_trayOwners[trayId] = scriptPath;
    }

    void UnregisterTrayOwner(int trayId)
    {
        g_trayOwners.erase(trayId);
    }

    void Reload()
    {
        if (!g_loadedScriptPaths.empty())
        {
            LoadAndExecuteScripts(nullptr, g_loadedScriptPaths);
        }
        else
        {
            LoadAndExecuteScript(nullptr, g_lastScriptPath);
        }
    }

    bool AddScript(const std::wstring &scriptPath)
    {
        const std::wstring resolved = ResolveEntryScript(scriptPath);
        for (const auto &p : g_loadedScriptPaths)
        {
            if (p == resolved)
                return true;
        }
        if (g_staleScripts.find(resolved) != g_staleScripts.end())
        {
            std::vector<std::wstring> next = g_loadedScriptPaths;
            next.push_back(resolved);
            return LoadAndExecuteScripts(nullptr, next);
        }
        if (!EnsureRuntime())
            return false;
        if (!ExecuteScriptFile(resolved))
            return false;
        g_loadedScriptPaths.push_back(resolved);
        return true;
    }

    bool RemoveScript(const std::wstring &scriptPath)
    {
        const std::wstring resolved = ResolveEntryScript(scriptPath);
        std::vector<std::wstring> next;
        for (const auto &p : g_loadedScriptPaths)
        {
            if (p != resolved)
            {
                next.push_back(p);
            }
        }
        if (next.size() == g_loadedScriptPaths.size())
            return false;
        DestroyWidgetsForScript(resolved);
        ClearTraysForScript(resolved);
        ClearTimersForScript(resolved);
        ClearIpcListenersForScript(g_mainIpcListeners, resolved);
        ClearIpcListenersForScript(g_uiIpcListeners, resolved);
        ClearIpcChannelListenersForScript(g_mainIpcChannelListeners, resolved);
        ClearIpcChannelListenersForScript(g_uiIpcChannelListeners, resolved);
        ClearIpcHandlersForScript(g_mainIpcHandlers, resolved);
        g_loadedScriptPaths = next;
        g_staleScripts.insert(resolved);
        g_scriptEvalRevisions.erase(resolved);
        return true;
    }

    bool RefreshScript(const std::wstring &scriptPath)
    {
        const std::wstring resolved = ResolveEntryScript(scriptPath);
        bool found = false;
        for (const auto &p : g_loadedScriptPaths)
        {
            if (p == resolved)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
        DestroyWidgetsForScript(resolved);
        ClearTraysForScript(resolved);
        ClearTimersForScript(resolved);
        ClearIpcListenersForScript(g_mainIpcListeners, resolved);
        ClearIpcListenersForScript(g_uiIpcListeners, resolved);
        ClearIpcChannelListenersForScript(g_mainIpcChannelListeners, resolved);
        ClearIpcChannelListenersForScript(g_uiIpcChannelListeners, resolved);
        ClearIpcHandlersForScript(g_mainIpcHandlers, resolved);
        if (!EnsureRuntime())
            return false;
        return ExecuteScriptFile(resolved);
    }

    std::vector<std::wstring> GetLoadedScripts()
    {
        return g_loadedScriptPaths;
    }

    void OnTimer(UINT_PTR id)
    {
        auto it = g_timers.find(id);
        if (it == g_timers.end())
            return;

        if (it->second.repeat)
        {
            TimerEntry &entry = it->second;
            ScriptExecutionScope scope(entry.owner);
            JSValue ret = JS_Call(g_context, entry.callback, JS_UNDEFINED, static_cast<int>(entry.args.size()), entry.args.data());
            if (JS_IsException(ret))
            {
                LogQuickJsException(g_context);
            }
            else
            {
                JS_FreeValue(g_context, ret);
            }
            return;
        }

        TimerEntry entry = std::move(it->second);
        if (g_messageWindow)
            KillTimer(g_messageWindow, id);
        g_timers.erase(it);

        ScriptExecutionScope scope(entry.owner);
        JSValue ret = JS_Call(g_context, entry.callback, JS_UNDEFINED, static_cast<int>(entry.args.size()), entry.args.data());
        if (JS_IsException(ret))
        {
            LogQuickJsException(g_context);
        }
        else
        {
            JS_FreeValue(g_context, ret);
        }
        JS_FreeValue(g_context, entry.callback);
        for (JSValue &a : entry.args)
            JS_FreeValue(g_context, a);
    }

    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NOVADESK_DISPATCH)
        {
            using DispatchFn = void (*)(void *);
            DispatchFn fn = reinterpret_cast<DispatchFn>(wParam);
            if (fn)
            {
                fn(reinterpret_cast<void *>(lParam));
            }
        }
    }

    void SetMessageWindow(HWND hWnd)
    {
        g_messageWindow = hWnd;
    }

    HWND GetMessageWindow()
    {
        return g_messageWindow;
    }

    void OnTrayCommand(int commandId)
    {
        auto it = g_trayCommandCallbacks.find(commandId);
        if (it == g_trayCommandCallbacks.end())
            return;
        std::wstring ownerScriptPath;
        auto ownerIt = g_trayOwners.find(it->second.trayId);
        if (ownerIt != g_trayOwners.end())
        {
            ownerScriptPath = ownerIt->second;
        }
        Logging::Log(
            LogLevel::Info,
            L"[novadesk] executing tray command callback id=%d tray=%d owner='%s'",
            commandId,
            it->second.trayId,
            ownerScriptPath.c_str());
        ScriptExecutionScope scope(ownerScriptPath);
        JSValue ret = JS_Call(g_context, it->second.callback, JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret))
        {
            LogQuickJsException(g_context);
        }
        else
        {
            JS_FreeValue(g_context, ret);
        }
    }

    void DispatchTrayEvent(int trayId, const std::string &eventName)
    {
        if (!g_context)
            return;
        auto it = g_trayEventCallbacks.find(trayId);
        if (it == g_trayEventCallbacks.end())
            return;
        auto evIt = it->second.find(eventName);
        if (evIt == it->second.end())
            return;
        std::wstring ownerScriptPath;
        auto ownerIt = g_trayOwners.find(trayId);
        if (ownerIt != g_trayOwners.end())
        {
            ownerScriptPath = ownerIt->second;
        }
        ScriptExecutionScope scope(ownerScriptPath);
        for (JSValue &cb : evIt->second)
        {
            JSValue ret = JS_Call(g_context, cb, JS_UNDEFINED, 0, nullptr);
            if (JS_IsException(ret))
            {
                LogQuickJsException(g_context);
            }
            else
            {
                JS_FreeValue(g_context, ret);
            }
        }
    }

    void OnWidgetContextCommand(const std::wstring &widgetId, int commandId)
    {
        auto wit = g_widgetContextMenuCallbacks.find(widgetId);
        if (wit == g_widgetContextMenuCallbacks.end())
            return;
        auto cit = wit->second.find(commandId);
        if (cit == wit->second.end())
            return;

        const std::wstring ownerScriptPath = GetWidgetOwnerScriptPathById(widgetId);
        ScriptExecutionScope scope(ownerScriptPath);
        JSValue ret = JS_Call(g_context, cit->second, JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret))
        {
            LogQuickJsException(g_context);
        }
        else
        {
            JS_FreeValue(g_context, ret);
        }
    }

    void TriggerWidgetEvent(Widget *widget, const char *eventName, const MouseEventData *data)
    {
        if (!widget || !eventName || !*eventName)
        {
            return;
        }

        auto widgetIt = g_widgetEventListeners.find(widget);
        if (widgetIt == g_widgetEventListeners.end())
        {
            return;
        }

        const std::string eventKey(eventName);
        auto eventIt = widgetIt->second.find(eventKey);
        if (eventIt == widgetIt->second.end())
        {
            return;
        }

        for (int callbackId : eventIt->second)
        {
            CallEventCallback(callbackId, widget, data);
        }
    }

    void CallEventCallback(int callbackId, Widget *widget, const MouseEventData *data)
    {
        if (!g_context || callbackId <= 0 || callbackId >= static_cast<int>(g_eventCallbacks.size()))
        {
            return;
        }

        JSValue callback = g_eventCallbacks[callbackId];
        if (JS_IsUndefined(callback) || JS_IsNull(callback))
        {
            return;
        }

        JSValue arg = JS_NewObject(g_context);
        JS_SetPropertyStr(g_context, arg, "__clientX", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__clientY", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__screenX", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__screenY", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__offsetX", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__offsetY", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__offsetXPercent", JS_NewInt32(g_context, 0));
        JS_SetPropertyStr(g_context, arg, "__offsetYPercent", JS_NewInt32(g_context, 0));
        if (widget)
        {
            JS_SetPropertyStr(g_context, arg, "widgetId",
                              JS_NewString(g_context, Utils::ToString(widget->GetOptions().id).c_str()));
        }

        int argc = 1;
        if (data)
        {
            JS_SetPropertyStr(g_context, arg, "__clientX", JS_NewInt32(g_context, data->clientX));
            JS_SetPropertyStr(g_context, arg, "__clientY", JS_NewInt32(g_context, data->clientY));
            JS_SetPropertyStr(g_context, arg, "__screenX", JS_NewInt32(g_context, data->screenX));
            JS_SetPropertyStr(g_context, arg, "__screenY", JS_NewInt32(g_context, data->screenY));
            JS_SetPropertyStr(g_context, arg, "__offsetX", JS_NewInt32(g_context, data->offsetX));
            JS_SetPropertyStr(g_context, arg, "__offsetY", JS_NewInt32(g_context, data->offsetY));
            JS_SetPropertyStr(g_context, arg, "__offsetXPercent", JS_NewInt32(g_context, data->offsetXPercent));
            JS_SetPropertyStr(g_context, arg, "__offsetYPercent", JS_NewInt32(g_context, data->offsetYPercent));
        }

        JSValue argv[1] = {arg};
        const std::wstring ownerScriptPath = GetWidgetOwnerScriptPath(widget);
        ScriptExecutionScope scope(ownerScriptPath);
        JSValue ret = JS_Call(g_context, callback, JS_UNDEFINED, argc, argv);
        JS_FreeValue(g_context, arg);
        if (JS_IsException(ret))
        {
            LogQuickJsException(g_context);
        }
        else
        {
            JS_FreeValue(g_context, ret);
        }
    }

    int RegisterEventCallback(JSContext *ctx, JSValueConst fn)
    {
        if (!ctx || !JS_IsFunction(ctx, fn))
        {
            return -1;
        }
        if (!EnsureRuntime() || !g_context)
        {
            return -1;
        }
        if (ctx != g_context)
        {
            return -1;
        }

        g_eventCallbacks.push_back(JS_DupValue(g_context, fn));
        return static_cast<int>(g_eventCallbacks.size() - 1);
    }

    bool RegisterWidgetEventListener(JSContext *ctx, Widget *widget, const std::string &eventName, JSValueConst fn)
    {
        if (!widget || eventName.empty())
        {
            return false;
        }

        const int callbackId = RegisterEventCallback(ctx, fn);
        if (callbackId < 0)
        {
            return false;
        }

        g_widgetEventListeners[widget][eventName].push_back(callbackId);
        return true;
    }

    bool RegisterWidgetContextMenuCallback(JSContext *ctx, const std::wstring &widgetId, int commandId, JSValueConst fn)
    {
        if (!ctx || ctx != g_context || widgetId.empty() || commandId <= 0 || !JS_IsFunction(ctx, fn))
        {
            return false;
        }

        auto &bucket = g_widgetContextMenuCallbacks[widgetId];
        auto it = bucket.find(commandId);
        if (it != bucket.end())
        {
            JS_FreeValue(ctx, it->second);
        }
        bucket[commandId] = JS_DupValue(ctx, fn);
        return true;
    }

    void ClearWidgetContextMenuCallbacks(const std::wstring &widgetId)
    {
        auto it = g_widgetContextMenuCallbacks.find(widgetId);
        if (it == g_widgetContextMenuCallbacks.end())
            return;
        for (auto &kv : it->second)
        {
            JS_FreeValue(g_context, kv.second);
        }
        g_widgetContextMenuCallbacks.erase(it);
    }

    bool RegisterTrayCommandCallback(JSContext *ctx, int trayId, int commandId, JSValueConst fn)
    {
        if (!ctx || ctx != g_context || commandId <= 0 || !JS_IsFunction(ctx, fn))
        {
            return false;
        }
        auto it = g_trayCommandCallbacks.find(commandId);
        if (it != g_trayCommandCallbacks.end())
        {
            JS_FreeValue(ctx, it->second.callback);
        }
        TrayCommandCallback entry{};
        entry.trayId = trayId;
        entry.callback = JS_DupValue(ctx, fn);
        g_trayCommandCallbacks[commandId] = entry;
        return true;
    }

    void ClearTrayCommandCallbacks(int trayId)
    {
        if (!g_context)
            return;
        std::vector<int> toErase;
        for (auto &kv : g_trayCommandCallbacks)
        {
            if (kv.second.trayId == trayId)
            {
                JS_FreeValue(g_context, kv.second.callback);
                toErase.push_back(kv.first);
            }
        }
        for (int id : toErase)
        {
            g_trayCommandCallbacks.erase(id);
        }
    }

    bool RegisterTrayEventCallback(JSContext *ctx, int trayId, const std::string &eventName, JSValueConst fn)
    {
        if (!ctx || ctx != g_context || eventName.empty() || !JS_IsFunction(ctx, fn))
        {
            return false;
        }
        g_trayEventCallbacks[trayId][eventName].push_back(JS_DupValue(ctx, fn));
        return true;
    }

    void ClearTrayEventCallbacks(int trayId)
    {
        if (!g_context)
            return;
        auto it = g_trayEventCallbacks.find(trayId);
        if (it == g_trayEventCallbacks.end())
            return;
        for (auto &kv : it->second)
        {
            for (JSValue &cb : kv.second)
            {
                JS_FreeValue(g_context, cb);
            }
        }
        g_trayEventCallbacks.erase(it);
    }

    bool ExecuteWidgetScript(Widget *widget)
    {
        if (!widget || !g_context)
        {
            return false;
        }
        const std::wstring &scriptPath = widget->GetOptions().scriptPath;
        if (scriptPath.empty())
        {
            return false;
        }
        return novadesk::scripting::quickjs::ExecuteWidgetUiScript(g_context, widget, scriptPath);
    }

    JSValue CreateUiIpcObject(JSContext *ctx)
    {
        return CreateUiIpcObjectImpl(ctx);
    }
} // namespace JSEngine
