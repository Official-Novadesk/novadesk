#include "WidgetWindowEventBindings.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "../parser/PropertyParser.h"

extern std::vector<Widget *> widgets;

namespace novadesk::scripting::quickjs
{
    namespace
    {
        JSClassID g_widgetWindowClassId = 0;
        int g_nextContextMenuId = 1;

        JSValue ThrowTypeError(JSContext *ctx, const char *method, const char *usage)
        {
            return JS_ThrowTypeError(ctx, "%s: %s", method, usage);
        }

        Widget *GetWidget(JSContext *ctx, JSValueConst thisVal)
        {
            (void)ctx;
            Widget *widget = static_cast<Widget *>(JS_GetOpaque(thisVal, g_widgetWindowClassId));
            return Widget::IsValid(widget) ? widget : nullptr;
        }

        Widget *GetWidgetRaw(JSValueConst thisVal)
        {
            return static_cast<Widget *>(JS_GetOpaque(thisVal, g_widgetWindowClassId));
        }

        bool DestroyWidgetInstance(Widget *widget, bool skipCloseEvent)
        {
            if (!widget)
                return false;
            auto it = std::find(widgets.begin(), widgets.end(), widget);
            if (it == widgets.end())
                return false;
            widget->SetSkipCloseEventOnDestroy(skipCloseEvent);
            widgets.erase(it);
            delete widget;
            return true;
        }

