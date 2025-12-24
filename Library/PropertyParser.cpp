/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "Utils.h"
#include "ColorUtil.h"
#include "Settings.h"
#include <string>

namespace PropertyParser {

    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options) {
        if (!duk_is_object(ctx, -1)) return;

        if (duk_get_prop_string(ctx, -1, "id")) options.id = Utils::ToWString(duk_get_string(ctx, -1));
        duk_pop(ctx);

        // Load settings if ID exists
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        if (duk_get_prop_string(ctx, -1, "width")) {
            options.width = duk_get_int(ctx, -1);
            options.m_WDefined = (options.width > 0);
        }
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "height")) {
            options.height = duk_get_int(ctx, -1);
            options.m_HDefined = (options.height > 0);
        }
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "backgroundcolor")) {
            options.backgroundColor = Utils::ToWString(duk_get_string(ctx, -1));
            ColorUtil::ParseRGBA(options.backgroundColor, options.color, options.bgAlpha);
        }
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "opacity")) {
            if (duk_is_string(ctx, -1)) {
                std::string s = duk_get_string(ctx, -1);
                if (!s.empty() && s.back() == '%') {
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

        if (duk_get_prop_string(ctx, -1, "zpos")) {
            std::string zPosStr = duk_get_string(ctx, -1);
            if (zPosStr == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
            else if (zPosStr == "ontop") options.zPos = ZPOSITION_ONTOP;
            else if (zPosStr == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
            else if (zPosStr == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
            else if (zPosStr == "normal") options.zPos = ZPOSITION_NORMAL;
        }
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "draggable")) options.draggable = duk_get_boolean(ctx, -1);
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "clickthrough")) options.clickThrough = duk_get_boolean(ctx, -1);
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "keeponscreen")) options.keepOnScreen = duk_get_boolean(ctx, -1);
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "snapedges")) options.snapEdges = duk_get_boolean(ctx, -1);
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "x")) options.x = duk_get_int(ctx, -1);
        duk_pop(ctx);

        if (duk_get_prop_string(ctx, -1, "y")) options.y = duk_get_int(ctx, -1);
        duk_pop(ctx);
    }

    void ApplyWidgetProperties(duk_context* ctx, Widget* widget) {
        if (!widget || !duk_is_object(ctx, -1)) return;

        // X, Y, Width, Height
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = -1, h = -1;
        bool posChange = false;

        if (duk_get_prop_string(ctx, -1, "x")) { x = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "y")) { y = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "width")) { w = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "height")) { h = duk_get_int(ctx, -1); posChange = true; }
        duk_pop(ctx);

        if (posChange) widget->SetWindowPosition(x, y, w, h);

        // Opacity
        if (duk_get_prop_string(ctx, -1, "opacity")) {
            BYTE alpha = 255;
            if (duk_is_string(ctx, -1)) {
                std::string s = duk_get_string(ctx, -1);
                if (!s.empty() && s.back() == '%') {
                    int pct = std::stoi(s.substr(0, s.length() - 1));
                    alpha = (BYTE)((pct / 100.0f) * 255);
                } else {
                    alpha = (BYTE)(duk_get_number(ctx, -1) * 255);
                }
            } else {
                alpha = (BYTE)(duk_get_number(ctx, -1) * 255);
            }
            widget->SetWindowOpacity(alpha);
        }
        duk_pop(ctx);

        // Z-Pos
        if (duk_get_prop_string(ctx, -1, "zpos")) {
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
        if (duk_get_prop_string(ctx, -1, "backgroundcolor")) {
            widget->SetBackgroundColor(Utils::ToWString(duk_get_string(ctx, -1)));
        }
        duk_pop(ctx);

        // Boolean toggles
        if (duk_get_prop_string(ctx, -1, "draggable")) widget->SetDraggable(duk_get_boolean(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "clickthrough")) widget->SetClickThrough(duk_get_boolean(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "keeponscreen")) widget->SetKeepOnScreen(duk_get_boolean(ctx, -1));
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, -1, "snapedges")) widget->SetSnapEdges(duk_get_boolean(ctx, -1));
        duk_pop(ctx);
    }

    void PushWidgetProperties(duk_context* ctx, Widget* widget) {
        if (!widget) return;
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
    }

    void ParseElementOptions(duk_context* ctx, Element* element) {
        if (!element || !duk_is_object(ctx, -1)) return;
        
        auto getStr = [&](const char* key) -> std::wstring {
            if (duk_get_prop_string(ctx, -1, key)) {
                if (duk_is_string(ctx, -1)) {
                    std::wstring val = Utils::ToWString(duk_get_string(ctx, -1));
                    duk_pop(ctx);
                    return val;
                }
            }
            duk_pop(ctx);
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
        
        // Gradient properties
        std::wstring solidColor2 = getStr("solidcolor2");
        if (!solidColor2.empty()) {
            COLORREF color2 = 0;
            BYTE alpha2 = 0;
            if (ColorUtil::ParseRGBA(solidColor2, color2, alpha2)) {
                float angle = 0.0f;
                if (duk_get_prop_string(ctx, -1, "gradientangle")) {
                    angle = (float)duk_get_number(ctx, -1);
                }
                duk_pop(ctx);
                element->SetGradient(color2, alpha2, angle);
            }
        }
        
        // Bevel properties
        if (duk_get_prop_string(ctx, -1, "beveltype")) {
            int bevelType = duk_get_int(ctx, -1);
            int bevelWidth = 2; // Default
            COLORREF bevelColor1 = RGB(255, 255, 255);
            BYTE bevelAlpha1 = 200;
            COLORREF bevelColor2 = RGB(0, 0, 0);
            BYTE bevelAlpha2 = 150;
            
            duk_pop(ctx);
            
            if (duk_get_prop_string(ctx, -1, "bevelwidth")) {
                bevelWidth = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            
            std::wstring bc1 = getStr("bevelcolor1");
            if (!bc1.empty()) {
                ColorUtil::ParseRGBA(bc1, bevelColor1, bevelAlpha1);
            }
            
            std::wstring bc2 = getStr("bevelcolor2");
            if (!bc2.empty()) {
                ColorUtil::ParseRGBA(bc2, bevelColor2, bevelAlpha2);
            }
            
            element->SetBevel(bevelType, bevelWidth, bevelColor1, bevelAlpha1, bevelColor2, bevelAlpha2);
        } else {
            duk_pop(ctx);
        }
        
        // Antialiasing property
        if (duk_get_prop_string(ctx, -1, "antialias")) {
            bool antialias = duk_get_boolean(ctx, -1);
            element->SetAntiAlias(antialias);
        }
        duk_pop(ctx);
        
        // Padding properties
        int padLeft = 0, padTop = 0, padRight = 0, padBottom = 0;
        bool hasPadding = false;
        
        if (duk_get_prop_string(ctx, -1, "padding")) {
            Logging::Log(LogLevel::Debug, L"PropertyParser: Found 'padding' property");
            if (duk_is_number(ctx, -1)) {
                // Single value for all sides
                int pad = duk_get_int(ctx, -1);
                padLeft = padTop = padRight = padBottom = pad;
                hasPadding = true;
                Logging::Log(LogLevel::Debug, L"PropertyParser: Parsed single padding: %d", pad);
            } else if (duk_is_array(ctx, -1)) {
                // Array: [left, top, right, bottom] or [horizontal, vertical]
                int len = (int)duk_get_length(ctx, -1);
                Logging::Log(LogLevel::Debug, L"PropertyParser: Parsed padding array length: %d", len);
                if (len == 4) {
                    duk_get_prop_index(ctx, -1, 0); padLeft = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 1); padTop = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 2); padRight = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 3); padBottom = duk_get_int(ctx, -1); duk_pop(ctx);
                    hasPadding = true;
                } else if (len == 2) {
                    duk_get_prop_index(ctx, -1, 0); padLeft = padRight = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 1); padTop = padBottom = duk_get_int(ctx, -1); duk_pop(ctx);
                    hasPadding = true;
                }
            } else {
                 Logging::Log(LogLevel::Debug, L"PropertyParser: 'padding' is neither number nor array");
            }
        } else {
             Logging::Log(LogLevel::Debug, L"PropertyParser: 'padding' property NOT found");
        }
        duk_pop(ctx);
        
        if (hasPadding) {
            Logging::Log(LogLevel::Debug, L"PropertyParser: Setting padding [%d, %d, %d, %d]", padLeft, padTop, padRight, padBottom);
            element->SetPadding(padLeft, padTop, padRight, padBottom);
        }
    }
}
