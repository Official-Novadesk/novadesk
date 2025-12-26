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
#include "PathUtils.h"
#include <string>

namespace PropertyParser {

    // Helper class for standardized property reading
    class PropertyReader {
    public:
        PropertyReader(duk_context* ctx) : m_Ctx(ctx) {}

        bool GetString(const char* key, std::wstring& outStr) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                if (duk_is_string(m_Ctx, -1)) {
                    outStr = Utils::ToWString(duk_get_string(m_Ctx, -1));
                    duk_pop(m_Ctx);
                    return true;
                }
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetInt(const char* key, int& outInt) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                outInt = duk_get_int(m_Ctx, -1);
                duk_pop(m_Ctx);
                return true;
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetFloat(const char* key, float& outFloat) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                outFloat = (float)duk_get_number(m_Ctx, -1);
                duk_pop(m_Ctx);
                return true;
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetBool(const char* key, bool& outBool) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                outBool = duk_get_boolean(m_Ctx, -1);
                duk_pop(m_Ctx);
                return true;
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetColor(const char* key, COLORREF& outColor, BYTE& outAlpha) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                std::wstring colorStr = Utils::ToWString(duk_get_string(m_Ctx, -1));
                if (ColorUtil::ParseRGBA(colorStr, outColor, outAlpha)) {
                    duk_pop(m_Ctx);
                    return true;
                }
            }
            duk_pop(m_Ctx);
            return false;
        }
        
        bool GetFloatArray(const char* key, std::vector<float>& outArray, int minSize) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                if (duk_is_array(m_Ctx, -1)) {
                    int len = (int)duk_get_length(m_Ctx, -1);
                    if (len >= minSize) {
                         outArray.resize(len);
                         for (int i = 0; i < len; i++) {
                             duk_get_prop_index(m_Ctx, -1, i);
                             outArray[i] = (float)duk_get_number(m_Ctx, -1);
                             duk_pop(m_Ctx);
                         }
                         duk_pop(m_Ctx);
                         return true;
                    }
                }
            }
            duk_pop(m_Ctx);
            return false;
        }

    private:
        duk_context* m_Ctx;
    };

    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options) {
        if (!duk_is_object(ctx, -1)) return;
        PropertyReader reader(ctx);

        reader.GetString("id", options.id);

        // Load settings if ID exists
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        if (reader.GetInt("width", options.width)) options.m_WDefined = (options.width > 0);
        if (reader.GetInt("height", options.height)) options.m_HDefined = (options.height > 0);
        
        reader.GetColor("backgroundcolor", options.color, options.bgAlpha);
        // Sync string version for now until WidgetOptions is fully refactored
        // options.backgroundColor = ... (We might need to reverse construct string or just keep it sync if needed)
        // For now let's re-read string if we really need it, or just rely on color/alpha
        if (duk_get_prop_string(ctx, -1, "backgroundcolor")) {
             options.backgroundColor = Utils::ToWString(duk_get_string(ctx, -1));
        }
        duk_pop(ctx);

        // Opacity
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

        std::wstring zPosStr;
        if (reader.GetString("zpos", zPosStr)) {
            // Simple mapping
            std::string s = Utils::ToString(zPosStr); // efficient enough
            if (s == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
            else if (s == "ontop") options.zPos = ZPOSITION_ONTOP;
            else if (s == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
            else if (s == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
            else if (s == "normal") options.zPos = ZPOSITION_NORMAL;
        }

        reader.GetBool("draggable", options.draggable);
        reader.GetBool("clickthrough", options.clickThrough);
        reader.GetBool("keeponscreen", options.keepOnScreen);
        reader.GetBool("snapedges", options.snapEdges);
        reader.GetInt("x", options.x);
        reader.GetInt("y", options.y);
    }

    void ParseElementOptionsInternal(duk_context* ctx, ElementOptions& options) {
        PropertyReader reader(ctx);

        reader.GetString("id", options.id);
        
        // Position & Size
        reader.GetInt("x", options.x);
        reader.GetInt("y", options.y);
        reader.GetInt("width", options.width);
        reader.GetInt("height", options.height);

        // Rotation
        reader.GetFloat("rotate", options.rotate);

        // Background / Gradient
        std::wstring solidColorStr;
        if (reader.GetString("solidcolor", solidColorStr)) {
             if (ColorUtil::ParseRGBA(solidColorStr, options.solidColor, options.solidAlpha)) {
                 options.hasSolidColor = true;
             }
        }
        reader.GetInt("solidcolorradius", options.solidColorRadius);
        
        std::wstring solidColor2Str;
        if (reader.GetString("solidcolor2", solidColor2Str)) {
             if (ColorUtil::ParseRGBA(solidColor2Str, options.solidColor2, options.solidAlpha2)) {
                 options.hasGradient = true;
             }
        }
        reader.GetFloat("gradientangle", options.gradientAngle);

        // Bevel
        reader.GetInt("beveltype", options.bevelType);
        if (options.bevelType > 0) {
            reader.GetInt("bevelwidth", options.bevelWidth);
            reader.GetColor("bevelcolor1", options.bevelColor1, options.bevelAlpha1);
            reader.GetColor("bevelcolor2", options.bevelColor2, options.bevelAlpha2);
        }

        // Padding
        if (duk_get_prop_string(ctx, -1, "padding")) {
            if (duk_is_number(ctx, -1)) {
                int pad = duk_get_int(ctx, -1);
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = pad;
            } else if (duk_is_array(ctx, -1)) {
                int len = (int)duk_get_length(ctx, -1);
                if (len == 4) {
                    duk_get_prop_index(ctx, -1, 0); options.paddingLeft = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 1); options.paddingTop = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 2); options.paddingRight = duk_get_int(ctx, -1); duk_pop(ctx);
                    duk_get_prop_index(ctx, -1, 3); options.paddingBottom = duk_get_int(ctx, -1); duk_pop(ctx);
                } else if (len == 2) {
                     duk_get_prop_index(ctx, -1, 0); options.paddingLeft = options.paddingRight = duk_get_int(ctx, -1); duk_pop(ctx);
                     duk_get_prop_index(ctx, -1, 1); options.paddingTop = options.paddingBottom = duk_get_int(ctx, -1); duk_pop(ctx);
                }
            }
        }
        duk_pop(ctx); // Pop 'padding' property

        // Mouse Actions
        reader.GetString("onleftmouseup", options.onLeftMouseUp);
        reader.GetString("onleftmousedown", options.onLeftMouseDown);
        reader.GetString("onleftdoubleclick", options.onLeftDoubleClick);
        reader.GetString("onrightmouseup", options.onRightMouseUp);
        reader.GetString("onrightmousedown", options.onRightMouseDown);
        reader.GetString("onrightdoubleclick", options.onRightDoubleClick);
        reader.GetString("onmiddlemouseup", options.onMiddleMouseUp);
        reader.GetString("onmiddlemousedown", options.onMiddleMouseDown);
        reader.GetString("onmiddledoubleclick", options.onMiddleDoubleClick);
        
        reader.GetString("onx1mouseup", options.onX1MouseUp);
        reader.GetString("onx1mousedown", options.onX1MouseDown);
        reader.GetString("onx1doubleclick", options.onX1DoubleClick);
        reader.GetString("onx2mouseup", options.onX2MouseUp);
        reader.GetString("onx2mousedown", options.onX2MouseDown);
        reader.GetString("onx2doubleclick", options.onX2DoubleClick);
        
        reader.GetString("onscrollup", options.onScrollUp);
        reader.GetString("onscrolldown", options.onScrollDown);
        reader.GetString("onscrollleft", options.onScrollLeft);
        reader.GetString("onscrollright", options.onScrollRight);
        
        reader.GetString("onmouseover", options.onMouseOver);
        reader.GetString("onmouseleave", options.onMouseLeave);
        
        reader.GetBool("antialias", options.antialias);
    }

    void ParseImageOptions(duk_context* ctx, ImageOptions& options) {
        if (!duk_is_object(ctx, -1)) return;
        
        // Parse base options first
        ParseElementOptionsInternal(ctx, options);
        PropertyReader reader(ctx);

        // Path
        if (reader.GetString("path", options.path)) {
            options.path = PathUtils::ResolvePath(options.path, PathUtils::GetWidgetsDir());
        }

        // Image specific
        reader.GetInt("preserveaspectratio", options.preserveAspectRatio);
        reader.GetBool("grayscale", options.grayscale);
        reader.GetBool("tile", options.tile);
        int alpha = 255;
        if (reader.GetInt("imagealpha", alpha)) options.imageAlpha = (BYTE)alpha;

        std::wstring tintStr;
        if (reader.GetString("imagetint", tintStr)) {
             if (ColorUtil::ParseRGBA(tintStr, options.imageTint, options.imageTintAlpha)) {
                 options.hasImageTint = true;
             }
        }
        
        if (reader.GetFloatArray("transformmatrix", options.transformMatrix, 6)) {
            options.hasTransformMatrix = true;
        }

        if (reader.GetFloatArray("colormatrix", options.colorMatrix, 20)) { 
             options.hasColorMatrix = true;
        }
    }

    void ParseTextOptions(duk_context* ctx, TextOptions& options) {
        if (!duk_is_object(ctx, -1)) return;

        // Parse base options first
        ParseElementOptionsInternal(ctx, options);
        PropertyReader reader(ctx);

        reader.GetString("text", options.text);
        reader.GetString("fontface", options.fontFace);
        reader.GetInt("fontsize", options.fontSize);
        
        reader.GetColor("fontcolor", options.fontColor, options.alpha);
        
        std::wstring weight;
        if (reader.GetString("fontweight", weight) && weight == L"bold") options.bold = true;
        
        std::wstring style;
        if (reader.GetString("fontstyle", style) && style == L"italic") options.italic = true;

        std::wstring alignStr;
        if (reader.GetString("textalign", alignStr)) {
            if (alignStr == L"left" || alignStr == L"lefttop") options.textAlign = ALIGN_LEFT_TOP;
            else if (alignStr == L"center" || alignStr == L"centertop") options.textAlign = ALIGN_CENTER_TOP;
            else if (alignStr == L"right" || alignStr == L"righttop") options.textAlign = ALIGN_RIGHT_TOP;
            else if (alignStr == L"leftcenter") options.textAlign = ALIGN_LEFT_CENTER;
            else if (alignStr == L"centercenter") options.textAlign = ALIGN_CENTER_CENTER;
            else if (alignStr == L"rightcenter") options.textAlign = ALIGN_RIGHT_CENTER;
            else if (alignStr == L"leftbottom") options.textAlign = ALIGN_LEFT_BOTTOM;
            else if (alignStr == L"centerbottom") options.textAlign = ALIGN_CENTER_BOTTOM;
            else if (alignStr == L"rightbottom") options.textAlign = ALIGN_RIGHT_BOTTOM;
        }
        
        int clipVal = 0;
        if (reader.GetInt("clipstring", clipVal)) options.clip = (ClipString)clipVal;
        reader.GetInt("clipstringw", options.clipW);
        reader.GetInt("clipstringh", options.clipH);
    }

    void ApplyWidgetProperties(duk_context* ctx, Widget* widget) {
        if (!widget || !duk_is_object(ctx, -1)) return;
        PropertyReader reader(ctx);

        // X, Y, Width, Height
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = -1, h = -1;
        bool posChange = false;

        if (reader.GetInt("x", x)) posChange = true;
        if (reader.GetInt("y", y)) posChange = true;
        if (reader.GetInt("width", w)) posChange = true;
        if (reader.GetInt("height", h)) posChange = true;

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
        std::wstring zPosStr;
        if (reader.GetString("zpos", zPosStr)) {
            std::string s = Utils::ToString(zPosStr);
            ZPOSITION zPos = widget->GetWindowZPosition();
            bool found = true;
            if (s == "ondesktop") zPos = ZPOSITION_ONDESKTOP;
            else if (s == "ontop") zPos = ZPOSITION_ONTOP;
            else if (s == "onbottom") zPos = ZPOSITION_ONBOTTOM;
            else if (s == "ontopmost") zPos = ZPOSITION_ONTOPMOST;
            else if (s == "normal") zPos = ZPOSITION_NORMAL;
            else found = false;
            
            if (found) widget->ChangeZPos(zPos);
        }

        // Background Color
        std::wstring bgColor;
        if (reader.GetString("backgroundcolor", bgColor)) {
            widget->SetBackgroundColor(bgColor);
        }

        // Boolean toggles
        bool draggable;
        if (reader.GetBool("draggable", draggable)) widget->SetDraggable(draggable);
        
        bool clickThrough;
        if (reader.GetBool("clickthrough", clickThrough)) widget->SetClickThrough(clickThrough);
        
        bool keepOnScreen;
        if (reader.GetBool("keeponscreen", keepOnScreen)) widget->SetKeepOnScreen(keepOnScreen);
        
        bool snapEdges;
        if (reader.GetBool("snapedges", snapEdges)) widget->SetSnapEdges(snapEdges);
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

    void ApplyElementOptions(Element* element, const ElementOptions& options) {
        if (!element) return;

        if (options.hasSolidColor) {
            element->SetSolidColor(options.solidColor, options.solidAlpha);
        }
        if (options.solidColorRadius > 0) {
            element->SetCornerRadius(options.solidColorRadius);
        }
        
        if (options.hasGradient) {
            element->SetGradient(options.solidColor2, options.solidAlpha2, options.gradientAngle);
        }

        if (options.bevelType > 0) {
            element->SetBevel(options.bevelType, options.bevelWidth, options.bevelColor1, options.bevelAlpha1, options.bevelColor2, options.bevelAlpha2);
        }
        
        element->SetRotate(options.rotate);

        // Padding
        if (options.paddingLeft || options.paddingTop || options.paddingRight || options.paddingBottom) {
            element->SetPadding(options.paddingLeft, options.paddingTop, options.paddingRight, options.paddingBottom);
        }

        // Mouse Actions
        if (!options.onLeftMouseUp.empty()) element->m_OnLeftMouseUp = options.onLeftMouseUp;
        if (!options.onLeftMouseDown.empty()) element->m_OnLeftMouseDown = options.onLeftMouseDown;
        if (!options.onLeftDoubleClick.empty()) element->m_OnLeftDoubleClick = options.onLeftDoubleClick;
        if (!options.onRightMouseUp.empty()) element->m_OnRightMouseUp = options.onRightMouseUp;
        if (!options.onRightMouseDown.empty()) element->m_OnRightMouseDown = options.onRightMouseDown;
        if (!options.onRightDoubleClick.empty()) element->m_OnRightDoubleClick = options.onRightDoubleClick;
        if (!options.onMiddleMouseUp.empty()) element->m_OnMiddleMouseUp = options.onMiddleMouseUp;
        if (!options.onMiddleMouseDown.empty()) element->m_OnMiddleMouseDown = options.onMiddleMouseDown;
        if (!options.onMiddleDoubleClick.empty()) element->m_OnMiddleDoubleClick = options.onMiddleDoubleClick;
        
        if (!options.onX1MouseUp.empty()) element->m_OnX1MouseUp = options.onX1MouseUp;
        if (!options.onX1MouseDown.empty()) element->m_OnX1MouseDown = options.onX1MouseDown;
        if (!options.onX1DoubleClick.empty()) element->m_OnX1DoubleClick = options.onX1DoubleClick;
        if (!options.onX2MouseUp.empty()) element->m_OnX2MouseUp = options.onX2MouseUp;
        if (!options.onX2MouseDown.empty()) element->m_OnX2MouseDown = options.onX2MouseDown;
        if (!options.onX2DoubleClick.empty()) element->m_OnX2DoubleClick = options.onX2DoubleClick;
        
        if (!options.onScrollUp.empty()) element->m_OnScrollUp = options.onScrollUp;
        if (!options.onScrollDown.empty()) element->m_OnScrollDown = options.onScrollDown;
        if (!options.onScrollLeft.empty()) element->m_OnScrollLeft = options.onScrollLeft;
        if (!options.onScrollRight.empty()) element->m_OnScrollRight = options.onScrollRight;
        
        if (!options.onMouseOver.empty()) element->m_OnMouseOver = options.onMouseOver;
        if (!options.onMouseLeave.empty()) element->m_OnMouseLeave = options.onMouseLeave;
        
        element->SetAntiAlias(options.antialias);
    }

    void ParseElementOptions(duk_context* ctx, Element* element) {
        if (!element || !duk_is_object(ctx, -1)) return;
        
        ElementOptions options;
        // Parse into our struct
        ParseElementOptionsInternal(ctx, options);
        // Apply to element
        ApplyElementOptions(element, options);
    }
}
