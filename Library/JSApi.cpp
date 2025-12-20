/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "Widget.h"
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
        Logging::Log(LogLevel::Debug, L"js_create_widget_window called");
        if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        WidgetOptions options;
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

        if (duk_get_prop_string(ctx, 0, "width")) options.width = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) options.height = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "backgroundColor")) {
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
        if (duk_get_prop_string(ctx, 0, "zPos")) {
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
        if (duk_get_prop_string(ctx, 0, "clickThrough")) options.clickThrough = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "keepOnScreen")) options.keepOnScreen = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "snapEdges")) options.snapEdges = duk_get_boolean(ctx, -1);
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
            
            return 1; // Return the object
        }
        else {
            Logging::Log(LogLevel::Error, L"Failed to create widget.");
            delete widget;
        }

        return 0;
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
        TextAlign align = ALIGN_LEFT;
        VerticalAlign vAlign = VALIGN_TOP;
        float lineHeight = 1.0f;

        if (duk_get_prop_string(ctx, 0, "id")) id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "text")) text = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontFamily")) fontFamily = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "x")) x = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "y")) y = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "width")) w = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) h = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontSize")) fontSize = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "color")) {
            std::wstring colorStr = Utils::ToWString(duk_get_string(ctx, -1));
            ColorUtil::ParseRGBA(colorStr, color, alpha);
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontWeight")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "bold") bold = true;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "fontStyle")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "italic") italic = true;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "align")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "center") align = ALIGN_CENTER;
            else if (s == "right") align = ALIGN_RIGHT;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "verticalAlign")) {
            std::string s = duk_get_string(ctx, -1);
            if (s == "middle") vAlign = VALIGN_MIDDLE;
            else if (s == "bottom") vAlign = VALIGN_BOTTOM;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "lineHeight")) lineHeight = (float)duk_get_number(ctx, -1);
        duk_pop(ctx);

        widget->AddText(id, x, y, w, h, text, fontFamily, fontSize, color, alpha, bold, italic, align, vAlign, lineHeight);
        
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

    void InitializeJavaScriptAPI(duk_context* ctx) {
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
        std::wstring scriptPath = exePath.substr(0, lastBackslash + 1) + L"index.js";

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

        Logging::Log(LogLevel::Info, L"Script execution successful. Calling novadeskAppReady()...");

        // Call novadeskAppReady if defined
        duk_get_global_string(ctx, "novadeskAppReady");
        if (duk_is_function(ctx, -1)) {
            if (duk_pcall(ctx, 0) != 0) {
                Logging::Log(LogLevel::Error, L"novadeskAppReady failed: %S", duk_safe_to_string(ctx, -1));
                duk_pop(ctx);
                return false;
            }
        }
        else {
            Logging::Log(LogLevel::Info, L"novadeskAppReady not defined.");
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
