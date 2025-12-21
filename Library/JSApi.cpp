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
#include <map>
#include <fstream>
#include <sstream>
#include <Windows.h>

extern std::vector<Widget*> widgets;

namespace JSApi {

    // Timer logic
    struct Timer {
        bool repeating;
        int callbackIndex; // Key in the JS-side callback map
    };

    static HWND s_MessageWindow = nullptr;
    static std::map<UINT_PTR, Timer> s_Timers;
    static UINT_PTR s_NextTimerId = 1000;
    static const UINT WM_NOVADESK_ACTION = WM_USER + 100;

    // Global context pointer for callbacks
    static duk_context* s_JsContext = nullptr;

    // Helper to read file content
    std::string ReadFileContent(const std::wstring& path) {
        std::ifstream t(path);
        if (!t.is_open()) return "";
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    // Helper to get Widgets directory
    std::wstring GetWidgetsDir() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring exePath = path;
        size_t lastBackslash = exePath.find_last_of(L"\\");
        return exePath.substr(0, lastBackslash + 1) + L"Widgets\\";
    }

    // Helper to call a JS function by its index in the "pending handlers" object
    void CallStoredCallback(int id) {
        if (!s_JsContext) return;

        // Get the novadesk.__timers object
        duk_get_global_string(s_JsContext, "novadesk");
        duk_get_prop_string(s_JsContext, -1, "__timers");
        
        // Push the ID
        duk_push_int(s_JsContext, id);
        if (duk_get_prop(s_JsContext, -2)) {
            if (duk_is_function(s_JsContext, -1)) {
                if (duk_pcall(s_JsContext, 0) != 0) {
                    Logging::Log(LogLevel::Error, L"Timer callback error: %S", duk_safe_to_string(s_JsContext, -1));
                }
            }
            duk_pop(s_JsContext); // Pop result or error
        } else {
            duk_pop(s_JsContext); // Pop undefined
        }

        duk_pop_2(s_JsContext); // Pop __timers and novadesk
    }

