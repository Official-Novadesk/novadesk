/* Copyright (C) 2026 OfficialNovadesk 
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
#include "JSApi/JSEvents.h"
#include "JSApi/JSUtils.h"
#include "Logging.h"

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
                if (duk_is_number(m_Ctx, -1)) {
                    outInt = duk_get_int(m_Ctx, -1);
                    duk_pop(m_Ctx);
                    return true;
                }
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetFloat(const char* key, float& outFloat) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                if (duk_is_number(m_Ctx, -1)) {
                    outFloat = (float)duk_get_number(m_Ctx, -1);
                    duk_pop(m_Ctx);
                    return true;
                }
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetBool(const char* key, bool& outBool) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                if (duk_is_boolean(m_Ctx, -1)) {
                    outBool = duk_get_boolean(m_Ctx, -1) != 0;
                    duk_pop(m_Ctx);
                    return true;
                }
            }
            duk_pop(m_Ctx);
            return false;
        }

        bool GetColor(const char* key, COLORREF& outColor, BYTE& outAlpha) {
            std::wstring colorStr;
            if (GetString(key, colorStr)) {
                return ColorUtil::ParseRGBA(colorStr, outColor, outAlpha);
            }
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

        void GetEvent(const char* key, int& outId) {
            if (duk_get_prop_string(m_Ctx, -1, key)) {
                if (duk_is_function(m_Ctx, -1)) {
                    outId = JSApi::RegisterEventCallback(m_Ctx, -1);
                }
            }
            duk_pop(m_Ctx);
        }

        bool ParseShadow(TextShadow& shadow) {
            if (duk_is_object(m_Ctx, -1)) {
                if (duk_get_prop_string(m_Ctx, -1, "x")) shadow.offsetX = (float)duk_get_number(m_Ctx, -1);
                duk_pop(m_Ctx);
                if (duk_get_prop_string(m_Ctx, -1, "y")) shadow.offsetY = (float)duk_get_number(m_Ctx, -1);
                duk_pop(m_Ctx);
                if (duk_get_prop_string(m_Ctx, -1, "blur")) shadow.blur = (float)duk_get_number(m_Ctx, -1);
                duk_pop(m_Ctx);
                
                if (duk_get_prop_string(m_Ctx, -1, "color")) {
                    std::wstring colorStr = Utils::ToWString(duk_get_string(m_Ctx, -1));
                    ColorUtil::ParseRGBA(colorStr, shadow.color, shadow.alpha);
                }
                duk_pop(m_Ctx);
                return true;
            }
            return false;
        }

    private:
        duk_context* m_Ctx;
    };

    /*
    ** Parse WidgetOptions from a Duktape object at the top of the stack.
    ** Optionally loads settings if an 'id' is present.
    */

    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options, const std::wstring& baseDir) {
        if (!duk_is_object(ctx, -1)) return;
        PropertyReader reader(ctx);

        std::wstring finalBaseDir = baseDir.empty() ? PathUtils::GetWidgetsDir() : baseDir;

        reader.GetString("id", options.id);

        // Load settings if ID exists
        if (!options.id.empty()) {
            Settings::LoadWidget(options.id, options);
        }

        if (reader.GetInt("width", options.width)) options.m_WDefined = (options.width > 0);
        if (reader.GetInt("height", options.height)) options.m_HDefined = (options.height > 0);
        
        reader.GetColor("backgroundColor", options.color, options.bgAlpha);
        if (duk_get_prop_string(ctx, -1, "backgroundColor")) {
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
        if (reader.GetString("zPos", zPosStr)) {
            // Simple mapping
            std::string s = Utils::ToString(zPosStr); // efficient enough
            if (s == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
            else if (s == "ontop") options.zPos = ZPOSITION_ONTOP;
            else if (s == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
            else if (s == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
            else if (s == "normal") options.zPos = ZPOSITION_NORMAL;
        }

        reader.GetBool("draggable", options.draggable);
        reader.GetBool("clickThrough", options.clickThrough);
        reader.GetBool("keepOnScreen", options.keepOnScreen);
        reader.GetBool("snapEdges", options.snapEdges);
        reader.GetInt("x", options.x);
        reader.GetInt("y", options.y);
        reader.GetBool("show", options.show);

        if (reader.GetString("script", options.scriptPath)) {
            options.scriptPath = JSApi::ResolveScriptPath(ctx, options.scriptPath);
        }
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
        if (reader.GetString("solidColor", solidColorStr)) {
             if (ColorUtil::ParseRGBA(solidColorStr, options.solidColor, options.solidAlpha)) {
                 options.hasSolidColor = true;
             }
        }
        reader.GetInt("solidColorRadius", options.solidColorRadius);
        
        std::wstring solidColor2Str;
        if (reader.GetString("solidColor2", solidColor2Str)) {
             if (ColorUtil::ParseRGBA(solidColor2Str, options.solidColor2, options.solidAlpha2)) {
                 options.hasGradient = true;
             }
        }
        reader.GetFloat("gradientAngle", options.gradientAngle);

        // Bevel
        std::wstring bevelStr;
        if (reader.GetString("bevelType", bevelStr)) {
            if (bevelStr == L"none") options.bevelType = 0;
            else if (bevelStr == L"raised") options.bevelType = 1;
            else if (bevelStr == L"sunken") options.bevelType = 2;
            else if (bevelStr == L"emboss") options.bevelType = 3;
            else if (bevelStr == L"pillow") options.bevelType = 4;
            else options.bevelType = 0; // Default to none if unknown string
        } else {
            options.bevelType = 0; // Default to none if missing
        }

        if (options.bevelType > 0) {
            options.bevelWidth = 1; // Default to 1
            reader.GetInt("bevelWidth", options.bevelWidth);
            
            // bevelColor is the property for the primary bevel color
            reader.GetColor("bevelColor", options.bevelColor, options.bevelAlpha);
            reader.GetColor("bevelColor2", options.bevelColor2, options.bevelAlpha2);
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
        reader.GetEvent("onLeftMouseUp", options.onLeftMouseUpCallbackId);
        reader.GetEvent("onLeftMouseDown", options.onLeftMouseDownCallbackId);
        reader.GetEvent("onLeftDoubleClick", options.onLeftDoubleClickCallbackId);
        reader.GetEvent("onRightMouseUp", options.onRightMouseUpCallbackId);
        reader.GetEvent("onRightMouseDown", options.onRightMouseDownCallbackId);
        reader.GetEvent("onRightDoubleClick", options.onRightDoubleClickCallbackId);
        reader.GetEvent("onMiddleMouseUp", options.onMiddleMouseUpCallbackId);
        reader.GetEvent("onMiddleMouseDown", options.onMiddleMouseDownCallbackId);
        reader.GetEvent("onMiddleDoubleClick", options.onMiddleDoubleClickCallbackId);
        
        reader.GetEvent("onX1MouseUp", options.onX1MouseUpCallbackId);
        reader.GetEvent("onX1MouseDown", options.onX1MouseDownCallbackId);
        reader.GetEvent("onX1DoubleClick", options.onX1DoubleClickCallbackId);
        reader.GetEvent("onX2MouseUp", options.onX2MouseUpCallbackId);
        reader.GetEvent("onX2MouseDown", options.onX2MouseDownCallbackId);
        reader.GetEvent("onX2DoubleClick", options.onX2DoubleClickCallbackId);
        
        reader.GetEvent("onScrollUp", options.onScrollUpCallbackId);
        reader.GetEvent("onScrollDown", options.onScrollDownCallbackId);
        reader.GetEvent("onScrollLeft", options.onScrollLeftCallbackId);
        reader.GetEvent("onScrollRight", options.onScrollRightCallbackId);
        
        reader.GetEvent("onMouseOver", options.onMouseOverCallbackId);
        reader.GetEvent("onMouseLeave", options.onMouseLeaveCallbackId);
        
        // Tooltip properties
        reader.GetString("tooltipText", options.tooltipText);
        reader.GetString("tooltipTitle", options.tooltipTitle);
        reader.GetString("tooltipIcon", options.tooltipIcon);
        reader.GetInt("tooltipMaxWidth", options.tooltipMaxWidth);
        reader.GetInt("tooltipMaxHeight", options.tooltipMaxHeight);
        reader.GetBool("tooltipBalloon", options.tooltipBalloon);

        reader.GetBool("antiAlias", options.antialias);

        if (reader.GetFloatArray("transformMatrix", options.transformMatrix, 6)) {
            options.hasTransformMatrix = true;
        }
    }

    /*
    ** Parse ElementOptions from a Duktape object at the top of the stack.
    */

    void ParseImageOptions(duk_context* ctx, ImageOptions& options, const std::wstring& baseDir) {
        if (!duk_is_object(ctx, -1)) return;
        
        std::wstring finalBaseDir = baseDir.empty() ? PathUtils::GetWidgetsDir() : baseDir;

        // Parse base options first
        ParseElementOptionsInternal(ctx, options);
        PropertyReader reader(ctx);

        // Path
        if (reader.GetString("path", options.path)) {
            options.path = JSApi::ResolveScriptPath(ctx, options.path);
        }

        // Image specific
        std::wstring aspectStr;
        if (reader.GetString("preserveAspectRatio", aspectStr)) {
            if (aspectStr == L"preserve") options.preserveAspectRatio = IMAGE_ASPECT_PRESERVE;
            else if (aspectStr == L"crop") options.preserveAspectRatio = IMAGE_ASPECT_CROP;
            else if (aspectStr == L"stretch") options.preserveAspectRatio = IMAGE_ASPECT_STRETCH;
        }
        reader.GetBool("grayscale", options.grayscale);
        reader.GetBool("tile", options.tile);
        int alpha = 255;
        if (reader.GetInt("imageAlpha", alpha)) options.imageAlpha = (BYTE)alpha;

        std::wstring tintStr;
        if (reader.GetString("imageTint", tintStr)) {
             if (ColorUtil::ParseRGBA(tintStr, options.imageTint, options.imageTintAlpha)) {
                 options.hasImageTint = true;
             }
        }
        
        std::vector<float> matrixRaw;
        if (reader.GetFloatArray("colorMatrix", matrixRaw, 20)) {
            if (matrixRaw.size() >= 25) {
                // Convert 5x5 (25 numbers) to 5x4 (20 floats) by skipping the 5th column of each row
                options.colorMatrix.clear();
                for (int row = 0; row < 5; ++row) {
                    for (int col = 0; col < 4; ++col) {
                        options.colorMatrix.push_back(matrixRaw[row * 5 + col]);
                    }
                }
                options.hasColorMatrix = true;
            } else if (matrixRaw.size() >= 20) {
                options.colorMatrix = matrixRaw;
                options.hasColorMatrix = true;
            }
        }
    }

    /*
    ** Parse TextOptions from a Duktape object at the top of the stack.
    */
    void ParseTextOptions(duk_context* ctx, TextOptions& options, const std::wstring& baseDir) {
        if (!duk_is_object(ctx, -1)) return;
        
        std::wstring finalBaseDir = baseDir.empty() ? PathUtils::GetWidgetsDir() : baseDir;

        // Parse base options first
        ParseElementOptionsInternal(ctx, options);
        PropertyReader reader(ctx);

        reader.GetString("text", options.text);
        if (!reader.GetString("fontFace", options.fontFace)) options.fontFace = L"Arial";
        reader.GetInt("fontSize", options.fontSize);
        
        reader.GetColor("fontColor", options.fontColor, options.alpha);
        
        if (duk_get_prop_string(ctx, -1, "fontWeight")) {
            if (duk_is_number(ctx, -1)) {
                 options.fontWeight = duk_get_int(ctx, -1);
            } else if (duk_is_string(ctx, -1)) {
                std::wstring weightStr = Utils::ToWString(duk_get_string(ctx, -1));
                std::transform(weightStr.begin(), weightStr.end(), weightStr.begin(), ::towlower);
                
                if (weightStr == L"thin") options.fontWeight = 100;
                else if (weightStr == L"extralight" || weightStr == L"ultralight") options.fontWeight = 200;
                else if (weightStr == L"light") options.fontWeight = 300;
                else if (weightStr == L"normal" || weightStr == L"regular") options.fontWeight = 400;
                else if (weightStr == L"medium") options.fontWeight = 500;
                else if (weightStr == L"semibold" || weightStr == L"demibold") options.fontWeight = 600;
                else if (weightStr == L"bold") options.fontWeight = 700;
                else if (weightStr == L"extrabold" || weightStr == L"ultrabold") options.fontWeight = 800;
                else if (weightStr == L"black" || weightStr == L"heavy") options.fontWeight = 900;
            }
        }
        duk_pop(ctx);
        if (reader.GetString("fontPath", options.fontPath)) {
            options.fontPath = PathUtils::ResolvePath(options.fontPath, baseDir);
            Logging::Log(LogLevel::Debug, L"PropertyParser: Resolved fontPath: '%s'", options.fontPath.c_str());
        }
        
        std::wstring style;
        if (reader.GetString("fontStyle", style) && style == L"italic") options.italic = true;

        std::wstring alignStr;
        if (reader.GetString("textAlign", alignStr) || reader.GetString("align", alignStr)) {
            // Convert to lowercase for comparison
            std::transform(alignStr.begin(), alignStr.end(), alignStr.begin(), ::towlower);

            if (alignStr == L"left" || alignStr == L"lefttop") options.textAlign = TEXT_ALIGN_LEFT_TOP;
            else if (alignStr == L"center" || alignStr == L"centertop") options.textAlign = TEXT_ALIGN_CENTER_TOP;
            else if (alignStr == L"right" || alignStr == L"righttop") options.textAlign = TEXT_ALIGN_RIGHT_TOP;
            else if (alignStr == L"leftcenter") options.textAlign = TEXT_ALIGN_LEFT_CENTER;
            else if (alignStr == L"centercenter" || alignStr == L"middlecenter" || alignStr == L"middle") options.textAlign = TEXT_ALIGN_CENTER_CENTER;
            else if (alignStr == L"rightcenter") options.textAlign = TEXT_ALIGN_RIGHT_CENTER;
            else if (alignStr == L"leftbottom") options.textAlign = TEXT_ALIGN_LEFT_BOTTOM;
            else if (alignStr == L"centerbottom") options.textAlign = TEXT_ALIGN_CENTER_BOTTOM;
            else if (alignStr == L"rightbottom") options.textAlign = TEXT_ALIGN_RIGHT_BOTTOM;
        }
        
        std::wstring clipStr;
        if (reader.GetString("clipString", clipStr)) {
            if (clipStr == L"none") options.clip = TEXT_CLIP_NONE;
            else if (clipStr == L"on" || clipStr == L"clip") options.clip = TEXT_CLIP_ON;
            else if (clipStr == L"wrap") options.clip = TEXT_CLIP_WRAP;
            else if (clipStr == L"ellipsis") options.clip = TEXT_CLIP_ELLIPSIS;
        }

        if (duk_get_prop_string(ctx, -1, "fontShadow")) {
            options.shadows.clear();
            if (duk_is_array(ctx, -1)) {
                int len = (int)duk_get_length(ctx, -1);
                for (int i = 0; i < len; i++) {
                    duk_get_prop_index(ctx, -1, i);
                    TextShadow shadow;
                    if (reader.ParseShadow(shadow)) options.shadows.push_back(shadow);
                    duk_pop(ctx);
                }
            } else if (duk_is_object(ctx, -1)) {
                TextShadow shadow;
                if (reader.ParseShadow(shadow)) options.shadows.push_back(shadow);
            }
        }
        duk_pop(ctx);
    }

    /*
    ** Parse BarOptions from a Duktape object at the top of the stack.
    */
    void ParseBarOptions(duk_context* ctx, BarOptions& options, const std::wstring& baseDir) {
        if (!duk_is_object(ctx, -1)) return;

        // Parse base options first
        ParseElementOptionsInternal(ctx, options);
        PropertyReader reader(ctx);

        reader.GetFloat("value", options.value);
        
        std::wstring orientationStr;
        if (reader.GetString("orientation", orientationStr)) {
            if (orientationStr == L"horizontal") options.orientation = BAR_HORIZONTAL;
            else if (orientationStr == L"vertical") options.orientation = BAR_VERTICAL;
        } else {
            // Fallback to int for backward compatibility if needed, though we just implemented it
            int orient = 0;
            if (reader.GetInt("orientation", orient)) options.orientation = (BarOrientation)orient;
        }

        reader.GetInt("barCornerRadius", options.barCornerRadius);

        std::wstring barColorStr;
        if (reader.GetString("barColor", barColorStr)) {
            if (ColorUtil::ParseRGBA(barColorStr, options.barColor, options.barAlpha)) {
                options.hasBarColor = true;
            }
        }

        std::wstring barColor2Str;
        if (reader.GetString("barColor2", barColor2Str)) {
            if (ColorUtil::ParseRGBA(barColor2Str, options.barColor2, options.barAlpha2)) {
                options.hasBarGradient = true;
            }
        }
        reader.GetFloat("barGradientAngle", options.barGradientAngle);
    }

    /*
    ** Apply properties from a Duktape object to a Widget.
    */
    void ApplyWidgetProperties(duk_context* ctx, Widget* widget, const std::wstring& baseDir) {
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
        if (reader.GetString("zPos", zPosStr)) {
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
        if (reader.GetString("backgroundColor", bgColor)) {
            widget->SetBackgroundColor(bgColor);
        }

        // Visibility
        bool show;
        if (reader.GetBool("show", show)) {
            if (show) widget->Show();
            else widget->Hide();
        }

        // Boolean toggles
        bool draggable;
        if (reader.GetBool("draggable", draggable)) widget->SetDraggable(draggable);
        
        bool clickThrough;
        if (reader.GetBool("clickThrough", clickThrough)) widget->SetClickThrough(clickThrough);
        
        bool keepOnScreen;
        if (reader.GetBool("keepOnScreen", keepOnScreen)) widget->SetKeepOnScreen(keepOnScreen);
        
        bool snapEdges;
        if (reader.GetBool("snapEdges", snapEdges)) widget->SetSnapEdges(snapEdges);
    }

    /*
    ** Push a Widget's current properties as a JavaScript object onto the stack.
    */

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
        duk_push_string(ctx, zPosStr); duk_put_prop_string(ctx, -2, "zPos");
        
        duk_push_number(ctx, opt.windowOpacity / 255.0); duk_put_prop_string(ctx, -2, "opacity");
        duk_push_boolean(ctx, opt.draggable); duk_put_prop_string(ctx, -2, "draggable");
        duk_push_boolean(ctx, opt.clickThrough); duk_put_prop_string(ctx, -2, "clickThrough");
        duk_push_boolean(ctx, opt.keepOnScreen); duk_put_prop_string(ctx, -2, "keepOnScreen");
        duk_push_boolean(ctx, opt.snapEdges); duk_put_prop_string(ctx, -2, "snapEdges");
        duk_push_string(ctx, Utils::ToString(opt.backgroundColor).c_str()); duk_put_prop_string(ctx, -2, "backgroundColor");
        duk_push_boolean(ctx, opt.show); duk_put_prop_string(ctx, -2, "show");
    }

    /*
    ** Push an Element's current properties as a JavaScript object onto the stack.
    */

    void PushElementProperties(duk_context* ctx, Element* element) {
        if (!element) return;
        duk_push_object(ctx);
        
        duk_push_string(ctx, Utils::ToString(element->GetId()).c_str()); duk_put_prop_string(ctx, -2, "id");
        duk_push_int(ctx, element->GetX()); duk_put_prop_string(ctx, -2, "x");
        duk_push_int(ctx, element->GetY()); duk_put_prop_string(ctx, -2, "y");
        duk_push_int(ctx, element->GetWidth()); duk_put_prop_string(ctx, -2, "width");
        duk_push_int(ctx, element->GetHeight()); duk_put_prop_string(ctx, -2, "height");
        duk_push_number(ctx, element->GetRotate()); duk_put_prop_string(ctx, -2, "rotate");
        duk_push_boolean(ctx, element->GetAntiAlias()); duk_put_prop_string(ctx, -2, "antiAlias");

        // Background
        if (element->HasSolidColor()) {
            COLORREF c = element->GetSolidColor();
            BYTE a = element->GetSolidAlpha();
            std::wstring colorStr = ColorUtil::ToRGBAString(c, a);
            duk_push_string(ctx, Utils::ToString(colorStr).c_str()); duk_put_prop_string(ctx, -2, "solidColor");
        }
        duk_push_int(ctx, element->GetCornerRadius()); duk_put_prop_string(ctx, -2, "solidColorRadius");

        // Transform Matrix
        if (element->HasTransformMatrix()) {
            const float* matrix = element->GetTransformMatrix();
            duk_push_array(ctx);
            for (int i = 0; i < 6; i++) {
                duk_push_number(ctx, matrix[i]);
                duk_put_prop_index(ctx, -2, i);
            }
            duk_put_prop_string(ctx, -2, "transformMatrix");
        }

        // Gradient
        if (element->HasGradient()) {
            COLORREF c2 = element->GetSolidColor2();
            BYTE a2 = element->GetSolidAlpha2();
            std::wstring color2Str = ColorUtil::ToRGBAString(c2, a2);
            duk_push_string(ctx, Utils::ToString(color2Str).c_str()); duk_put_prop_string(ctx, -2, "solidColor2");
            duk_push_number(ctx, element->GetGradientAngle()); duk_put_prop_string(ctx, -2, "gradientAngle");
        }

        // Bevel
        int bt = element->GetBevelType();
        const char* bevStr = "none";
        switch (bt) {
            case 1: bevStr = "raised"; break;
            case 2: bevStr = "sunken"; break;
            case 3: bevStr = "emboss"; break;
            case 4: bevStr = "pillow"; break;
        }
        duk_push_string(ctx, bevStr); duk_put_prop_string(ctx, -2, "bevelType");

        if (bt > 0) {
            duk_push_int(ctx, element->GetBevelWidth()); duk_put_prop_string(ctx, -2, "bevelWidth");
            
            std::wstring bc1 = ColorUtil::ToRGBAString(element->GetBevelColor(), element->GetBevelAlpha());
            duk_push_string(ctx, Utils::ToString(bc1).c_str()); duk_put_prop_string(ctx, -2, "bevelColor");
            
            std::wstring bc2 = ColorUtil::ToRGBAString(element->GetBevelColor2(), element->GetBevelAlpha2());
            duk_push_string(ctx, Utils::ToString(bc2).c_str()); duk_put_prop_string(ctx, -2, "bevelColor2");
        }

        // Padding
        duk_push_array(ctx);
        duk_push_int(ctx, element->GetPaddingLeft()); duk_put_prop_index(ctx, -2, 0);
        duk_push_int(ctx, element->GetPaddingTop()); duk_put_prop_index(ctx, -2, 1);
        duk_push_int(ctx, element->GetPaddingRight()); duk_put_prop_index(ctx, -2, 2);
        duk_push_int(ctx, element->GetPaddingBottom()); duk_put_prop_index(ctx, -2, 3);
        duk_put_prop_string(ctx, -2, "padding");

        // Type Specific
        if (element->GetType() == ELEMENT_TEXT) {
            TextElement* t = static_cast<TextElement*>(element);
            duk_push_string(ctx, Utils::ToString(t->GetText()).c_str()); duk_put_prop_string(ctx, -2, "text");
            duk_push_string(ctx, Utils::ToString(t->GetFontFace()).c_str()); duk_put_prop_string(ctx, -2, "fontFace");
            duk_push_int(ctx, t->GetFontSize()); duk_put_prop_string(ctx, -2, "fontSize");
            
            std::wstring fc = ColorUtil::ToRGBAString(t->GetFontColor(), t->GetFontAlpha());
            duk_push_string(ctx, Utils::ToString(fc).c_str()); duk_put_prop_string(ctx, -2, "fontColor");
            
            duk_push_int(ctx, t->GetFontWeight()); duk_put_prop_string(ctx, -2, "fontWeight");
            duk_push_boolean(ctx, t->IsItalic()); duk_put_prop_string(ctx, -2, "italic");
            
            // Align string mapping (needs inverse)
            const char* alStr = "lefttop";
            switch(t->GetTextAlign()) {
                case TEXT_ALIGN_LEFT_TOP: alStr = "lefttop"; break;
                case TEXT_ALIGN_CENTER_TOP: alStr = "centertop"; break;
                case TEXT_ALIGN_RIGHT_TOP: alStr = "righttop"; break;
                case TEXT_ALIGN_LEFT_CENTER: alStr = "leftcenter"; break;
                case TEXT_ALIGN_CENTER_CENTER: alStr = "centercenter"; break;
                case TEXT_ALIGN_RIGHT_CENTER: alStr = "rightcenter"; break;
                case TEXT_ALIGN_LEFT_BOTTOM: alStr = "leftbottom"; break;
                case TEXT_ALIGN_CENTER_BOTTOM: alStr = "centerbottom"; break;
                case TEXT_ALIGN_RIGHT_BOTTOM: alStr = "rightbottom"; break;
            }
            duk_push_string(ctx, alStr); duk_put_prop_string(ctx, -2, "textAlign");
            const char* clipStr = "none";
            switch (t->GetClipString()) {
                case TEXT_CLIP_ON: clipStr = "clip"; break;
                case TEXT_CLIP_ELLIPSIS: clipStr = "ellipsis"; break;
            }
            duk_push_string(ctx, clipStr); duk_put_prop_string(ctx, -2, "clipString");

            const auto& shadows = t->GetShadows();
            if (!shadows.empty()) {
                duk_push_array(ctx);
                for (int i = 0; i < (int)shadows.size(); i++) {
                    duk_push_object(ctx);
                    duk_push_number(ctx, shadows[i].offsetX); duk_put_prop_string(ctx, -2, "x");
                    duk_push_number(ctx, shadows[i].offsetY); duk_put_prop_string(ctx, -2, "y");
                    duk_push_number(ctx, shadows[i].blur); duk_put_prop_string(ctx, -2, "blur");
                    duk_push_string(ctx, Utils::ToString(ColorUtil::ToRGBAString(shadows[i].color, shadows[i].alpha)).c_str());
                    duk_put_prop_string(ctx, -2, "color");
                    duk_put_prop_index(ctx, -2, i);
                }
                duk_put_prop_string(ctx, -2, "fontShadow");
            }
            
            if (!t->GetFontPath().empty()) {
                duk_push_string(ctx, Utils::ToString(t->GetFontPath()).c_str()); duk_put_prop_string(ctx, -2, "fontPath");
            }

            
        } else if (element->GetType() == ELEMENT_IMAGE) {
            ImageElement* img = static_cast<ImageElement*>(element);
            duk_push_string(ctx, Utils::ToString(img->GetImagePath()).c_str()); duk_put_prop_string(ctx, -2, "path");
            
            const char* aspectStr = "stretch";
            switch (img->GetPreserveAspectRatio()) {
                case IMAGE_ASPECT_PRESERVE: aspectStr = "preserve"; break;
                case IMAGE_ASPECT_CROP:     aspectStr = "crop";     break;
            }
            duk_push_string(ctx, aspectStr); duk_put_prop_string(ctx, -2, "preserveAspectRatio");
            duk_push_boolean(ctx, img->IsGrayscale()); duk_put_prop_string(ctx, -2, "grayscale");
            duk_push_boolean(ctx, img->IsTile()); duk_put_prop_string(ctx, -2, "tile");
            duk_push_int(ctx, img->GetImageAlpha()); duk_put_prop_string(ctx, -2, "imageAlpha");
            
            if (img->HasImageTint()) {
                std::wstring itStr = ColorUtil::ToRGBAString(img->GetImageTint(), img->GetImageTintAlpha());
                duk_push_string(ctx, Utils::ToString(itStr).c_str()); duk_put_prop_string(ctx, -2, "imageTint");
            }
        } else if (element->GetType() == ELEMENT_BAR) {
            BarElement* bar = static_cast<BarElement*>(element);
            duk_push_number(ctx, bar->GetValue()); duk_put_prop_string(ctx, -2, "value");
            
            const char* orientStr = (bar->GetOrientation() == BAR_VERTICAL) ? "vertical" : "horizontal";
            duk_push_string(ctx, orientStr); duk_put_prop_string(ctx, -2, "orientation");
            
            duk_push_int(ctx, bar->GetBarCornerRadius()); duk_put_prop_string(ctx, -2, "barCornerRadius");
            duk_push_number(ctx, bar->GetBarGradientAngle()); duk_put_prop_string(ctx, -2, "barGradientAngle");
        }
    }


    void ParseElementOptions(duk_context* ctx, Element* element) {
        if (!element || !duk_is_object(ctx, -1)) return;
        
        ElementOptions options;
        PropertyReader reader(ctx);
        ParseElementOptionsInternal(ctx, options);
        ApplyElementOptions(element, options);
    }

    /*
    ** Apply properties from an ElementOptions struct to an Element instance.
    */

    void ApplyElementOptions(Element* element, const ElementOptions& options) {
        if (!element) return;
        
        element->SetPosition(options.x, options.y);
        if (options.width > 0 || options.height > 0) {
            element->SetSize(options.width, options.height);
        }

        if (options.hasSolidColor) {
            element->SetSolidColor(options.solidColor, options.solidAlpha);
        }
        element->SetCornerRadius(options.solidColorRadius);
        
        if (options.hasGradient) {
            element->SetGradient(options.solidColor2, options.solidAlpha2, options.gradientAngle);
        }

        if (options.bevelType > 0) {
            element->SetBevel(options.bevelType, options.bevelWidth, options.bevelColor, options.bevelAlpha, options.bevelColor2, options.bevelAlpha2);
        } else if (options.bevelType == 0) {
            element->SetBevel(0, 0, 0, 0, 0, 0);
        }
        
        element->SetRotate(options.rotate);
        element->SetPadding(options.paddingLeft, options.paddingTop, options.paddingRight, options.paddingBottom);

        // Mouse Actions (Directly overwrite if not empty in options)
        if (options.onLeftMouseUpCallbackId != -1) element->m_OnLeftMouseUpCallbackId = options.onLeftMouseUpCallbackId;
        if (options.onLeftMouseDownCallbackId != -1) element->m_OnLeftMouseDownCallbackId = options.onLeftMouseDownCallbackId;
        if (options.onLeftDoubleClickCallbackId != -1) element->m_OnLeftDoubleClickCallbackId = options.onLeftDoubleClickCallbackId;
        if (options.onRightMouseUpCallbackId != -1) element->m_OnRightMouseUpCallbackId = options.onRightMouseUpCallbackId;
        if (options.onRightMouseDownCallbackId != -1) element->m_OnRightMouseDownCallbackId = options.onRightMouseDownCallbackId;
        if (options.onRightDoubleClickCallbackId != -1) element->m_OnRightDoubleClickCallbackId = options.onRightDoubleClickCallbackId;
        if (options.onMiddleMouseUpCallbackId != -1) element->m_OnMiddleMouseUpCallbackId = options.onMiddleMouseUpCallbackId;
        if (options.onMiddleMouseDownCallbackId != -1) element->m_OnMiddleMouseDownCallbackId = options.onMiddleMouseDownCallbackId;
        if (options.onMiddleDoubleClickCallbackId != -1) element->m_OnMiddleDoubleClickCallbackId = options.onMiddleDoubleClickCallbackId;
        if (options.onX1MouseUpCallbackId != -1) element->m_OnX1MouseUpCallbackId = options.onX1MouseUpCallbackId;
        if (options.onX1MouseDownCallbackId != -1) element->m_OnX1MouseDownCallbackId = options.onX1MouseDownCallbackId;
        if (options.onX1DoubleClickCallbackId != -1) element->m_OnX1DoubleClickCallbackId = options.onX1DoubleClickCallbackId;
        if (options.onX2MouseUpCallbackId != -1) element->m_OnX2MouseUpCallbackId = options.onX2MouseUpCallbackId;
        if (options.onX2MouseDownCallbackId != -1) element->m_OnX2MouseDownCallbackId = options.onX2MouseDownCallbackId;
        if (options.onX2DoubleClickCallbackId != -1) element->m_OnX2DoubleClickCallbackId = options.onX2DoubleClickCallbackId;
        if (options.onScrollUpCallbackId != -1) element->m_OnScrollUpCallbackId = options.onScrollUpCallbackId;
        if (options.onScrollDownCallbackId != -1) element->m_OnScrollDownCallbackId = options.onScrollDownCallbackId;
        if (options.onScrollLeftCallbackId != -1) element->m_OnScrollLeftCallbackId = options.onScrollLeftCallbackId;
        if (options.onScrollRightCallbackId != -1) element->m_OnScrollRightCallbackId = options.onScrollRightCallbackId;
        if (options.onMouseOverCallbackId != -1) element->m_OnMouseOverCallbackId = options.onMouseOverCallbackId;
        if (options.onMouseLeaveCallbackId != -1) element->m_OnMouseLeaveCallbackId = options.onMouseLeaveCallbackId;
        
        // Tooltip properties
        if (!options.tooltipText.empty()) {
            element->SetToolTip(options.tooltipText, options.tooltipTitle, options.tooltipIcon, options.tooltipMaxWidth, options.tooltipMaxHeight, options.tooltipBalloon);
        }

        if (options.hasTransformMatrix) {
            element->SetTransformMatrix(options.transformMatrix.data());
        }

        element->SetAntiAlias(options.antialias);
    }

    /*
    ** Apply properties from an ImageOptions struct to an ImageElement instance.
    */

    void ApplyImageOptions(ImageElement* element, const ImageOptions& options) {
        if (!element) return;
        ApplyElementOptions(element, options);
        
        if (!options.path.empty()) element->UpdateImage(options.path);
        element->SetPreserveAspectRatio(options.preserveAspectRatio);
        element->SetGrayscale(options.grayscale);
        element->SetTile(options.tile);
        element->SetImageAlpha(options.imageAlpha);
        
        if (options.hasImageTint) {
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        }
    }

    /*
    ** Apply properties from a TextOptions struct to a TextElement instance.
    */
    
    void ApplyTextOptions(TextElement* element, const TextOptions& options) {
        if (!element) return;
        ApplyElementOptions(element, options);
        
        element->SetText(options.text);
        element->SetFontFace(options.fontFace);
        element->SetFontSize(options.fontSize);
        element->SetFontColor(options.fontColor, options.alpha);
        element->SetFontWeight(options.fontWeight);
        element->SetItalic(options.italic);
        element->SetTextAlign(options.textAlign);
        element->SetClip(options.clip);
        element->SetFontPath(options.fontPath);
        element->SetShadows(options.shadows);
    }

    /*
    ** Apply properties from a BarOptions struct to a BarElement instance.
    */
    void ApplyBarOptions(BarElement* element, const BarOptions& options) {
        if (!element) return;
        ApplyElementOptions(element, options);

        element->SetValue(options.value);
        element->SetOrientation(options.orientation);
        element->SetBarCornerRadius(options.barCornerRadius);

        if (options.hasBarColor) {
            element->SetBarColor(options.barColor, options.barAlpha);
        }
        if (options.hasBarGradient) {
            element->SetBarColor2(options.barColor2, options.barAlpha2, options.barGradientAngle);
        }
    }

    void PreFillElementOptions(ElementOptions& options, Element* element) {
        if (!element) return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        
        // Subtract padding to get internal dimensions
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;
        
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();

        options.hasSolidColor = element->HasSolidColor();
        options.solidColor = element->GetSolidColor();
        options.solidAlpha = element->GetSolidAlpha();
        options.solidColorRadius = element->GetCornerRadius();

        options.hasGradient = element->HasGradient();
        options.solidColor2 = element->GetSolidColor2();
        options.solidAlpha2 = element->GetSolidAlpha2();
        options.gradientAngle = element->GetGradientAngle();

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        options.onLeftMouseUpCallbackId = element->m_OnLeftMouseUpCallbackId;
        options.onLeftMouseDownCallbackId = element->m_OnLeftMouseDownCallbackId;
        options.onLeftDoubleClickCallbackId = element->m_OnLeftDoubleClickCallbackId;
        options.onRightMouseUpCallbackId = element->m_OnRightMouseUpCallbackId;
        options.onRightMouseDownCallbackId = element->m_OnRightMouseDownCallbackId;
        options.onRightDoubleClickCallbackId = element->m_OnRightDoubleClickCallbackId;
        options.onMiddleMouseUpCallbackId = element->m_OnMiddleMouseUpCallbackId;
        options.onMiddleMouseDownCallbackId = element->m_OnMiddleMouseDownCallbackId;
        options.onMiddleDoubleClickCallbackId = element->m_OnMiddleDoubleClickCallbackId;
        options.onX1MouseUpCallbackId = element->m_OnX1MouseUpCallbackId;
        options.onX1MouseDownCallbackId = element->m_OnX1MouseDownCallbackId;
        options.onX1DoubleClickCallbackId = element->m_OnX1DoubleClickCallbackId;
        options.onX2MouseUpCallbackId = element->m_OnX2MouseUpCallbackId;
        options.onX2MouseDownCallbackId = element->m_OnX2MouseDownCallbackId;
        options.onX2DoubleClickCallbackId = element->m_OnX2DoubleClickCallbackId;
        options.onScrollUpCallbackId = element->m_OnScrollUpCallbackId;
        options.onScrollDownCallbackId = element->m_OnScrollDownCallbackId;
        options.onScrollLeftCallbackId = element->m_OnScrollLeftCallbackId;
        options.onScrollRightCallbackId = element->m_OnScrollRightCallbackId;
        options.onMouseOverCallbackId = element->m_OnMouseOverCallbackId;
        options.onMouseLeaveCallbackId = element->m_OnMouseLeaveCallbackId;
    }

    void PreFillTextOptions(TextOptions& options, TextElement* element) {
        if (!element) return;
        PreFillElementOptions(options, element);
        options.text = element->GetText();
        options.fontFace = element->GetFontFace();
        options.fontSize = element->GetFontSize();
        options.fontColor = element->GetFontColor();
        options.alpha = element->GetFontAlpha();
        options.fontWeight = element->GetFontWeight();
        options.italic = element->IsItalic();
        options.textAlign = element->GetTextAlign();
        options.clip = element->GetClipString();
        options.fontPath = element->GetFontPath();
        options.shadows = element->GetShadows();
    }

    void PreFillImageOptions(ImageOptions& options, ImageElement* element) {
        if (!element) return;
        PreFillElementOptions(options, element);
        options.path = element->GetImagePath();
        options.preserveAspectRatio = element->GetPreserveAspectRatio();
        options.imageAlpha = element->GetImageAlpha();
        options.grayscale = element->IsGrayscale();
        options.tile = element->IsTile();
        if (element->HasImageTint()) {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasTransformMatrix()) {
            options.hasTransformMatrix = true;
            const float* matrix = element->GetTransformMatrix();
            options.transformMatrix.assign(matrix, matrix + 6);
        }
    }

    void PreFillBarOptions(BarOptions& options, BarElement* element) {
        if (!element) return;
        PreFillElementOptions(options, element);
        options.value = element->GetValue();
        options.orientation = element->GetOrientation();
        options.barCornerRadius = element->GetBarCornerRadius();
        options.hasBarColor = element->HasBarColor();
        options.barColor = element->GetBarColor();
        options.barAlpha = element->GetBarAlpha();
        options.hasBarGradient = element->HasBarGradient();
        options.barColor2 = element->GetBarColor2();
        options.barAlpha2 = element->GetBarAlpha2();
        options.barGradientAngle = element->GetBarGradientAngle();
    }
}
