/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "Widget.h"
#include "Settings.h"
#include "Logging.h"
#include "ColorUtil.h"
#include "Utils.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <Windows.h>

extern std::vector<Widget*> widgets;

namespace JSApi {

    // JS API: novadesk.log(...)
    duk_ret_t js_log(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Info, L"%s", msg.c_str());
        return 0;
    }

    // JS API: novadesk.error(...)
    duk_ret_t js_error(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Error, L"%s", msg.c_str());
        return 0;
    }

    // JS API: novadesk.debug(...)
    duk_ret_t js_debug(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Debug, L"%s", msg.c_str());
        return 0;
    }

    // JS API: new widgetWindow(options)
    duk_ret_t js_create_widget_window(duk_context* ctx) {
        if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        WidgetOptions options;
        options.x = CW_USEDEFAULT;
        options.y = CW_USEDEFAULT;
        options.width = 400;
        options.height = 300;
        options.backgroundColor = L"rgba(255,255,255,255)";
        options.zPos = ZPOSITION_NORMAL;
        options.bgAlpha = 255;
        options.windowOpacity = 255;
        options.color = RGB(255, 255, 255);
        options.draggable = false;
        options.clickThrough = false;
        options.keepOnScreen = false;
        options.snapEdges = false;

        if (duk_get_prop_string(ctx, 0, "id")) options.id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);

        // Load saved setttings if ID exists
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        // Override with explicit JS options if provided (users explicitly passing options usually want them to take effect, 
        // BUT if it's a reload/restart, we usually want saved settings to persist for things like position.
        // However, if the user changes the script to width: 500, they might expect it to update?
        // Usually persistence overrides hardcoded defaults, but maybe not hardcoded changes?
        // Rainmeter standard: Skin options are defaults, [Variables] or saved state overrides?
        // Let's assume saved settings override defaults, but explicit JS arguments override saved settings IF we want strict control?
        // No, typically restoration implies "restore last state". 
        // If I change the code, I might want to force a change.
        // Compromise: Load settings, then parse JS options again? No, that's wasteful.
        // Actually, typical flow: 
        // 1. Defaults.
        // 2. JS Options (Code).
        // 3. Saved Settings (User runtime changes).
        // Saved settings should generally win for Position/Size/State.
        // But maybe not for Color if I changed the color in code?
        // Hard to distinguish code change vs old saved state.
        // Let's stick to: JS Options are defaults/initial values. Saved State overrides them.
        
        // So:
        // 1. Parse JS Options into 'options'.
        // 2. LoadWidget(id, options) -> This updates 'options' with saved values.
        
        // Moving parsing block BEFORE LoadWidget.

        if (duk_get_prop_string(ctx, 0, "width")) options.width = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) options.height = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "backgroundcolor")) {
            options.backgroundColor = Utils::ToWString(duk_get_string(ctx, -1));
            ColorUtil::ParseRGBA(options.backgroundColor, options.color, options.bgAlpha);
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "opacity")) {
            if (duk_is_string(ctx, -1)) {
                std::string s = duk_get_string(ctx, -1);
                if (s.back() == '%') {
                    int pct = std::stoi(s.substr(0, s.length() - 1));
                    options.windowOpacity = (BYTE)((pct / 100.0f) * 255);
                } else {
                    options.windowOpacity = (BYTE)(duk_get_number(ctx, -1) * 255);
                }
            } else {
                options.windowOpacity = (BYTE)(duk_get_number(ctx, -1) * 255);
            }
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "zpos")) {
            std::string zPosStr = duk_get_string(ctx, -1);
            if (zPosStr == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
            else if (zPosStr == "ontop") options.zPos = ZPOSITION_ONTOP;
            else if (zPosStr == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
            else if (zPosStr == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
            else if (zPosStr == "normal") options.zPos = ZPOSITION_NORMAL;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "draggable")) options.draggable = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "clickthrough")) options.clickThrough = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "keeponscreen")) options.keepOnScreen = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "snapedges")) options.snapEdges = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        
        // Finally, load settings to override with saved state
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        Widget* widget = new Widget(options);
        if (widget->Create()) {
            widget->Show();
            widgets.push_back(widget);
            Logging::Log(LogLevel::Info, L"Widget created and shown. HWND: 0x%p", widget->GetWindow());
            
            // Create JavaScript object to represent the widget
            duk_push_object(ctx);
            
            // Store widget pointer
            duk_push_pointer(ctx, widget);
            duk_put_prop_string(ctx, -2, "\xFF" "widgetPtr");
            
            // Add methods to the widget object
            duk_push_c_function(ctx, js_widget_add_image, 1);
            duk_put_prop_string(ctx, -2, "addImage");
            
            duk_push_c_function(ctx, js_widget_add_text, 1);
            duk_put_prop_string(ctx, -2, "addText");
            
            duk_push_c_function(ctx, js_widget_update_image, 2);
            duk_put_prop_string(ctx, -2, "updateImage");
            
            duk_push_c_function(ctx, js_widget_update_text, 2);
            duk_put_prop_string(ctx, -2, "updateText");
            
            duk_push_c_function(ctx, js_widget_remove_content, 1);
            duk_put_prop_string(ctx, -2, "removeContent");
            
            duk_push_c_function(ctx, js_widget_clear_content, 0);
            duk_put_prop_string(ctx, -2, "clearContent");
            
            return 1; // Return the object
        }
        else {
            Logging::Log(LogLevel::Error, L"Failed to create widget.");
            delete widget;
        }

        return 0;
    }

    void ParseElementOptions(duk_context* ctx, Element* element) {
        if (!element) return;
        
        // Helper to get string prop
        auto getStr = [&](const char* key) -> std::wstring {
            if (duk_get_prop_string(ctx, 0, key)) {
                if (duk_is_string(ctx, -1)) {
                    std::wstring val = Utils::ToWString(duk_get_string(ctx, -1));
                    duk_pop(ctx);
                    return val;
                }
            }
            duk_pop(ctx); // pop undefined or non-string
            return L"";
        };

        std::wstring val;
        if (!(val = getStr("onleftmouseup")).empty()) element->m_OnLeftMouseUp = val;
        if (!(val = getStr("onleftmousedown")).empty()) element->m_OnLeftMouseDown = val;
        if (!(val = getStr("onleftdoubleclick")).empty()) element->m_OnLeftDoubleClick = val;
        if (!(val = getStr("onrightmouseup")).empty()) element->m_OnRightMouseUp = val;
        if (!(val = getStr("onrightmousedown")).empty()) element->m_OnRightMouseDown = val;
        if (!(val = getStr("onrightdoubleclick")).empty()) element->m_OnRightDoubleClick = val;
        if (!(val = getStr("onmiddlemouseup")).empty()) element->m_OnMiddleMouseUp = val;
        if (!(val = getStr("onmiddlemousedown")).empty()) element->m_OnMiddleMouseDown = val;
        if (!(val = getStr("onmiddledoubleclick")).empty()) element->m_OnMiddleDoubleClick = val;
        if (!(val = getStr("onx1mouseup")).empty()) element->m_OnX1MouseUp = val;
        if (!(val = getStr("onx1mousedown")).empty()) element->m_OnX1MouseDown = val;
        if (!(val = getStr("onx1doubleclick")).empty()) element->m_OnX1DoubleClick = val;
        if (!(val = getStr("onx2mouseup")).empty()) element->m_OnX2MouseUp = val;
        if (!(val = getStr("onx2mousedown")).empty()) element->m_OnX2MouseDown = val;
        if (!(val = getStr("onx2doubleclick")).empty()) element->m_OnX2DoubleClick = val;
        if (!(val = getStr("onscrollup")).empty()) element->m_OnScrollUp = val;
        if (!(val = getStr("onscrolldown")).empty()) element->m_OnScrollDown = val;
        if (!(val = getStr("onscrollleft")).empty()) element->m_OnScrollLeft = val;
        if (!(val = getStr("onscrollright")).empty()) element->m_OnScrollRight = val;
        if (!(val = getStr("onmouseover")).empty()) element->m_OnMouseOver = val;
        if (!(val = getStr("onmouseleave")).empty()) element->m_OnMouseLeave = val;
        
    }

    duk_ret_t js_widget_add_image(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        // Defaults
        std::wstring id = L"", path = L"";
        int x = 0, y = 0, w = 100, h = 100;

        if (duk_get_prop_string(ctx, 0, "id")) id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "path")) path = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "x")) x = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "y")) y = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "width")) w = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) h = duk_get_int(ctx, -1);
        duk_pop(ctx);

        widget->AddImage(id, x, y, w, h, path);
        
        // Parse Mouse Actions
        Element* el = widget->FindElementById(id);
        ParseElementOptions(ctx, el);

        // Return 'this' for chaining
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_add_text(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        // Defaults
        std::wstring id = L"", text = L"", fontFamily = L"Arial";
        int x = 0, y = 0, w = 100, h = 30;
        int fontSize = 12;
        COLORREF color = RGB(0, 0, 0);
        BYTE alpha = 255;
        bool bold = false, italic = false;
        if (duk_get_prop_string(ctx, 0, "id")) id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "text")) text = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontfamily")) fontFamily = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "x")) x = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "y")) y = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "width")) w = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) h = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontsize")) fontSize = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "color")) {
            std::wstring colorStr = Utils::ToWString(duk_get_string(ctx, -1));
            ColorUtil::ParseRGBA(colorStr, color, alpha);
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontweight")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "bold") bold = true;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontstyle")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "italic") italic = true;
        }
        duk_pop(ctx);

        Alignment align = ALIGN_LEFT_TOP;
        if (duk_get_prop_string(ctx, 0, "align")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "left" || s == "lefttop") align = ALIGN_LEFT_TOP;
            else if (s == "center" || s == "centertop") align = ALIGN_CENTER_TOP;
            else if (s == "right" || s == "righttop") align = ALIGN_RIGHT_TOP;
            else if (s == "leftcenter") align = ALIGN_LEFT_CENTER;
            else if (s == "centercenter") align = ALIGN_CENTER_CENTER;
            else if (s == "rightcenter") align = ALIGN_RIGHT_CENTER;
            else if (s == "leftbottom") align = ALIGN_LEFT_BOTTOM;
            else if (s == "centerbottom") align = ALIGN_CENTER_BOTTOM;
            else if (s == "rightbottom") align = ALIGN_RIGHT_BOTTOM;
        }
        duk_pop(ctx);

        widget->AddText(id, x, y, w, h, text, fontFamily, fontSize, color, alpha, bold, italic, align);
        
        // Parse Mouse Actions
        Element* el = widget->FindElementById(id);
        ParseElementOptions(ctx, el);

        // Return 'this' for chaining
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_update_image(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring path = Utils::ToWString(duk_get_string(ctx, 1));

        bool result = widget->UpdateImage(id, path);
        duk_push_boolean(ctx, result);
        return 1;
    }

    duk_ret_t js_widget_update_text(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        std::wstring text = Utils::ToWString(duk_get_string(ctx, 1));

        bool result = widget->UpdateText(id, text);
        duk_push_boolean(ctx, result);
        return 1;
    }

    duk_ret_t js_widget_remove_content(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        bool result = widget->RemoveContent(id);
        duk_push_boolean(ctx, result);
        return 1;
    }

    // Global context pointer for callbacks
    static duk_context* s_JsContext = nullptr;

    duk_ret_t js_widget_clear_content(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        widget->ClearContent();
        
        // Return 'this' for chaining
        duk_push_this(ctx);
        return 1;
    }

    void ExecuteScript(const std::wstring& script) {
        if (!s_JsContext) {
            Logging::Log(LogLevel::Error, L"ExecuteScript: Context not set");
            return;
        }

        std::string scriptStr = Utils::ToString(script);
        if (duk_peval_string(s_JsContext, scriptStr.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"JS Error: %S", duk_safe_to_string(s_JsContext, -1));
        }
        duk_pop(s_JsContext); // pop result/error
    }

    void InitializeJavaScriptAPI(duk_context* ctx) {
        s_JsContext = ctx; // Store context for callbacks

        // Register novadesk object and methods
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_log, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "log");
        duk_push_c_function(ctx, js_error, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "error");
        duk_push_c_function(ctx, js_debug, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "debug");
        duk_put_global_string(ctx, "novadesk");

        // Register widgetWindow constructor
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    bool LoadAndExecuteScript(duk_context* ctx) {
        // Get executable path to find index.js
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring exePath = path;
        size_t lastBackslash = exePath.find_last_of(L"\\");
        std::wstring scriptPath = exePath.substr(0, lastBackslash + 1) + L"Widgets\\index.js";

        Logging::Log(LogLevel::Info, L"Loading script from: %s", scriptPath.c_str());

        std::ifstream t(scriptPath);
        if (!t.is_open()) {
            Logging::Log(LogLevel::Error, L"Failed to open index.js at %s", scriptPath.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << t.rdbuf();
        std::string content = buffer.str();
        Logging::Log(LogLevel::Info, L"Script loaded, size: %zu", content.size());

        if (duk_peval_string(ctx, content.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
            duk_pop(ctx);
            return false;
        }

        Logging::Log(LogLevel::Info, L"Script execution successful. Calling onAppReady()...");

        // Call onAppReady if defined
        duk_get_global_string(ctx, "onAppReady");
        if (duk_is_function(ctx, -1)) {
            if (duk_pcall(ctx, 0) != 0) {
                Logging::Log(LogLevel::Error, L"onAppReady failed: %S", duk_safe_to_string(ctx, -1));
                duk_pop(ctx);
                return false;
            }
        }
        else {
            Logging::Log(LogLevel::Info, L"onAppReady not defined.");
        }
        duk_pop(ctx);
        duk_pop(ctx);

        return true;
    }

    void ReloadScripts(duk_context* ctx) {
        Logging::Log(LogLevel::Info, L"Reloading scripts...");

        // Destroy all existing widgets
        for (auto w : widgets) {
            delete w;
        }
        widgets.clear();

        // Reload and execute script
        LoadAndExecuteScript(ctx);
    }

}