    // JS API: novadesk.setInterval(cb, ms)
    duk_ret_t js_set_timer(duk_context* ctx) {
        bool repeating = duk_get_current_magic(ctx) != 0;
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int ms = duk_get_int_default(ctx, 1, 0);

        UINT_PTR id = s_NextTimerId++;
        
        // Store callback in novadesk.__timers[id]
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, (int)id);
        duk_dup(ctx, 0); // Duplicate the callback function
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx); // Pop __timers and novadesk

        s_Timers[id] = { repeating, (int)id };
        SetTimer(s_MessageWindow, id, ms, NULL);

        duk_push_int(ctx, (int)id);
        return 1;
    }

    // JS API: novadesk.clearTimer(id)
    duk_ret_t js_clear_timer(duk_context* ctx) {
        if (duk_get_top(ctx) == 0) return 0;
        UINT_PTR id = (UINT_PTR)duk_get_int(ctx, 0);
        auto it = s_Timers.find(id);
        if (it != s_Timers.end()) {
            KillTimer(s_MessageWindow, id);
            
            // Remove from JS side
            duk_get_global_string(ctx, "novadesk");
            if (duk_get_prop_string(ctx, -1, "__timers")) {
                duk_push_int(ctx, (int)id);
                duk_del_prop(ctx, -2);
            }
            duk_pop_2(ctx);

            s_Timers.erase(it);
        }
        return 0;
    }

    // JS API: novadesk.setImmediate(cb) / process.nextTick(cb)
    duk_ret_t js_set_immediate(duk_context* ctx) {
        if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;

        UINT_PTR id = s_NextTimerId++;
        
        // Same storage logic
        duk_get_global_string(ctx, "novadesk");
        if (!duk_get_prop_string(ctx, -1, "__timers")) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_put_prop_string(ctx, -2, "__timers");
            duk_get_prop_string(ctx, -1, "__timers");
        }
        
        duk_push_int(ctx, (int)id);
        duk_dup(ctx, 0);
        duk_put_prop(ctx, -3);
        duk_pop_2(ctx);

        s_Timers[id] = { false, (int)id };
        PostMessage(s_MessageWindow, WM_NOVADESK_ACTION, id, 0);

        duk_push_int(ctx, (int)id);
        return 1;
    }

    // JS API: novadesk.include(filename)
    duk_ret_t js_include(duk_context* ctx) {
        std::wstring filename = Utils::ToWString(duk_require_string(ctx, 0));
        std::wstring fullPath = GetWidgetsDir() + filename;

        std::string content = ReadFileContent(fullPath);
        if (content.empty()) {
            return duk_error(ctx, DUK_ERR_ERROR, "Could not read file: %s", Utils::ToString(filename).c_str());
        }

        // Execute in global scope
        if (duk_peval_string(ctx, content.c_str()) != 0) {
            return duk_throw(ctx); // Re-throw error
        }

        duk_pop(ctx); // Pop result
        return 0;
    }

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
        options.width = 0;              // Default to auto
        options.height = 0;             // Default to auto
        options.m_WDefined = false;
        options.m_HDefined = false;
        options.backgroundColor = L"rgba(255,255,255,255)";
        options.zPos = ZPOSITION_ONDESKTOP;
        options.bgAlpha = 255;
        options.windowOpacity = 255;
        options.color = RGB(255, 255, 255);
        options.draggable = true;
        options.clickThrough = false;
        options.keepOnScreen = false;
        options.snapEdges = true;

        if (duk_get_prop_string(ctx, 0, "id")) options.id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);

        // 1. Load settings FIRST to set defaults from saved state
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        // 2. Overwrite with JS options if present
        if (duk_get_prop_string(ctx, 0, "width")) {
            options.width = duk_get_int(ctx, -1);
            options.m_WDefined = (options.width > 0);
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) {
            options.height = duk_get_int(ctx, -1);
            options.m_HDefined = (options.height > 0);
        }
        duk_pop(ctx);
        // ... (rest of the properties)
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
        if (duk_get_prop_string(ctx, 0, "x")) options.x = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "y")) options.y = duk_get_int(ctx, -1);
        duk_pop(ctx);

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

            duk_push_c_function(ctx, js_widget_set_properties, 1);
            duk_put_prop_string(ctx, -2, "setProperties");

            duk_push_c_function(ctx, js_widget_get_properties, 0);
            duk_put_prop_string(ctx, -2, "getProperties");
            
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
        int x = 0, y = 0, w = 0, h = 0; // Default to auto

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
        int x = 0, y = 0, w = 0, h = 0; // Default to auto
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

    duk_ret_t js_widget_set_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        // X, Y, Width, Height
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = -1, h = -1;
        bool posChange = false;

        if (duk_get_prop_string(ctx, 0, "x")) { x = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "y")) { y = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "width")) { w = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) { h = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);

        if (posChange) widget->SetWindowPosition(x, y, w, h);

        // Opacity
        if (duk_get_prop_string(ctx, 0, "opacity")) {
            BYTE alpha = 255;
            if (duk_is_string(ctx, -1)) {
                std::string s = duk_get_string(ctx, -1);
                if (s.back() == '%') {
                    int pct = std::stoi(s.substr(0, s.length() - 1));
                    alpha = (BYTE)((pct / 100.0f) * 255);
                }
            } else {
                alpha = (BYTE)(duk_get_number(ctx, -1) * 255);
            }
            widget->SetWindowOpacity(alpha);
        }
        duk_pop(ctx);

        // Z-Pos
        if (duk_get_prop_string(ctx, 0, "zpos")) {
            std::string zPosStr = duk_get_string(ctx, -1);
            ZPOSITION zPos = widget->GetWindowZPosition();
            bool found = true;
            if (zPosStr == "ondesktop") zPos = ZPOSITION_ONDESKTOP;
            else if (zPosStr == "ontop") zPos = ZPOSITION_ONTOP;
            else if (zPosStr == "onbottom") zPos = ZPOSITION_ONBOTTOM;
            else if (zPosStr == "ontopmost") zPos = ZPOSITION_ONTOPMOST;
            else if (zPosStr == "normal") zPos = ZPOSITION_NORMAL;
            else found = false;
            
            if (found) widget->ChangeZPos(zPos);
        }
        duk_pop(ctx);

        // Background Color
        if (duk_get_prop_string(ctx, 0, "backgroundcolor")) {
            widget->SetBackgroundColor(Utils::ToWString(duk_get_string(ctx, -1)));
        }
        duk_pop(ctx);

        // Draggable
        if (duk_get_prop_string(ctx, 0, "draggable")) widget->SetDraggable(duk_get_boolean(ctx, -1));
        duk_pop(ctx);

        // ClickThrough
        if (duk_get_prop_string(ctx, 0, "clickthrough")) widget->SetClickThrough(duk_get_boolean(ctx, -1));
        duk_pop(ctx);

        // KeepOnScreen
        if (duk_get_prop_string(ctx, 0, "keeponscreen")) widget->SetKeepOnScreen(duk_get_boolean(ctx, -1));
        duk_pop(ctx);

        // SnapEdges
        if (duk_get_prop_string(ctx, 0, "snapedges")) widget->SetSnapEdges(duk_get_boolean(ctx, -1));
        duk_pop(ctx);

        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_get_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!widget) return DUK_RET_TYPE_ERROR;

        const WidgetOptions& opt = widget->GetOptions();
        
        duk_push_object(ctx);
        duk_push_int(ctx, opt.x); duk_put_prop_string(ctx, -2, "x");
        duk_push_int(ctx, opt.y); duk_put_prop_string(ctx, -2, "y");
        duk_push_int(ctx, opt.width); duk_put_prop_string(ctx, -2, "width");
        duk_push_int(ctx, opt.height); duk_put_prop_string(ctx, -2, "height");
        
        const char* zPosStr = "normal";
        switch (opt.zPos) {
            case ZPOSITION_ONDESKTOP: zPosStr = "ondesktop"; break;
            case ZPOSITION_ONTOP:     zPosStr = "ontop";     break;
            case ZPOSITION_ONBOTTOM:  zPosStr = "onbottom";  break;
            case ZPOSITION_ONTOPMOST: zPosStr = "ontopmost"; break;
        }
        duk_push_string(ctx, zPosStr); duk_put_prop_string(ctx, -2, "zpos");
        
        duk_push_number(ctx, opt.windowOpacity / 255.0); duk_put_prop_string(ctx, -2, "opacity");
        duk_push_boolean(ctx, opt.draggable); duk_put_prop_string(ctx, -2, "draggable");
        duk_push_boolean(ctx, opt.clickThrough); duk_put_prop_string(ctx, -2, "clickthrough");
        duk_push_boolean(ctx, opt.keepOnScreen); duk_put_prop_string(ctx, -2, "keeponscreen");
        duk_push_boolean(ctx, opt.snapEdges); duk_put_prop_string(ctx, -2, "snapedges");
        duk_push_string(ctx, Utils::ToString(opt.backgroundColor).c_str()); duk_put_prop_string(ctx, -2, "backgroundcolor");

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
        duk_push_c_function(ctx, js_include, 1);
        duk_put_prop_string(ctx, -2, "include");

        // Timers
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 1); // 1 = repeating
        duk_put_prop_string(ctx, -2, "setInterval");
        
        duk_push_c_function(ctx, js_set_timer, 2);
        duk_set_magic(ctx, -1, 0); // 0 = one-shot
        duk_put_prop_string(ctx, -2, "setTimeout");

        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_prop_string(ctx, -2, "clearInterval");
        duk_push_c_function(ctx, js_clear_timer, 1);
        duk_put_prop_string(ctx, -2, "clearTimeout");

        duk_push_c_function(ctx, js_set_immediate, 1);
        duk_put_prop_string(ctx, -2, "setImmediate");

        // process.nextTick
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_set_immediate, 1);
        duk_put_prop_string(ctx, -2, "nextTick");
        duk_put_prop_string(ctx, -2, "process");

        duk_put_global_string(ctx, "novadesk");

        // Register widgetWindow constructor
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    bool LoadAndExecuteScript(duk_context* ctx) {
        // Get executable path to find index.js
        std::wstring scriptPath = GetWidgetsDir() + L"index.js";

        Logging::Log(LogLevel::Info, L"Loading script from: %s", scriptPath.c_str());

        std::string content = ReadFileContent(scriptPath);
        if (content.empty()) {
            Logging::Log(LogLevel::Error, L"Failed to open index.js at %s", scriptPath.c_str());
            return false;
        }
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

    void OnTimer(UINT_PTR id) {
        auto it = s_Timers.find(id);
        if (it != s_Timers.end()) {
            Timer t = it->second;
            if (!t.repeating) {
                KillTimer(s_MessageWindow, id);
                s_Timers.erase(it);
                
                // Note: We don't remove from JS object here, we do it in CallStoredCallback
                // or just leave it for potential memory leak... actually let's do it right.
                CallStoredCallback(t.callbackIndex);
                
                // Cleanup JS storage after execution
                duk_get_global_string(s_JsContext, "novadesk");
                if (duk_get_prop_string(s_JsContext, -1, "__timers")) {
                    duk_push_int(s_JsContext, t.callbackIndex);
                    duk_del_prop(s_JsContext, -2);
                }
                duk_pop_2(s_JsContext);
            } else {
                CallStoredCallback(t.callbackIndex);
            }
        }
    }

    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NOVADESK_ACTION) {
            UINT_PTR id = (UINT_PTR)wParam;
            auto it = s_Timers.find(id);
            if (it != s_Timers.end()) {
                Timer t = it->second;
                s_Timers.erase(it);
                CallStoredCallback(t.callbackIndex);
                
                // Cleanup JS storage
                duk_get_global_string(s_JsContext, "novadesk");
                if (duk_get_prop_string(s_JsContext, -1, "__timers")) {
                    duk_push_int(s_JsContext, t.callbackIndex);
                    duk_del_prop(s_JsContext, -2);
                }
                duk_pop_2(s_JsContext);
            }
        }
    }

    void SetMessageWindow(HWND hWnd) {
        s_MessageWindow = hWnd;
    }

}