        JSValue JsWidgetWindowOn(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            if (argc < 2 || !JS_IsFunction(ctx, argv[1]))
            {
                return ThrowTypeError(ctx, "on", "expected (eventName, callback)");
            }

            const char *eventName = JS_ToCString(ctx, argv[0]);
            if (!eventName || !*eventName)
            {
                if (eventName)
                    JS_FreeCString(ctx, eventName);
                return ThrowTypeError(ctx, "on", "eventName must be non-empty string");
            }

            const std::string event(eventName);
            JS_FreeCString(ctx, eventName);

            if (!JSEngine::RegisterWidgetEventListener(ctx, widget, event, argv[1]))
            {
                return JS_ThrowInternalError(ctx, "failed to register widget event listener");
            }

            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowSetProperties(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsObject(argv[0]))
            {
                return ThrowTypeError(ctx, "setProperties", "expected options object");
            }

            parser::WidgetWindowOptions parsed;
            parser::ParseWidgetWindowOptions(ctx, argv[0], parsed);

            if (parsed.hasX || parsed.hasY || parsed.hasWidth || parsed.hasHeight)
            {
                const int x = parsed.hasX ? parsed.x : CW_USEDEFAULT;
                const int y = parsed.hasY ? parsed.y : CW_USEDEFAULT;
                const int w = parsed.hasWidth ? parsed.width : -1;
                const int h = parsed.hasHeight ? parsed.height : -1;
                widget->SetWindowPosition(x, y, w, h);
            }

            if (parsed.hasBackgroundColor)
                widget->SetBackgroundColor(parsed.backgroundColor);
            if (parsed.hasWindowOpacity)
                widget->SetWindowOpacity(parsed.windowOpacity);
            if (parsed.hasDraggable)
                widget->SetDraggable(parsed.draggable);
            if (parsed.hasClickThrough)
                widget->SetClickThrough(parsed.clickThrough);
            if (parsed.hasKeepOnScreen)
                widget->SetKeepOnScreen(parsed.keepOnScreen);
            if (parsed.hasSnapEdges)
                widget->SetSnapEdges(parsed.snapEdges);
            if (parsed.hasShowInToolbar)
                widget->SetShowInToolbar(parsed.showInToolbar);
            if (parsed.hasToolbarTitle)
                widget->SetToolbarTitle(parsed.toolbarTitle);
            if (parsed.hasToolbarIcon)
            {
                std::wstring iconPath = parsed.toolbarIcon;
                if (!iconPath.empty())
                {
                    if (PathUtils::IsPathRelative(iconPath))
                    {
                        std::wstring base = JSEngine::GetCurrentScriptDir();
                        if (base.empty())
                            base = JSEngine::GetEntryScriptDir();
                        if (!base.empty())
                            iconPath = PathUtils::ResolvePath(iconPath, base);
                        else
                            iconPath = PathUtils::ResolvePath(iconPath, PathUtils::GetWidgetsDir());
                    }
                    else
                    {
                        iconPath = PathUtils::NormalizePath(iconPath);
                    }
                }
                widget->SetToolbarIcon(iconPath);
            }
            if (parsed.hasShow)
            {
                if (parsed.show)
                    widget->Show();
                else
                    widget->Hide();
            }
            if (parsed.hasZPos)
                widget->ChangeZPos(static_cast<ZPOSITION>(parsed.zPos));

            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetProperties(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            const WidgetOptions &o = widget->GetOptions();
            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "id", JS_NewString(ctx, Utils::ToString(o.id).c_str()));
            JS_SetPropertyStr(ctx, out, "x", JS_NewInt32(ctx, o.x));
            JS_SetPropertyStr(ctx, out, "y", JS_NewInt32(ctx, o.y));
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, o.width));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, o.height));
            JS_SetPropertyStr(ctx, out, "draggable", JS_NewBool(ctx, o.draggable ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "clickThrough", JS_NewBool(ctx, o.clickThrough ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "keepOnScreen", JS_NewBool(ctx, o.keepOnScreen ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "snapEdges", JS_NewBool(ctx, o.snapEdges ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "showInToolbar", JS_NewBool(ctx, o.showInToolbar ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "toolbarIcon", JS_NewString(ctx, Utils::ToString(o.toolbarIcon).c_str()));
            JS_SetPropertyStr(ctx, out, "toolbarTitle", JS_NewString(ctx, Utils::ToString(o.toolbarTitle).c_str()));
            JS_SetPropertyStr(ctx, out, "show", JS_NewBool(ctx, IsWindowVisible(widget->GetWindow()) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "windowOpacity", JS_NewInt32(ctx, static_cast<int>(o.windowOpacity)));
            JS_SetPropertyStr(ctx, out, "backgroundColor", JS_NewString(ctx, Utils::ToString(o.backgroundColor).c_str()));
            JS_SetPropertyStr(ctx, out, "zPos", JS_NewInt32(ctx, static_cast<int>(o.zPos)));
            JS_SetPropertyStr(ctx, out, "script", JS_NewString(ctx, Utils::ToString(o.scriptPath).c_str()));
            return out;
        }

        JSValue JsWidgetWindowClose(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            DestroyWidgetInstance(widget, false);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowDestroy(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            DestroyWidgetInstance(widget, true);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowShow(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Show();
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowHide(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Hide();
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowIsFocused(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            HWND hWnd = widget->GetWindow();
            return JS_NewBool(ctx, (hWnd && GetFocus() == hWnd) ? 1 : 0);
        }

        JSValue JsWidgetWindowIsVisible(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            HWND hWnd = widget->GetWindow();
            return JS_NewBool(ctx, (hWnd && IsWindowVisible(hWnd)) ? 1 : 0);
        }

        JSValue JsWidgetWindowIsDestroyed(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *raw = GetWidgetRaw(thisVal);
            const bool destroyed = !(raw && Widget::IsValid(raw));
            return JS_NewBool(ctx, destroyed ? 1 : 0);
        }

        JSValue JsWidgetWindowSetBounds(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsObject(argv[0]))
            {
                return ThrowTypeError(ctx, "setBounds", "expected bounds object");
            }

            int x = CW_USEDEFAULT;
            int y = CW_USEDEFAULT;
            int w = -1;
            int h = -1;

            int32_t v = 0;
            JSValue xv = JS_GetPropertyStr(ctx, argv[0], "x");
            if (!JS_IsUndefined(xv) && !JS_IsNull(xv) && JS_ToInt32(ctx, &v, xv) == 0) x = static_cast<int>(v);
            JS_FreeValue(ctx, xv);

            JSValue yv = JS_GetPropertyStr(ctx, argv[0], "y");
            if (!JS_IsUndefined(yv) && !JS_IsNull(yv) && JS_ToInt32(ctx, &v, yv) == 0) y = static_cast<int>(v);
            JS_FreeValue(ctx, yv);

            JSValue wv = JS_GetPropertyStr(ctx, argv[0], "width");
            if (!JS_IsUndefined(wv) && !JS_IsNull(wv) && JS_ToInt32(ctx, &v, wv) == 0) w = static_cast<int>(v);
            JS_FreeValue(ctx, wv);

            JSValue hv = JS_GetPropertyStr(ctx, argv[0], "height");
            if (!JS_IsUndefined(hv) && !JS_IsNull(hv) && JS_ToInt32(ctx, &v, hv) == 0) h = static_cast<int>(v);
            JS_FreeValue(ctx, hv);

            widget->SetWindowPosition(x, y, w, h);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetBounds(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            HWND hWnd = widget->GetWindow();
            if (!hWnd)
                return JS_NULL;

            RECT rc{};
            if (!GetWindowRect(hWnd, &rc))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "x", JS_NewInt32(ctx, rc.left));
            JS_SetPropertyStr(ctx, out, "y", JS_NewInt32(ctx, rc.top));
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, rc.right - rc.left));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, rc.bottom - rc.top));
            return out;
        }

        JSValue JsWidgetWindowSetSize(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 2)
            {
                return ThrowTypeError(ctx, "setSize", "expected (width, height)");
            }

            int32_t w = 0;
            int32_t h = 0;
            if (JS_ToInt32(ctx, &w, argv[0]) != 0 || JS_ToInt32(ctx, &h, argv[1]) != 0)
            {
                return ThrowTypeError(ctx, "setSize", "width/height must be numbers");
            }

            widget->SetWindowPosition(CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(w), static_cast<int>(h));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetSize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            HWND hWnd = widget->GetWindow();
            if (!hWnd)
                return JS_NULL;

            RECT rc{};
            if (!GetWindowRect(hWnd, &rc))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, rc.right - rc.left));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, rc.bottom - rc.top));
            return out;
        }

        JSValue JsWidgetWindowGetBackgroundColor(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewString(ctx, Utils::ToString(widget->GetOptions().backgroundColor).c_str());
        }

        JSValue JsWidgetWindowSetBackgroundColor(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setBackgroundColor", "expected color string");
            }
            const char *colorUtf8 = JS_ToCString(ctx, argv[0]);
            if (!colorUtf8)
                return JS_EXCEPTION;
            std::wstring color = Utils::ToWString(colorUtf8);
            JS_FreeCString(ctx, colorUtf8);
            widget->SetBackgroundColor(color);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowSetOpacity(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setOpacity", "expected value");
            }

            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, argv[0]) != 0)
            {
                return ThrowTypeError(ctx, "setOpacity", "value must be number");
            }

            int opacity = 255;
            if (d <= 1.0)
            {
                opacity = static_cast<int>(d * 255.0);
            }
            else if (d <= 100.0)
            {
                opacity = static_cast<int>((d / 100.0) * 255.0);
            }
            else
            {
                opacity = static_cast<int>(d);
            }

            opacity = std::clamp(opacity, 0, 255);
            widget->SetWindowOpacity(static_cast<BYTE>(opacity));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowRefresh(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Refresh();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowSetFocus(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->SetFocus();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowUnFocus(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->UnFocus();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowMinimize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Minimize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowUnMinimize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->UnMinimize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowGetHandle(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget->GetWindow())));
        }

        JSValue JsWidgetWindowGetInternalPointer(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget)));
        }

        JSValue JsWidgetWindowGetTitle(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewString(ctx, "");
            return JS_NewString(ctx, Utils::ToString(widget->GetTitle()).c_str());
        }

        bool ParseContextMenuItems(JSContext *ctx, JSValueConst arr, const std::wstring &widgetId, std::vector<MenuItem> &out)
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
                        item.id = 2000 + g_nextContextMenuId++;
                        JSEngine::RegisterWidgetContextMenuCallback(ctx, widgetId, item.id, actionV);
                    }
                    JS_FreeValue(ctx, actionV);

                    JSValue childV = JS_GetPropertyStr(ctx, itemV, "items");
                    if (JS_IsArray(childV))
                        ParseContextMenuItems(ctx, childV, widgetId, item.children);
                    JS_FreeValue(ctx, childV);
                }

                out.push_back(std::move(item));
                JS_FreeValue(ctx, itemV);
            }

            return true;
        }

        JSValue JsWidgetWindowSetContextMenu(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsArray(argv[0]))
                return ThrowTypeError(ctx, "setContextMenu", "expected items array");

            const std::wstring widgetId = widget->GetOptions().id;
            JSEngine::ClearWidgetContextMenuCallbacks(widgetId);

            std::vector<MenuItem> menu;
            if (!ParseContextMenuItems(ctx, argv[0], widgetId, menu))
            {
                return JS_ThrowTypeError(ctx, "setContextMenu: invalid items");
            }

            widget->SetContextMenu(menu);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowClearContextMenu(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->ClearContextMenu();
            JSEngine::ClearWidgetContextMenuCallbacks(widget->GetOptions().id);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowDisableContextMenu(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            bool disable = true;
            if (argc > 0)
            {
                int b = JS_ToBool(ctx, argv[0]);
                if (b >= 0)
                    disable = (b != 0);
            }
            widget->SetContextMenuDisabled(disable);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowShowDefaultContextMenuItems(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc > 0)
            {
                int b = JS_ToBool(ctx, argv[0]);
                if (b >= 0)
                    widget->SetShowDefaultContextMenuItems(b != 0);
            }
            return JS_DupValue(ctx, thisVal);
        }

        const JSCFunctionListEntry kWidgetWindowEventFuncs[] = {
            JS_CFUNC_DEF("setProperties", 1, JsWidgetWindowSetProperties),
            JS_CFUNC_DEF("getProperties", 0, JsWidgetWindowGetProperties),
            JS_CFUNC_DEF("close", 0, JsWidgetWindowClose),
            JS_CFUNC_DEF("destroy", 0, JsWidgetWindowDestroy),
            JS_CFUNC_DEF("show", 0, JsWidgetWindowShow),
            JS_CFUNC_DEF("hide", 0, JsWidgetWindowHide),
            JS_CFUNC_DEF("isFocused", 0, JsWidgetWindowIsFocused),
            JS_CFUNC_DEF("isVisible", 0, JsWidgetWindowIsVisible),
            JS_CFUNC_DEF("isDestroyed", 0, JsWidgetWindowIsDestroyed),
            JS_CFUNC_DEF("setBounds", 1, JsWidgetWindowSetBounds),
            JS_CFUNC_DEF("getBounds", 0, JsWidgetWindowGetBounds),
            JS_CFUNC_DEF("setSize", 2, JsWidgetWindowSetSize),
            JS_CFUNC_DEF("getSize", 0, JsWidgetWindowGetSize),
            JS_CFUNC_DEF("getBackgroundColor", 0, JsWidgetWindowGetBackgroundColor),
            JS_CFUNC_DEF("setBackgroundColor", 1, JsWidgetWindowSetBackgroundColor),
            JS_CFUNC_DEF("setOpacity", 1, JsWidgetWindowSetOpacity),
            JS_CFUNC_DEF("refresh", 0, JsWidgetWindowRefresh),
            JS_CFUNC_DEF("setFocus", 0, JsWidgetWindowSetFocus),
            JS_CFUNC_DEF("unFocus", 0, JsWidgetWindowUnFocus),
            JS_CFUNC_DEF("minimize", 0, JsWidgetWindowMinimize),
            JS_CFUNC_DEF("unMinimize", 0, JsWidgetWindowUnMinimize),
            JS_CFUNC_DEF("getHandle", 0, JsWidgetWindowGetHandle),
            JS_CFUNC_DEF("getInternalPointer", 0, JsWidgetWindowGetInternalPointer),
            JS_CFUNC_DEF("getTitle", 0, JsWidgetWindowGetTitle),
            JS_CFUNC_DEF("on", 2, JsWidgetWindowOn),
            JS_CFUNC_DEF("setContextMenu", 1, JsWidgetWindowSetContextMenu),
            JS_CFUNC_DEF("clearContextMenu", 0, JsWidgetWindowClearContextMenu),
            JS_CFUNC_DEF("disableContextMenu", 1, JsWidgetWindowDisableContextMenu),
            JS_CFUNC_DEF("showDefaultContextMenuItems", 1, JsWidgetWindowShowDefaultContextMenuItems),
        };
    } // namespace

    void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId)
    {
        g_widgetWindowClassId = widgetWindowClassId;
    }

    void AttachWidgetWindowEventMethods(JSContext *ctx, JSValue proto)
    {
        JS_SetPropertyFunctionList(
            ctx,
            proto,
            kWidgetWindowEventFuncs,
            sizeof(kWidgetWindowEventFuncs) / sizeof(kWidgetWindowEventFuncs[0]));
    }

    void InvokeWidgetContextMenuCallback(const std::wstring &, int)
    {
        // Routed via JSEngine::OnWidgetContextCommand.
    }
} // namespace novadesk::scripting::quickjs
