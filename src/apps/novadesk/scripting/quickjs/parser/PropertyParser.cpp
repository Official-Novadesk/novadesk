#include "PropertyParser.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <filesystem>

#include "../../../shared/ColorUtil.h"
#include "../../../shared/Logging.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../../render/RotatorElement.h"
#include "../../render/AreaGraphElement.h"
#include "../../render/HistogramElement.h"
#include "../engine/JSEngine.h"

namespace PropertyParser
{
    namespace
    {
        std::vector<std::wstring> SplitByComma(const std::wstring &s)
        {
            std::vector<std::wstring> parts;
            int depth = 0;
            size_t last = 0;
            for (size_t i = 0; i < s.length(); i++)
            {
                if (s[i] == L'(')
                    depth++;
                else if (s[i] == L')')
                    depth--;
                else if (s[i] == L',' && depth == 0)
                {
                    parts.push_back(s.substr(last, i - last));
                    last = i + 1;
                }
            }
            parts.push_back(s.substr(last));
            for (auto &p : parts)
            {
                p.erase(0, p.find_first_not_of(L' '));
                p.erase(p.find_last_not_of(L' ') + 1);
            }
            return parts;
        }

        JSValue GetGlobalProperty(JSContext *ctx, const char *key)
        {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue v = JS_GetPropertyStr(ctx, global, key);
            JS_FreeValue(ctx, global);
            return v;
        }

        std::wstring GetStringProp(JSContext *ctx, JSValueConst obj, const char *key)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return L"";
            }
            const char *s = JS_ToCString(ctx, v);
            std::wstring out;
            if (s)
            {
                out = Utils::ToWString(s);
                JS_FreeCString(ctx, s);
            }
            JS_FreeValue(ctx, v);
            return out;
        }

        bool GetIntProp(JSContext *ctx, JSValueConst obj, const char *key, int &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            int32_t i = 0;
            bool ok = JS_ToInt32(ctx, &i, v) == 0;
            JS_FreeValue(ctx, v);
            if (ok)
                out = static_cast<int>(i);
            return ok;
        }

        bool GetFloatProp(JSContext *ctx, JSValueConst obj, const char *key, float &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            double d = 0.0;
            bool ok = JS_ToFloat64(ctx, &d, v) == 0;
            JS_FreeValue(ctx, v);
            if (ok)
                out = static_cast<float>(d);
            return ok;
        }

        bool GetBoolProp(JSContext *ctx, JSValueConst obj, const char *key, bool &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            int b = JS_ToBool(ctx, v);
            JS_FreeValue(ctx, v);
            if (b < 0)
                return false;
            out = b != 0;
            return true;
        }

        bool GetFloatArrayProp(JSContext *ctx, JSValueConst obj, const char *key, std::vector<float> &out, int minSize)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || !JS_IsArray(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }

            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, v, "length");
            if (JS_ToUint32(ctx, &len, lenV) != 0)
            {
                JS_FreeValue(ctx, lenV);
                JS_FreeValue(ctx, v);
                return false;
            }
            JS_FreeValue(ctx, lenV);

            std::vector<float> tmp;
            tmp.reserve(len);
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue iv = JS_GetPropertyUint32(ctx, v, i);
                double d = 0.0;
                if (JS_ToFloat64(ctx, &d, iv) == 0)
                {
                    tmp.push_back(static_cast<float>(d));
                }
                JS_FreeValue(ctx, iv);
            }
            JS_FreeValue(ctx, v);

            if (static_cast<int>(tmp.size()) < minSize)
                return false;
            out = std::move(tmp);
            return true;
        }

        bool GetEventCallbackProp(JSContext *ctx, JSValueConst obj, const char *key, int &outId)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            if (!JS_IsFunction(ctx, v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            outId = JSEngine::RegisterEventCallback(ctx, v);
            JS_FreeValue(ctx, v);
            return outId >= 0;
        }

        void ParseGradientOrColor(const std::wstring &v, COLORREF &color, BYTE &alpha, GradientInfo &gradient, bool &hasColor)
        {
            if (v.empty())
                return;
            GradientInfo parsed;
            if (ParseGradientString(v, parsed))
            {
                gradient = parsed;
                hasColor = true;
                return;
            }
            if (ColorUtil::ParseRGBA(v, color, alpha))
            {
                hasColor = true;
            }
        }

        D2D1_COMBINE_MODE ParseCombineMode(const std::wstring &s)
        {
            if (s == L"intersect")
                return D2D1_COMBINE_MODE_INTERSECT;
            if (s == L"exclude")
                return D2D1_COMBINE_MODE_EXCLUDE;
            if (s == L"xor")
                return D2D1_COMBINE_MODE_XOR;
            return D2D1_COMBINE_MODE_UNION;
        }

        bool GetFloatArrayPropAllowEmpty(JSContext *ctx, JSValueConst obj, const char *key, std::vector<float> &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || !JS_IsArray(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }

            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, v, "length");
            if (JS_ToUint32(ctx, &len, lenV) != 0)
            {
                JS_FreeValue(ctx, lenV);
                JS_FreeValue(ctx, v);
                return false;
            }
            JS_FreeValue(ctx, lenV);

            std::vector<float> tmp;
            tmp.reserve(len);
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue iv = JS_GetPropertyUint32(ctx, v, i);
                double d = 0.0;
                if (JS_ToFloat64(ctx, &d, iv) == 0)
                {
                    tmp.push_back(static_cast<float>(d));
                }
                JS_FreeValue(ctx, iv);
            }

            JS_FreeValue(ctx, v);
            out = std::move(tmp);
            return true;
        }

        void ParsePrefixedGeneralImageOptions(
            JSContext *ctx,
            JSValueConst obj,
            const std::string &prefix,
            GeneralImageOptions &out)
        {
            out.imageFlip = IMAGE_FLIP_NONE;
            out.hasImageCrop = false;
            out.useExifOrientation = false;
            out.imageAlpha = 255;
            out.grayscale = false;
            out.hasColorMatrix = false;
            out.hasImageTint = false;

            std::wstring imageFlip = GetStringProp(ctx, obj, (prefix + "ImageFlip").c_str());
            std::transform(imageFlip.begin(), imageFlip.end(), imageFlip.begin(), ::towlower);
            if (imageFlip == L"horizontal")
                out.imageFlip = IMAGE_FLIP_HORIZONTAL;
            else if (imageFlip == L"vertical")
                out.imageFlip = IMAGE_FLIP_VERTICAL;
            else if (imageFlip == L"both")
                out.imageFlip = IMAGE_FLIP_BOTH;
            else if (imageFlip == L"none")
                out.imageFlip = IMAGE_FLIP_NONE;

            std::vector<float> imageCrop;
            if (GetFloatArrayProp(ctx, obj, (prefix + "ImageCrop").c_str(), imageCrop, 4))
            {
                out.hasImageCrop = true;
                out.imageCropX = imageCrop[0];
                out.imageCropY = imageCrop[1];
                out.imageCropW = imageCrop[2];
                out.imageCropH = imageCrop[3];
                out.imageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
                if (imageCrop.size() >= 5)
                {
                    int origin = static_cast<int>(imageCrop[4]);
                    if (origin < static_cast<int>(IMAGE_CROP_ORIGIN_TOP_LEFT))
                        origin = static_cast<int>(IMAGE_CROP_ORIGIN_TOP_LEFT);
                    if (origin > static_cast<int>(IMAGE_CROP_ORIGIN_CENTER))
                        origin = static_cast<int>(IMAGE_CROP_ORIGIN_CENTER);
                    out.imageCropOrigin = static_cast<ImageCropOrigin>(origin);
                }
            }

            GetBoolProp(ctx, obj, (prefix + "GreyScale").c_str(), out.grayscale);
            GetBoolProp(ctx, obj, (prefix + "UseExifOrientation").c_str(), out.useExifOrientation);

            int alpha = 255;
            if (GetIntProp(ctx, obj, (prefix + "ImageAlpha").c_str(), alpha))
                out.imageAlpha = static_cast<BYTE>(alpha);

            std::wstring tint = GetStringProp(ctx, obj, (prefix + "ImageTint").c_str());
            if (!tint.empty() && ColorUtil::ParseRGBA(tint, out.imageTint, out.imageTintAlpha))
            {
                out.hasImageTint = true;
            }

            std::vector<float> colorMatrix;
            if (GetFloatArrayProp(ctx, obj, (prefix + "ColorMatrix").c_str(), colorMatrix, 20))
            {
                for (int i = 0; i < 20; ++i)
                    out.colorMatrix[i] = colorMatrix[(size_t)i];
                out.hasColorMatrix = true;
            }
            else
            {
                bool hasComponents = false;
                for (int i = 1; i <= 20; ++i)
                {
                    float val = 0.0f;
                    if (GetFloatProp(ctx, obj, (prefix + "ColorMatrix" + std::to_string(i)).c_str(), val))
                    {
                        out.colorMatrix[(size_t)(i - 1)] = val;
                        hasComponents = true;
                    }
                }
                out.hasColorMatrix = hasComponents;
            }
        }
    } // namespace

    bool ParseGradientString(const std::wstring &str, GradientInfo &out)
    {
        if (str.empty())
            return false;
        std::wstring s = str;
        s.erase(0, s.find_first_not_of(L' '));
        s.erase(s.find_last_not_of(L' ') + 1);

        std::wstring lowerS = s;
        std::transform(lowerS.begin(), lowerS.end(), lowerS.begin(), ::towlower);

        if (lowerS.find(L"lineargradient(") == 0)
            out.type = GRADIENT_LINEAR;
        else if (lowerS.find(L"radialgradient(") == 0)
            out.type = GRADIENT_RADIAL;
        else
            return false;

        size_t start = lowerS.find(L'(') + 1;
        size_t end = lowerS.find_last_of(L')');
        if (end == std::wstring::npos || end <= start)
            return false;

        std::wstring content = s.substr(start, end - start);
        std::vector<std::wstring> parts = SplitByComma(content);
        if (parts.empty())
            return false;

        int colorStartIndex = 0;
        if (out.type == GRADIENT_LINEAR)
        {
            std::wstring dir = parts[0];
            std::transform(dir.begin(), dir.end(), dir.begin(), ::towlower);
            dir.erase(std::remove_if(dir.begin(), dir.end(), isspace), dir.end());

            if (!dir.empty() && (iswdigit(dir[0]) || dir[0] == L'-' || dir[0] == L'.'))
            {
                try
                {
                    size_t pos = 0;
                    out.angle = std::stof(dir, &pos);
                    if (pos > 0)
                        colorStartIndex = 1;
                }
                catch (...)
                {
                }
            }
        }
        else
        {
            std::wstring shape = parts[0];
            std::transform(shape.begin(), shape.end(), shape.begin(), ::towlower);
            shape.erase(std::remove_if(shape.begin(), shape.end(), isspace), shape.end());

            if (shape == L"circle" || shape == L"ellipse")
            {
                out.shape = shape;
                colorStartIndex = 1;
            }
        }

        out.stops.clear();
        for (size_t i = colorStartIndex; i < parts.size(); i++)
        {
            GradientStop stop;
            if (ColorUtil::ParseRGBA(parts[i], stop.color, stop.alpha))
            {
                out.stops.push_back(stop);
            }
        }

        if (out.stops.size() < 2)
            return false;

        for (size_t i = 0; i < out.stops.size(); i++)
        {
            out.stops[i].position = (float)i / (out.stops.size() - 1);
        }

        return true;
    }

    D2D1_CAP_STYLE GetCapStyle(const std::wstring &str)
    {
        if (str == L"Round")
            return D2D1_CAP_STYLE_ROUND;
        if (str == L"Square")
            return D2D1_CAP_STYLE_SQUARE;
        if (str == L"Triangle")
            return D2D1_CAP_STYLE_TRIANGLE;
        return D2D1_CAP_STYLE_FLAT;
    }

    D2D1_LINE_JOIN GetLineJoin(const std::wstring &str)
    {
        if (str == L"Bevel")
            return D2D1_LINE_JOIN_BEVEL;
        if (str == L"Round")
            return D2D1_LINE_JOIN_ROUND;
        if (str == L"MiterOrBevel")
            return D2D1_LINE_JOIN_MITER_OR_BEVEL;
        return D2D1_LINE_JOIN_MITER;
    }

    PropertyReader::PropertyReader(duk_context *ctx)
        : m_Ctx(ctx)
    {
    }

    bool PropertyReader::GetString(const char *key, std::wstring &outStr)
    {
        if (!m_Ctx || !key)
            return false;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue v = GetGlobalProperty(ctx, key);
        if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
        {
            JS_FreeValue(ctx, v);
            return false;
        }
        const char *s = JS_ToCString(ctx, v);
        if (!s)
        {
            JS_FreeValue(ctx, v);
            return false;
        }
        outStr = Utils::ToWString(s);
        JS_FreeCString(ctx, s);
        JS_FreeValue(ctx, v);
        return true;
    }

    bool PropertyReader::GetInt(const char *key, int &outInt)
    {
        if (!m_Ctx || !key)
            return false;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue v = GetGlobalProperty(ctx, key);
        if (JS_IsException(v))
        {
            JS_FreeValue(ctx, v);
            return false;
        }
        int32_t tmp = 0;
        bool ok = (JS_ToInt32(ctx, &tmp, v) == 0);
        JS_FreeValue(ctx, v);
        if (!ok)
            return false;
        outInt = static_cast<int>(tmp);
        return true;
    }

    bool PropertyReader::GetFloat(const char *key, float &outFloat)
    {
        if (!m_Ctx || !key)
            return false;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue v = GetGlobalProperty(ctx, key);
        if (JS_IsException(v))
        {
            JS_FreeValue(ctx, v);
            return false;
        }
        double tmp = 0.0;
        bool ok = (JS_ToFloat64(ctx, &tmp, v) == 0);
        JS_FreeValue(ctx, v);
        if (!ok)
            return false;
        outFloat = static_cast<float>(tmp);
        return true;
    }

    bool PropertyReader::GetBool(const char *key, bool &outBool)
    {
        if (!m_Ctx || !key)
            return false;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue v = GetGlobalProperty(ctx, key);
        if (JS_IsException(v))
        {
            JS_FreeValue(ctx, v);
            return false;
        }
        int b = JS_ToBool(ctx, v);
        JS_FreeValue(ctx, v);
        if (b < 0)
            return false;
        outBool = (b != 0);
        return true;
    }

    bool PropertyReader::GetColor(const char *key, COLORREF &outColor, BYTE &outAlpha)
    {
        std::wstring colorStr;
        return GetString(key, colorStr) && ColorUtil::ParseRGBA(colorStr, outColor, outAlpha);
    }

    bool PropertyReader::GetGradientOrColor(const char *key, COLORREF &outColor, BYTE &outAlpha, GradientInfo &outGradient)
    {
        std::wstring colorStr;
        if (!GetString(key, colorStr))
            return false;
        if (ParseGradientString(colorStr, outGradient))
            return true;
        if (ColorUtil::ParseRGBA(colorStr, outColor, outAlpha))
        {
            outGradient.type = GRADIENT_NONE;
            outGradient.stops.clear();
            outGradient.angle = 0.0f;
            outGradient.shape.clear();
            return true;
        }
        return false;
    }

    bool PropertyReader::GetFloatArray(const char *key, std::vector<float> &outArray, int minSize)
    {
        if (!m_Ctx || !key)
            return false;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue arr = GetGlobalProperty(ctx, key);
        if (JS_IsException(arr) || !JS_IsArray(arr))
        {
            JS_FreeValue(ctx, arr);
            return false;
        }
        uint32_t len = 0;
        JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
        bool okLen = (JS_ToUint32(ctx, &len, lenV) == 0);
        JS_FreeValue(ctx, lenV);
        if (!okLen || static_cast<int>(len) < minSize)
        {
            JS_FreeValue(ctx, arr);
            return false;
        }

        std::vector<float> out;
        out.reserve(len);
        for (uint32_t i = 0; i < len; ++i)
        {
            JSValue iv = JS_GetPropertyUint32(ctx, arr, i);
            double n = 0.0;
            bool ok = (JS_ToFloat64(ctx, &n, iv) == 0);
            JS_FreeValue(ctx, iv);
            if (!ok)
            {
                JS_FreeValue(ctx, arr);
                return false;
            }
            out.push_back(static_cast<float>(n));
        }
        JS_FreeValue(ctx, arr);
        outArray = std::move(out);
        return true;
    }

    void PropertyReader::GetEvent(const char *key, int &outId)
    {
        if (!m_Ctx || !key)
            return;
        JSContext *ctx = reinterpret_cast<JSContext *>(m_Ctx);
        JSValue fn = GetGlobalProperty(ctx, key);
        if (!JS_IsException(fn) && JS_IsFunction(ctx, fn))
        {
            outId = JSEngine::RegisterEventCallback(ctx, fn);
        }
        JS_FreeValue(ctx, fn);
    }

    bool PropertyReader::ParseShadow(TextShadow &shadow)
    {
        if (!m_Ctx)
            return false;
        float f = 0.0f;
        if (GetFloat("x", f))
            shadow.offsetX = f;
        if (GetFloat("y", f))
            shadow.offsetY = f;
        if (GetFloat("blur", f))
            shadow.blur = f;

        std::wstring colorStr;
        if (GetString("color", colorStr))
        {
            ColorUtil::ParseRGBA(colorStr, shadow.color, shadow.alpha);
        }
        return true;
    }

    void ParseElementOptions(JSContext *ctx, JSValueConst obj, ElementOptions &options, const std::wstring &baseDir)
    {
        if (!JS_IsObject(obj))
            return;

        options.id = GetStringProp(ctx, obj, "id");
        GetIntProp(ctx, obj, "x", options.x);
        GetIntProp(ctx, obj, "y", options.y);
        GetIntProp(ctx, obj, "width", options.width);
        GetIntProp(ctx, obj, "height", options.height);
        GetFloatProp(ctx, obj, "rotate", options.rotate);

        std::wstring bg = GetStringProp(ctx, obj, "backgroundColor");
        ParseGradientOrColor(bg, options.solidColor, options.solidAlpha, options.solidGradient, options.hasSolidColor);
        GetIntProp(ctx, obj, "backgroundColorRadius", options.solidColorRadius);

        std::wstring bevelType = GetStringProp(ctx, obj, "bevelType");
        if (bevelType == L"raised")
            options.bevelType = 1;
        else if (bevelType == L"sunken")
            options.bevelType = 2;
        else if (bevelType == L"emboss")
            options.bevelType = 3;
        else if (bevelType == L"pillow")
            options.bevelType = 4;
        else if (!bevelType.empty())
            options.bevelType = 0;

        GetIntProp(ctx, obj, "bevelWidth", options.bevelWidth);
        std::wstring b1 = GetStringProp(ctx, obj, "bevelColor");
        if (!b1.empty())
        {
            bool hasBevelColor = false;
            ParseGradientOrColor(b1, options.bevelColor, options.bevelAlpha, options.bevelGradient, hasBevelColor);
        }
        std::wstring b2 = GetStringProp(ctx, obj, "bevelColor2");
        if (!b2.empty())
        {
            bool hasBevelColor2 = false;
            ParseGradientOrColor(b2, options.bevelColor2, options.bevelAlpha2, options.bevelGradient2, hasBevelColor2);
        }

        JSValue pad = JS_GetPropertyStr(ctx, obj, "padding");
        if (JS_IsNumber(pad))
        {
            int32_t p = 0;
            if (JS_ToInt32(ctx, &p, pad) == 0)
            {
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = p;
            }
        }
        else if (JS_IsArray(pad))
        {
            std::vector<float> arr;
            if (GetFloatArrayProp(ctx, obj, "padding", arr, 1))
            {
                if (arr.size() >= 4)
                {
                    options.paddingLeft = static_cast<int>(arr[0]);
                    options.paddingTop = static_cast<int>(arr[1]);
                    options.paddingRight = static_cast<int>(arr[2]);
                    options.paddingBottom = static_cast<int>(arr[3]);
                }
                else if (arr.size() >= 2)
                {
                    options.paddingLeft = options.paddingRight = static_cast<int>(arr[0]);
                    options.paddingTop = options.paddingBottom = static_cast<int>(arr[1]);
                }
            }
        }
        JS_FreeValue(ctx, pad);

        GetBoolProp(ctx, obj, "antiAlias", options.antialias);
        if (GetBoolProp(ctx, obj, "pixelHitTest", options.pixelHitTest))
            options.hasPixelHitTest = true;
        GetBoolProp(ctx, obj, "show", options.show);
        options.containerId = GetStringProp(ctx, obj, "container");
        options.groupId = GetStringProp(ctx, obj, "group");
        GetBoolProp(ctx, obj, "mouseEventCursor", options.mouseEventCursor);
        options.mouseEventCursorName = GetStringProp(ctx, obj, "mouseEventCursorName");
        options.cursorsDir = GetStringProp(ctx, obj, "cursorsDir");
        if (!options.cursorsDir.empty())
        {
            options.cursorsDir = PathUtils::ResolvePath(options.cursorsDir, baseDir);
        }

        if (GetFloatArrayProp(ctx, obj, "transformMatrix", options.transformMatrix, 6))
        {
            options.hasTransformMatrix = true;
        }

        GetEventCallbackProp(ctx, obj, "onLeftMouseUp", options.onLeftMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftMouseDown", options.onLeftMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftDoubleClick", options.onLeftDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseUp", options.onRightMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseDown", options.onRightMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightDoubleClick", options.onRightDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseUp", options.onMiddleMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseDown", options.onMiddleMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleDoubleClick", options.onMiddleDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseUp", options.onX1MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseDown", options.onX1MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1DoubleClick", options.onX1DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseUp", options.onX2MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseDown", options.onX2MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2DoubleClick", options.onX2DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollUp", options.onScrollUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollDown", options.onScrollDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollLeft", options.onScrollLeftCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollRight", options.onScrollRightCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseOver", options.onMouseOverCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseLeave", options.onMouseLeaveCallbackId);
        GetEventCallbackProp(ctx, obj, "onDragStart", options.onDragStartCallbackId);
        GetEventCallbackProp(ctx, obj, "onDrag", options.onDragCallbackId);
        GetEventCallbackProp(ctx, obj, "onDragEnd", options.onDragEndCallbackId);

        options.tooltipText = GetStringProp(ctx, obj, "tooltipText");
        options.tooltipTitle = GetStringProp(ctx, obj, "tooltipTitle");
        options.tooltipIcon = GetStringProp(ctx, obj, "tooltipIcon");
        GetIntProp(ctx, obj, "tooltipMaxWidth", options.tooltipMaxWidth);
        GetIntProp(ctx, obj, "tooltipMaxHeight", options.tooltipMaxHeight);
        GetBoolProp(ctx, obj, "tooltipBalloon", options.tooltipBalloon);
        GetBoolProp(ctx, obj, "tooltipDisabled", options.tooltipDisabled);
    }

    void ParseGeneralImageOptions(JSContext *ctx, JSValueConst obj, GeneralImageOptions &options)
    {
        std::wstring imageFlip = GetStringProp(ctx, obj, "imageFlip");
        std::transform(imageFlip.begin(), imageFlip.end(), imageFlip.begin(), ::towlower);
        if (imageFlip == L"horizontal")
            options.imageFlip = IMAGE_FLIP_HORIZONTAL;
        else if (imageFlip == L"vertical")
            options.imageFlip = IMAGE_FLIP_VERTICAL;
        else if (imageFlip == L"both")
            options.imageFlip = IMAGE_FLIP_BOTH;
        else if (imageFlip == L"none")
            options.imageFlip = IMAGE_FLIP_NONE;

        std::vector<float> imageCrop;
        if (GetFloatArrayProp(ctx, obj, "imageCrop", imageCrop, 4))
        {
            if (imageCrop.size() >= 4)
            {
                options.hasImageCrop = true;
                options.imageCropX = imageCrop[0];
                options.imageCropY = imageCrop[1];
                options.imageCropW = imageCrop[2];
                options.imageCropH = imageCrop[3];
                options.imageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
                if (imageCrop.size() >= 5)
                {
                    int origin = (int)imageCrop[4];
                    if (origin < (int)IMAGE_CROP_ORIGIN_TOP_LEFT)
                        origin = (int)IMAGE_CROP_ORIGIN_TOP_LEFT;
                    if (origin > (int)IMAGE_CROP_ORIGIN_CENTER)
                        origin = (int)IMAGE_CROP_ORIGIN_CENTER;
                    options.imageCropOrigin = (ImageCropOrigin)origin;
                }
            }
        }

        GetBoolProp(ctx, obj, "grayscale", options.grayscale);
        GetBoolProp(ctx, obj, "useExifOrientation", options.useExifOrientation);
        int alpha = 255;
        if (GetIntProp(ctx, obj, "imageAlpha", alpha))
            options.imageAlpha = static_cast<BYTE>(alpha);

        std::wstring tint = GetStringProp(ctx, obj, "imageTint");
        if (!tint.empty() && ColorUtil::ParseRGBA(tint, options.imageTint, options.imageTintAlpha))
        {
            options.hasImageTint = true;
        }

        std::vector<float> colorMatrix;
        if (GetFloatArrayProp(ctx, obj, "colorMatrix", colorMatrix, 20))
        {
            if (colorMatrix.size() >= 20)
            {
                for (int i = 0; i < 20; ++i)
                    options.colorMatrix[i] = colorMatrix[i];
                options.hasColorMatrix = true;
            }
        }
    }

    void ParseImageOptions(JSContext *ctx, JSValueConst obj, ImageOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        options.path = GetStringProp(ctx, obj, "path");
        if (!options.path.empty())
        {
            options.path = PathUtils::ResolvePath(options.path, baseDir);
            std::error_code ec;
            const bool exists = std::filesystem::exists(std::filesystem::path(options.path), ec);
        }

        std::wstring aspect = GetStringProp(ctx, obj, "preserveAspectRatio");
        if (aspect == L"preserve")
            options.preserveAspectRatio = IMAGE_ASPECT_PRESERVE;
        else if (aspect == L"crop")
            options.preserveAspectRatio = IMAGE_ASPECT_CROP;
        else if (aspect == L"stretch")
            options.preserveAspectRatio = IMAGE_ASPECT_STRETCH;

        std::vector<float> scaleMargins;
        if (GetFloatArrayProp(ctx, obj, "scaleMargins", scaleMargins, 4))
        {
            options.hasScaleMargins = true;
            options.scaleMarginLeft = scaleMargins[0];
            options.scaleMarginTop = scaleMargins[1];
            options.scaleMarginRight = scaleMargins[2];
            options.scaleMarginBottom = scaleMargins[3];
        }

        GetBoolProp(ctx, obj, "tile", options.tile);
    }

    void ParseButtonOptions(JSContext *ctx, JSValueConst obj, ButtonOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);
        
        options.buttonImageName = GetStringProp(ctx, obj, "buttonImageName");
        if (!options.buttonImageName.empty())
        {
            options.buttonImageName = PathUtils::ResolvePath(options.buttonImageName, baseDir);
        }

        GetEventCallbackProp(ctx, obj, "buttonAction", options.onLeftMouseUpCallbackId);
    }

    void ParseBitmapOptions(JSContext *ctx, JSValueConst obj, BitmapOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        // Bitmap meter behavior is frame-driven; width/height are ignored.
        options.width = 0;
        options.height = 0;

        // ImageCrop is intentionally ignored for Bitmap element semantics.
        options.hasImageCrop = false;

        // Read as double to preserve numeric precision from JS.
        JSValue v = JS_GetPropertyStr(ctx, obj, "value");
        if (!JS_IsException(v) && !JS_IsUndefined(v) && !JS_IsNull(v))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, v) == 0)
            {
                options.value = d;
            }
        }
        JS_FreeValue(ctx, v);

        options.bitmapImageName = GetStringProp(ctx, obj, "bitmapImageName");
        if (!options.bitmapImageName.empty())
        {
            options.bitmapImageName = PathUtils::ResolvePath(options.bitmapImageName, baseDir);
        }

        GetIntProp(ctx, obj, "bitmapFrames", options.bitmapFrames);
        GetBoolProp(ctx, obj, "bitmapZeroFrame", options.bitmapZeroFrame);
        GetBoolProp(ctx, obj, "bitmapExtend", options.bitmapExtend);
        { float tmp = static_cast<float>(options.minValue); if (GetFloatProp(ctx, obj, "minValue", tmp)) options.minValue = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.maxValue); if (GetFloatProp(ctx, obj, "maxValue", tmp)) options.maxValue = static_cast<double>(tmp); }
        options.bitmapOrientation = GetStringProp(ctx, obj, "bitmapOrientation");
        GetIntProp(ctx, obj, "bitmapDigits", options.bitmapDigits);
        GetIntProp(ctx, obj, "bitmapSeparation", options.bitmapSeparation);

        std::wstring align = GetStringProp(ctx, obj, "bitmapAlign");
        std::transform(align.begin(), align.end(), align.begin(), ::towlower);
        if (align == L"center")
            options.bitmapAlign = BITMAP_ALIGN_CENTER;
        else if (align == L"right")
            options.bitmapAlign = BITMAP_ALIGN_RIGHT;
        else if (align == L"left")
            options.bitmapAlign = BITMAP_ALIGN_LEFT;
    }

    void ParseRotatorOptions(JSContext *ctx, JSValueConst obj, RotatorOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        options.hasImageCrop = false;

        // Read value
        JSValue v = JS_GetPropertyStr(ctx, obj, "value");
        if (!JS_IsException(v) && !JS_IsUndefined(v) && !JS_IsNull(v))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, v) == 0)
            {
                options.value = d;
            }
        }
        JS_FreeValue(ctx, v);

        options.rotatorImageName = GetStringProp(ctx, obj, "rotatorImageName");
        if (!options.rotatorImageName.empty())
        {
            options.rotatorImageName = PathUtils::ResolvePath(options.rotatorImageName, baseDir);
        }

        { float tmp = static_cast<float>(options.offsetX); if (GetFloatProp(ctx, obj, "offsetX", tmp)) options.offsetX = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.offsetY); if (GetFloatProp(ctx, obj, "offsetY", tmp)) options.offsetY = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.startAngle); if (GetFloatProp(ctx, obj, "startAngle", tmp)) options.startAngle = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.rotationAngle); if (GetFloatProp(ctx, obj, "rotationAngle", tmp)) options.rotationAngle = static_cast<double>(tmp); }
        GetIntProp(ctx, obj, "valueRemainder", options.valueRemainder);
        { float tmp = static_cast<float>(options.minValue); if (GetFloatProp(ctx, obj, "minValue", tmp)) options.minValue = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.maxValue); if (GetFloatProp(ctx, obj, "maxValue", tmp)) options.maxValue = static_cast<double>(tmp); }
    }

    void ParseAreaGraphOptions(JSContext *ctx, JSValueConst obj, AreaGraphOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        JSValue dataVal = JS_GetPropertyStr(ctx, obj, "data");
        if (JS_IsArray(dataVal))
        {
            uint32_t len = 0;
            JSValue lenVal = JS_GetPropertyStr(ctx, dataVal, "length");
            JS_ToUint32(ctx, &len, lenVal);
            JS_FreeValue(ctx, lenVal);

            options.data.clear();
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue v = JS_GetPropertyUint32(ctx, dataVal, i);
                double d = 0.0;
                JS_ToFloat64(ctx, &d, v);
                options.data.push_back(static_cast<float>(d));
                JS_FreeValue(ctx, v);
            }
        }
        JS_FreeValue(ctx, dataVal);

        GetFloatProp(ctx, obj, "minValue", options.minValue);
        GetFloatProp(ctx, obj, "maxValue", options.maxValue);
        GetBoolProp(ctx, obj, "autoRange", options.autoRange);

        std::wstring lc = GetStringProp(ctx, obj, "lineColor");
        if (!lc.empty())
        {
            GradientInfo parsedGradient;
            BYTE parsedAlpha = 255;
            if (ParseGradientString(lc, parsedGradient))
            {
                options.lineGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(lc, options.lineColor, parsedAlpha))
            {
                options.lineGradient = GradientInfo();
            }
        }
        GetFloatProp(ctx, obj, "lineWidth", options.lineWidth);

        std::wstring fc = GetStringProp(ctx, obj, "fillColor");
        if (!fc.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(fc, parsedGradient))
            {
                options.fillGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(fc, options.fillColor, options.fillAlpha))
            {
                options.fillGradient = GradientInfo();
            }
        }
        GetIntProp(ctx, obj, "maxPoints", options.maxPoints);

        std::wstring gc = GetStringProp(ctx, obj, "gridColor");
        if (!gc.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(gc, parsedGradient))
            {
                options.gridGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(gc, options.gridColor, options.gridAlpha))
            {
                options.gridGradient = GradientInfo();
            }
        }

        GetIntProp(ctx, obj, "gridX", options.gridX);
        GetIntProp(ctx, obj, "gridY", options.gridY);
        GetBoolProp(ctx, obj, "gridVisible", options.gridVisible);
        GetBoolProp(ctx, obj, "graphStartLeft", options.graphStartLeft);
        GetBoolProp(ctx, obj, "flip", options.flip);
    }

    void ParseTextOptions(JSContext *ctx, JSValueConst obj, TextOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        {
            JSValue textProp = JS_GetPropertyStr(ctx, obj, "text");
            if (!JS_IsException(textProp) && !JS_IsUndefined(textProp) && !JS_IsNull(textProp))
            {
                if (JS_IsString(textProp))
                {
                    const char *s = JS_ToCString(ctx, textProp);
                    if (s)
                    {
                        options.text = Utils::ToWString(s);
                        JS_FreeCString(ctx, s);
                    }
                }
            }
            JS_FreeValue(ctx, textProp);
        }
        std::wstring fontFace = GetStringProp(ctx, obj, "fontFace");
        if (!fontFace.empty())
            options.fontFace = fontFace;
        GetIntProp(ctx, obj, "fontSize", options.fontSize);

        std::wstring fc = GetStringProp(ctx, obj, "fontColor");
        if (!fc.empty())
        {
            ParseGradientOrColor(fc, options.fontColor, options.alpha, options.fontGradient, options.hasSolidColor);
        }

        GetFloatProp(ctx, obj, "letterSpacing", options.letterSpacing);
        GetBoolProp(ctx, obj, "underLine", options.underLine);
        GetBoolProp(ctx, obj, "strikeThrough", options.strikeThrough);

        std::wstring caseStr = GetStringProp(ctx, obj, "case");
        if (caseStr == L"upper")
            options.textCase = TEXT_CASE_UPPER;
        else if (caseStr == L"lower")
            options.textCase = TEXT_CASE_LOWER;
        else if (caseStr == L"capitalize")
            options.textCase = TEXT_CASE_CAPITALIZE;
        else if (caseStr == L"sentence")
            options.textCase = TEXT_CASE_SENTENCE;

        JSValue fw = JS_GetPropertyStr(ctx, obj, "fontWeight");
        if (JS_IsNumber(fw))
        {
            int32_t w = 400;
            if (JS_ToInt32(ctx, &w, fw) == 0)
                options.fontWeight = w;
        }
        else if (JS_IsString(fw))
        {
            const char *s = JS_ToCString(ctx, fw);
            if (s)
            {
                std::wstring weight = Utils::ToWString(s);
                std::transform(weight.begin(), weight.end(), weight.begin(), ::towlower);
                if (weight == L"thin")
                    options.fontWeight = 100;
                else if (weight == L"extralight" || weight == L"ultralight")
                    options.fontWeight = 200;
                else if (weight == L"light")
                    options.fontWeight = 300;
                else if (weight == L"normal" || weight == L"regular")
                    options.fontWeight = 400;
                else if (weight == L"medium")
                    options.fontWeight = 500;
                else if (weight == L"semibold" || weight == L"demibold")
                    options.fontWeight = 600;
                else if (weight == L"bold")
                    options.fontWeight = 700;
                else if (weight == L"extrabold" || weight == L"ultrabold")
                    options.fontWeight = 800;
                else if (weight == L"black" || weight == L"heavy")
                    options.fontWeight = 900;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, fw);

        options.fontPath = GetStringProp(ctx, obj, "fontPath");
        if (!options.fontPath.empty())
        {
            options.fontPath = PathUtils::ResolvePath(options.fontPath, baseDir);
        }

        std::wstring style = GetStringProp(ctx, obj, "fontStyle");
        options.italic = (style == L"italic");

        std::wstring align = GetStringProp(ctx, obj, "textAlign");
        if (align.empty())
            align = GetStringProp(ctx, obj, "align");
        std::transform(align.begin(), align.end(), align.begin(), ::towlower);
        if (align == L"left" || align == L"lefttop")
            options.textAlign = TEXT_ALIGN_LEFT_TOP;
        else if (align == L"center" || align == L"centertop")
            options.textAlign = TEXT_ALIGN_CENTER_TOP;
        else if (align == L"right" || align == L"righttop")
            options.textAlign = TEXT_ALIGN_RIGHT_TOP;
        else if (align == L"leftcenter")
            options.textAlign = TEXT_ALIGN_LEFT_CENTER;
        else if (align == L"centercenter" || align == L"middlecenter" || align == L"middle")
            options.textAlign = TEXT_ALIGN_CENTER_CENTER;
        else if (align == L"rightcenter")
            options.textAlign = TEXT_ALIGN_RIGHT_CENTER;
        else if (align == L"leftbottom")
            options.textAlign = TEXT_ALIGN_LEFT_BOTTOM;
        else if (align == L"centerbottom")
            options.textAlign = TEXT_ALIGN_CENTER_BOTTOM;
        else if (align == L"rightbottom")
            options.textAlign = TEXT_ALIGN_RIGHT_BOTTOM;

        std::wstring clip = GetStringProp(ctx, obj, "textClip");
        if (clip == L"none")
            options.clip = TEXT_CLIP_NONE;
        else if (clip == L"on" || clip == L"clip")
            options.clip = TEXT_CLIP_ON;
        else if (clip == L"wrap")
            options.clip = TEXT_CLIP_WRAP;
        else if (clip == L"ellipsis")
            options.clip = TEXT_CLIP_ELLIPSIS;
    }

    void ParseBarOptions(JSContext *ctx, JSValueConst obj, BarOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        GetFloatProp(ctx, obj, "value", options.value);

        std::wstring orientation = GetStringProp(ctx, obj, "orientation");
        if (orientation == L"vertical")
            options.orientation = BAR_VERTICAL;
        else if (orientation == L"horizontal")
            options.orientation = BAR_HORIZONTAL;
        GetIntProp(ctx, obj, "barCornerRadius", options.barCornerRadius);

        std::wstring bc = GetStringProp(ctx, obj, "barColor");
        ParseGradientOrColor(bc, options.barColor, options.barAlpha, options.barGradient, options.hasBarColor);
    }

    void ParseLineOptions(JSContext *ctx, JSValueConst obj, LineOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetIntProp(ctx, obj, "lineCount", options.lineCount);
        if (options.lineCount < 1)
        {
            options.lineCount = 1;
        }

        GetFloatProp(ctx, obj, "lineWidth", options.lineWidth);
        if (options.lineWidth < 1.0f)
        {
            options.lineWidth = 1.0f;
        }
        GetIntProp(ctx, obj, "maxPoints", options.maxPoints);
        if (options.maxPoints < 0)
        {
            options.maxPoints = 0;
        }

        GetBoolProp(ctx, obj, "horizontalLines", options.horizontalLines);
        std::wstring horizontalLineColor = GetStringProp(ctx, obj, "horizontalLineColor");
        if (!horizontalLineColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(horizontalLineColor, parsedGradient))
            {
                options.horizontalLineGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(horizontalLineColor, options.horizontalLineColor, options.horizontalLineAlpha))
            {
                options.horizontalLineGradient = GradientInfo();
            }
        }

        std::wstring graphStart = GetStringProp(ctx, obj, "graphStart");
        std::transform(graphStart.begin(), graphStart.end(), graphStart.begin(), ::towlower);
        if (graphStart == L"left")
        {
            options.graphStartLeft = true;
        }
        else if (graphStart == L"right")
        {
            options.graphStartLeft = false;
        }

        std::wstring graphOrientation = GetStringProp(ctx, obj, "graphOrientation");
        std::transform(graphOrientation.begin(), graphOrientation.end(), graphOrientation.begin(), ::towlower);
        if (graphOrientation == L"horizontal")
        {
            options.graphHorizontalOrientation = true;
        }
        else if (graphOrientation == L"vertical")
        {
            options.graphHorizontalOrientation = false;
        }

        GetBoolProp(ctx, obj, "flip", options.flip);

        std::wstring transformStroke = GetStringProp(ctx, obj, "transformStroke");
        std::transform(transformStroke.begin(), transformStroke.end(), transformStroke.begin(), ::towlower);
        if (transformStroke == L"fixed")
        {
            options.transformStroke = D2D1_STROKE_TRANSFORM_TYPE_FIXED;
        }
        else if (transformStroke == L"normal")
        {
            options.transformStroke = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
        }

        GetBoolProp(ctx, obj, "autoRange", options.autoRange);
        GetFloatProp(ctx, obj, "rangeMin", options.scaleMin);
        GetFloatProp(ctx, obj, "rangeMax", options.scaleMax);
        if (options.scaleMax < options.scaleMin)
        {
            std::swap(options.scaleMin, options.scaleMax);
        }
        if (fabsf(options.scaleMax - options.scaleMin) < 0.000001f)
        {
            options.scaleMax = options.scaleMin + 1.0f;
        }

        const size_t desiredLineCount = (size_t)options.lineCount;

        if (options.dataSets.size() != desiredLineCount)
        {
            options.dataSets.resize(desiredLineCount);
        }

        if (options.lineColors.size() < desiredLineCount)
        {
            options.lineColors.resize(desiredLineCount, RGB(255, 255, 255));
        }
        else if (options.lineColors.size() > desiredLineCount)
        {
            options.lineColors.resize(desiredLineCount);
        }

        if (options.lineAlphas.size() < desiredLineCount)
        {
            options.lineAlphas.resize(desiredLineCount, 255);
        }
        else if (options.lineAlphas.size() > desiredLineCount)
        {
            options.lineAlphas.resize(desiredLineCount);
        }

        if (options.lineGradients.size() < desiredLineCount)
        {
            options.lineGradients.resize(desiredLineCount);
        }
        else if (options.lineGradients.size() > desiredLineCount)
        {
            options.lineGradients.resize(desiredLineCount);
        }

        if (options.scaleValues.size() < desiredLineCount)
        {
            options.scaleValues.resize(desiredLineCount, 1.0f);
        }
        else if (options.scaleValues.size() > desiredLineCount)
        {
            options.scaleValues.resize(desiredLineCount);
        }

        for (int i = 0; i < options.lineCount; ++i)
        {
            std::wstring dataKey = (i == 0) ? L"data" : (L"data" + std::to_wstring(i + 1));
            std::vector<float> data;
            std::string dataKeyUtf8 = Utils::ToString(dataKey);
            if (GetFloatArrayProp(ctx, obj, dataKeyUtf8.c_str(), data, 1))
            {
                options.dataSets[(size_t)i] = std::move(data);
            }

            std::wstring colorKey = (i == 0) ? L"lineColor" : (L"lineColor" + std::to_wstring(i + 1));
            std::string colorKeyUtf8 = Utils::ToString(colorKey);
            std::wstring colorValue = GetStringProp(ctx, obj, colorKeyUtf8.c_str());
            if (!colorValue.empty())
            {
                GradientInfo parsedGradient;
                if (ParseGradientString(colorValue, parsedGradient))
                {
                    options.lineGradients[(size_t)i] = parsedGradient;
                }
                else if (ColorUtil::ParseRGBA(colorValue, options.lineColors[(size_t)i], options.lineAlphas[(size_t)i]))
                {
                    options.lineGradients[(size_t)i] = GradientInfo();
                }
            }

            std::wstring scaleKey = (i == 0) ? L"lineScale" : (L"lineScale" + std::to_wstring(i + 1));
            std::string scaleKeyUtf8 = Utils::ToString(scaleKey);
            float scaleValue = 1.0f;
            if (GetFloatProp(ctx, obj, scaleKeyUtf8.c_str(), scaleValue))
            {
                if (!std::isfinite(scaleValue))
                {
                    scaleValue = 1.0f;
                }
                options.scaleValues[(size_t)i] = scaleValue;
            }
        }
    }

    void ParseHistogramOptions(JSContext *ctx, JSValueConst obj, HistogramOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetFloatArrayPropAllowEmpty(ctx, obj, "data", options.data);
        GetFloatArrayPropAllowEmpty(ctx, obj, "data2", options.data2);

        GetBoolProp(ctx, obj, "autoRange", options.autoRange);
        GetBoolProp(ctx, obj, "flip", options.flip);

        std::wstring graphStart = GetStringProp(ctx, obj, "graphStart");
        std::transform(graphStart.begin(), graphStart.end(), graphStart.begin(), ::towlower);
        if (graphStart == L"left")
            options.graphStartLeft = true;
        else if (graphStart == L"right")
            options.graphStartLeft = false;

        std::wstring graphOrientation = GetStringProp(ctx, obj, "graphOrientation");
        std::transform(graphOrientation.begin(), graphOrientation.end(), graphOrientation.begin(), ::towlower);
        if (graphOrientation == L"horizontal")
            options.graphHorizontalOrientation = true;
        else if (graphOrientation == L"vertical")
            options.graphHorizontalOrientation = false;

        std::wstring primaryColor = GetStringProp(ctx, obj, "primaryColor");
        if (!primaryColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(primaryColor, parsedGradient))
            {
                options.primaryGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(primaryColor, options.primaryColor, options.primaryAlpha))
            {
                options.primaryGradient = GradientInfo();
            }
        }

        std::wstring secondaryColor = GetStringProp(ctx, obj, "secondaryColor");
        if (!secondaryColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(secondaryColor, parsedGradient))
            {
                options.secondaryGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(secondaryColor, options.secondaryColor, options.secondaryAlpha))
            {
                options.secondaryGradient = GradientInfo();
            }
        }

        std::wstring bothColor = GetStringProp(ctx, obj, "bothColor");
        if (!bothColor.empty())
        {
            GradientInfo parsedGradient;
            if (ParseGradientString(bothColor, parsedGradient))
            {
                options.bothGradient = parsedGradient;
            }
            else if (ColorUtil::ParseRGBA(bothColor, options.bothColor, options.bothAlpha))
            {
                options.bothGradient = GradientInfo();
            }
        }

    }

    void ParseRoundLineOptions(JSContext *ctx, JSValueConst obj, RoundLineOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetFloatProp(ctx, obj, "value", options.value);
        GetIntProp(ctx, obj, "radius", options.radius);
        GetIntProp(ctx, obj, "thickness", options.thickness);
        GetIntProp(ctx, obj, "endThickness", options.endThickness);
        GetFloatProp(ctx, obj, "startAngle", options.startAngle);
        GetFloatProp(ctx, obj, "totalAngle", options.totalAngle);
        GetBoolProp(ctx, obj, "clockwise", options.clockwise);

        std::wstring capType = GetStringProp(ctx, obj, "capType");
        if (capType == L"round")
            options.startCap = options.endCap = ROUNDLINE_CAP_ROUND;
        else if (capType == L"flat")
            options.startCap = options.endCap = ROUNDLINE_CAP_FLAT;

        std::wstring startCap = GetStringProp(ctx, obj, "startCap");
        if (startCap == L"round")
            options.startCap = ROUNDLINE_CAP_ROUND;
        else if (startCap == L"flat")
            options.startCap = ROUNDLINE_CAP_FLAT;

        std::wstring endCap = GetStringProp(ctx, obj, "endCap");
        if (endCap == L"round")
            options.endCap = ROUNDLINE_CAP_ROUND;
        else if (endCap == L"flat")
            options.endCap = ROUNDLINE_CAP_FLAT;

        std::vector<float> dash;
        if (GetFloatArrayProp(ctx, obj, "dashArray", dash, 1))
            options.dashArray = dash;

        GetIntProp(ctx, obj, "ticks", options.ticks);

        std::wstring lc = GetStringProp(ctx, obj, "lineColor");
        ParseGradientOrColor(lc, options.lineColor, options.lineAlpha, options.lineGradient, options.hasLineColor);

        std::wstring lcb = GetStringProp(ctx, obj, "lineColorBg");
        ParseGradientOrColor(lcb, options.lineColorBg, options.lineAlphaBg, options.lineGradientBg, options.hasLineColorBg);
    }

    void ParseShapeOptions(JSContext *ctx, JSValueConst obj, ShapeOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        options.shapeType = GetStringProp(ctx, obj, "type");
        GetFloatProp(ctx, obj, "strokeWidth", options.strokeWidth);

        std::wstring stroke = GetStringProp(ctx, obj, "strokeColor");
        bool hasStroke = false;
        ParseGradientOrColor(stroke, options.strokeColor, options.strokeAlpha, options.strokeGradient, hasStroke);

        std::wstring fill = GetStringProp(ctx, obj, "fillColor");
        bool hasFill = false;
        ParseGradientOrColor(fill, options.fillColor, options.fillAlpha, options.fillGradient, hasFill);

        bool hasRadiusX = GetFloatProp(ctx, obj, "radiusX", options.radiusX);
        bool hasRadiusY = GetFloatProp(ctx, obj, "radiusY", options.radiusY);
        float uniformRadius = 0.0f;
        if (GetFloatProp(ctx, obj, "radius", uniformRadius))
        {
            // Legacy alias: radius applies to both X/Y when specific values are not provided.
            if (!hasRadiusX)
                options.radiusX = uniformRadius;
            if (!hasRadiusY)
                options.radiusY = uniformRadius;
        }
        GetFloatProp(ctx, obj, "startX", options.startX);
        GetFloatProp(ctx, obj, "startY", options.startY);
        GetFloatProp(ctx, obj, "endX", options.endX);
        GetFloatProp(ctx, obj, "endY", options.endY);
        options.curveType = GetStringProp(ctx, obj, "curveType");
        GetFloatProp(ctx, obj, "controlX", options.controlX);
        GetFloatProp(ctx, obj, "controlY", options.controlY);
        GetFloatProp(ctx, obj, "control2X", options.control2X);
        GetFloatProp(ctx, obj, "control2Y", options.control2Y);
        GetFloatProp(ctx, obj, "startAngle", options.startArcAngle);
        GetFloatProp(ctx, obj, "endAngle", options.endArcAngle);
        GetBoolProp(ctx, obj, "clockwise", options.arcClockwise);
        options.pathData = GetStringProp(ctx, obj, "pathData");

        std::wstring cap = GetStringProp(ctx, obj, "strokeStartCap");
        if (!cap.empty())
            options.strokeStartCap = GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeEndCap");
        if (!cap.empty())
            options.strokeEndCap = GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeDashCap");
        if (!cap.empty())
            options.strokeDashCap = GetCapStyle(cap);

        std::wstring join = GetStringProp(ctx, obj, "strokeLineJoin");
        if (!join.empty())
            options.strokeLineJoin = GetLineJoin(join);
        GetFloatProp(ctx, obj, "strokeDashOffset", options.strokeDashOffset);
        GetFloatArrayProp(ctx, obj, "strokeDashes", options.strokeDashes, 1);

        // Combine shape options (legacy-only): type/base/ops/consume.
        std::wstring lowerType = options.shapeType;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::towlower);
        options.isCombine = (lowerType == L"combine");
        options.combineBaseId = GetStringProp(ctx, obj, "base");
        options.hasCombineConsumeAll = GetBoolProp(ctx, obj, "consume", options.combineConsumeAll);

        JSValue ops = JS_GetPropertyStr(ctx, obj, "ops");
        if (JS_IsArray(ops))
        {
            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, ops, "length");
            if (JS_ToUint32(ctx, &len, lenV) == 0)
            {
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue it = JS_GetPropertyUint32(ctx, ops, i);
                    if (JS_IsObject(it))
                    {
                        ShapeCombineOp op;
                        op.id = GetStringProp(ctx, it, "id");
                        op.mode = ParseCombineMode(GetStringProp(ctx, it, "mode"));
                        if (GetBoolProp(ctx, it, "consume", op.consume))
                            op.hasConsume = true;
                        if (!op.id.empty())
                            options.combineOps.push_back(op);
                    }
                    JS_FreeValue(ctx, it);
                }
            }
            JS_FreeValue(ctx, lenV);
        }
        JS_FreeValue(ctx, ops);
    }

    void ApplyElementOptions(Element *element, const ElementOptions &options)
    {
        if (!element)
            return;
        element->SetPosition(options.x, options.y);
        element->SetSize(options.width, options.height);
        element->SetRotate(options.rotate);
        element->SetAntiAlias(options.antialias);
        if (options.hasPixelHitTest)
            element->SetPixelHitTest(options.pixelHitTest);
        element->SetShow(options.show);
        element->SetContainerId(options.containerId);
        element->SetGroupId(options.groupId);
        element->SetMouseEventCursor(options.mouseEventCursor);
        element->SetMouseEventCursorName(options.mouseEventCursorName);
        element->SetCursorsDir(options.cursorsDir);
        element->SetCornerRadius(options.solidColorRadius);
        element->SetPadding(options.paddingLeft, options.paddingTop, options.paddingRight, options.paddingBottom);

        if (options.solidGradient.type != GRADIENT_NONE)
        {
            element->SetSolidGradient(options.solidGradient);
        }
        else if (options.hasSolidColor)
        {
            element->SetSolidColor(options.solidColor, options.solidAlpha);
        }

        if (options.bevelType > 0)
        {
            element->SetBevel(options.bevelType, options.bevelWidth, options.bevelColor, options.bevelAlpha, options.bevelColor2, options.bevelAlpha2);
            element->SetBevelGradient(options.bevelGradient);
            element->SetBevelGradient2(options.bevelGradient2);
        }
        else
        {
            element->SetBevel(0, 0, 0, 0, 0, 0);
            element->SetBevelGradient(GradientInfo());
            element->SetBevelGradient2(GradientInfo());
        }

        if (options.hasTransformMatrix && options.transformMatrix.size() >= 6)
        {
            element->SetTransformMatrix(options.transformMatrix.data());
        }

        if (options.onLeftMouseUpCallbackId != -1)
            element->m_OnLeftMouseUpCallbackId = options.onLeftMouseUpCallbackId;
        if (options.onLeftMouseDownCallbackId != -1)
            element->m_OnLeftMouseDownCallbackId = options.onLeftMouseDownCallbackId;
        if (options.onLeftDoubleClickCallbackId != -1)
            element->m_OnLeftDoubleClickCallbackId = options.onLeftDoubleClickCallbackId;
        if (options.onRightMouseUpCallbackId != -1)
            element->m_OnRightMouseUpCallbackId = options.onRightMouseUpCallbackId;
        if (options.onRightMouseDownCallbackId != -1)
            element->m_OnRightMouseDownCallbackId = options.onRightMouseDownCallbackId;
        if (options.onRightDoubleClickCallbackId != -1)
            element->m_OnRightDoubleClickCallbackId = options.onRightDoubleClickCallbackId;
        if (options.onMiddleMouseUpCallbackId != -1)
            element->m_OnMiddleMouseUpCallbackId = options.onMiddleMouseUpCallbackId;
        if (options.onMiddleMouseDownCallbackId != -1)
            element->m_OnMiddleMouseDownCallbackId = options.onMiddleMouseDownCallbackId;
        if (options.onMiddleDoubleClickCallbackId != -1)
            element->m_OnMiddleDoubleClickCallbackId = options.onMiddleDoubleClickCallbackId;
        if (options.onX1MouseUpCallbackId != -1)
            element->m_OnX1MouseUpCallbackId = options.onX1MouseUpCallbackId;
        if (options.onX1MouseDownCallbackId != -1)
            element->m_OnX1MouseDownCallbackId = options.onX1MouseDownCallbackId;
        if (options.onX1DoubleClickCallbackId != -1)
            element->m_OnX1DoubleClickCallbackId = options.onX1DoubleClickCallbackId;
        if (options.onX2MouseUpCallbackId != -1)
            element->m_OnX2MouseUpCallbackId = options.onX2MouseUpCallbackId;
        if (options.onX2MouseDownCallbackId != -1)
            element->m_OnX2MouseDownCallbackId = options.onX2MouseDownCallbackId;
        if (options.onX2DoubleClickCallbackId != -1)
            element->m_OnX2DoubleClickCallbackId = options.onX2DoubleClickCallbackId;
        if (options.onScrollUpCallbackId != -1)
            element->m_OnScrollUpCallbackId = options.onScrollUpCallbackId;
        if (options.onScrollDownCallbackId != -1)
            element->m_OnScrollDownCallbackId = options.onScrollDownCallbackId;
        if (options.onScrollLeftCallbackId != -1)
            element->m_OnScrollLeftCallbackId = options.onScrollLeftCallbackId;
        if (options.onScrollRightCallbackId != -1)
            element->m_OnScrollRightCallbackId = options.onScrollRightCallbackId;
        if (options.onMouseOverCallbackId != -1)
            element->m_OnMouseOverCallbackId = options.onMouseOverCallbackId;
        if (options.onMouseLeaveCallbackId != -1)
            element->m_OnMouseLeaveCallbackId = options.onMouseLeaveCallbackId;
        if (options.onDragStartCallbackId != -1)
            element->m_OnDragStartCallbackId = options.onDragStartCallbackId;
        if (options.onDragCallbackId != -1)
            element->m_OnDragCallbackId = options.onDragCallbackId;
        if (options.onDragEndCallbackId != -1)
            element->m_OnDragEndCallbackId = options.onDragEndCallbackId;

        if (!options.tooltipText.empty())
        {
            element->SetToolTip(
                options.tooltipText,
                options.tooltipTitle,
                options.tooltipIcon,
                options.tooltipMaxWidth,
                options.tooltipMaxHeight,
                options.tooltipBalloon);
        }
        element->SetToolTipDisabled(options.tooltipDisabled);
    }

    void ApplyGeneralImageOptions(GeneralImage *image, const GeneralImageOptions &options)
    {
        if (!image)
            return;
        image->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            image->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        image->SetUseExifOrientation(options.useExifOrientation);
        image->SetGrayscale(options.grayscale);
        image->SetImageAlpha(options.imageAlpha);
        if (options.hasImageTint)
            image->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            image->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyImageOptions(ImageElement *element, const ImageOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        if (!options.path.empty())
            element->UpdateImage(options.path);
        element->SetPreserveAspectRatio(options.preserveAspectRatio);
        if (options.hasScaleMargins)
            element->SetScaleMargins(options.scaleMarginLeft, options.scaleMarginTop, options.scaleMarginRight, options.scaleMarginBottom);
        element->SetTile(options.tile);
        
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            element->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyButtonOptions(ButtonElement *element, const ButtonOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        if (!options.buttonImageName.empty())
            element->UpdateImage(options.buttonImageName);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            element->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        else
            element->ClearImageCrop();

        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyBitmapOptions(BitmapElement *element, const BitmapOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        if (!options.bitmapImageName.empty())
            element->UpdateImage(options.bitmapImageName);

        element->SetValue(options.value);
        element->SetBitmapFrames(options.bitmapFrames);
        element->SetBitmapZeroFrame(options.bitmapZeroFrame);
        element->SetBitmapExtend(options.bitmapExtend);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);
        element->SetBitmapOrientation(options.bitmapOrientation);
        element->SetBitmapDigits(options.bitmapDigits);
        element->SetBitmapAlign(options.bitmapAlign);
        element->SetBitmapSeparation(options.bitmapSeparation);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyRotatorOptions(RotatorElement *element, const RotatorOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        if (!options.rotatorImageName.empty())
            element->UpdateImage(options.rotatorImageName);

        element->SetValue(options.value);
        element->SetOffsetX(options.offsetX);
        element->SetOffsetY(options.offsetY);
        element->SetStartAngle(options.startAngle);
        element->SetRotationAngle(options.rotationAngle);
        element->SetValueRemainder(options.valueRemainder);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyAreaGraphOptions(AreaGraphElement *element, const AreaGraphOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        element->SetData(options.data);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);
        element->SetAutoRange(options.autoRange);
        element->SetLineColor(options.lineColor);
        element->SetLineGradient(options.lineGradient);
        element->SetLineWidth(options.lineWidth);
        element->SetFillColor(options.fillColor, options.fillAlpha);
        element->SetFillGradient(options.fillGradient);
        element->SetMaxPoints(options.maxPoints);
        element->SetGridColor(options.gridColor, options.gridAlpha);
        element->SetGridGradient(options.gridGradient);
        element->SetGridVisible(options.gridVisible);
        element->SetGridXSpacing(options.gridX);
        element->SetGridYSpacing(options.gridY);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetFlip(options.flip);
    }

    void ApplyTextOptions(TextElement *element, const TextOptions &options)
    {
        if (!element)
            return;
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
        element->SetFontGradient(options.fontGradient);
        element->SetLetterSpacing(options.letterSpacing);
        element->SetUnderline(options.underLine);
        element->SetStrikethrough(options.strikeThrough);
        element->SetTextCase(options.textCase);
    }

    void ApplyBarOptions(BarElement *element, const BarOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetOrientation(options.orientation);
        element->SetBarCornerRadius(options.barCornerRadius);
        if (options.barGradient.type != GRADIENT_NONE)
            element->SetBarGradient(options.barGradient);
        else if (options.hasBarColor)
            element->SetBarColor(options.barColor, options.barAlpha);
    }

    void ApplyLineOptions(LineElement *element, const LineOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetLineCount(options.lineCount);
        element->SetDataSets(options.dataSets);
        element->SetLineColors(options.lineColors, options.lineAlphas);
        element->SetLineGradients(options.lineGradients);
        element->SetScaleValues(options.scaleValues);
        element->SetLineWidth(options.lineWidth);
        element->SetMaxPoints(options.maxPoints);
        element->SetHorizontalLines(options.horizontalLines);
        element->SetHorizontalLineColor(options.horizontalLineColor, options.horizontalLineAlpha);
        element->SetHorizontalLineGradient(options.horizontalLineGradient);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetGraphHorizontalOrientation(options.graphHorizontalOrientation);
        element->SetFlip(options.flip);
        element->SetStrokeTransformType(options.transformStroke);
        element->SetAutoRange(options.autoRange);
        element->SetScaleRange(options.scaleMin, options.scaleMax);
    }

    void ApplyHistogramOptions(HistogramElement *element, const HistogramOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        element->SetData(options.data);
        element->SetData2(options.data2);
        element->SetAutoRange(options.autoRange);
        element->SetGraphStartLeft(options.graphStartLeft);
        element->SetGraphHorizontalOrientation(options.graphHorizontalOrientation);
        element->SetFlip(options.flip);
        element->SetPrimaryColor(options.primaryColor, options.primaryAlpha);
        element->SetSecondaryColor(options.secondaryColor, options.secondaryAlpha);
        element->SetBothColor(options.bothColor, options.bothAlpha);
        element->SetPrimaryGradient(options.primaryGradient);
        element->SetSecondaryGradient(options.secondaryGradient);
        element->SetBothGradient(options.bothGradient);
    }

    void ApplyRoundLineOptions(RoundLineElement *element, const RoundLineOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetRadius(options.radius);
        element->SetThickness(options.thickness);
        element->SetEndThickness(options.endThickness);
        element->SetStartAngle(options.startAngle);
        element->SetTotalAngle(options.totalAngle);
        element->SetClockwise(options.clockwise);
        element->SetStartCap(options.startCap);
        element->SetEndCap(options.endCap);
        element->SetDashArray(options.dashArray);
        element->SetTicks(options.ticks);
        if (options.lineGradient.type != GRADIENT_NONE)
            element->SetLineGradient(options.lineGradient);
        else if (options.hasLineColor)
            element->SetLineColor(options.lineColor, options.lineAlpha);
        if (options.lineGradientBg.type != GRADIENT_NONE)
            element->SetLineGradientBg(options.lineGradientBg);
        else if (options.hasLineColorBg)
            element->SetLineColorBg(options.lineColorBg, options.lineAlphaBg);
    }

    void ApplyShapeOptions(ShapeElement *element, const ShapeOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);

        if (options.strokeGradient.type != GRADIENT_NONE)
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
            element->SetStrokeGradient(options.strokeGradient);
        }
        else
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
        }

        if (options.fillGradient.type != GRADIENT_NONE)
            element->SetFillGradient(options.fillGradient);
        else
            element->SetFill(options.fillColor, options.fillAlpha);

        element->SetRadii(options.radiusX, options.radiusY);
        element->SetLinePoints(options.startX, options.startY, options.endX, options.endY);
        element->SetArcParams(options.startArcAngle, options.endArcAngle, options.arcClockwise);
        element->SetPathData(options.pathData);
        element->SetCurveParams(
            options.startX,
            options.startY,
            options.controlX,
            options.controlY,
            options.control2X,
            options.control2Y,
            options.endX,
            options.endY,
            options.curveType);

        element->SetStrokeStyle(
            options.strokeStartCap,
            options.strokeEndCap,
            options.strokeDashCap,
            options.strokeLineJoin,
            options.strokeDashOffset,
            options.strokeDashes);
    }

    void PreFillElementOptions(ElementOptions &options, Element *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.hasPixelHitTest = true;
        options.pixelHitTest = element->GetPixelHitTest();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelGradient = element->GetBevelGradient();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();
        options.bevelGradient2 = element->GetBevelGradient2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.tooltipDisabled = element->GetToolTipDisabled();
    }

    void PreFillGeneralImageOptions(GeneralImageOptions &options, GeneralImage *image)
    {
        if (!image) return;
        options.imageFlip = image->GetImageFlip();
        options.hasImageCrop = image->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = image->GetImageCropX();
            options.imageCropY = image->GetImageCropY();
            options.imageCropW = image->GetImageCropW();
            options.imageCropH = image->GetImageCropH();
            options.imageCropOrigin = image->GetImageCropOrigin();
        }
        options.useExifOrientation = image->GetUseExifOrientation();
        options.imageAlpha = image->GetImageAlpha();
        options.grayscale = image->IsGrayscale();
        if (image->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = image->GetImageTint();
            options.imageTintAlpha = image->GetImageTintAlpha();
        }
        if (image->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = image->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillImageOptions(ImageOptions &options, ImageElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);
        options.path = element->GetImagePath();
        options.preserveAspectRatio = element->GetPreserveAspectRatio();
        options.tile = element->IsTile();

        options.imageFlip = element->GetImageFlip();
        options.hasImageCrop = element->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = element->GetImageCropX();
            options.imageCropY = element->GetImageCropY();
            options.imageCropW = element->GetImageCropW();
            options.imageCropH = element->GetImageCropH();
            options.imageCropOrigin = element->GetImageCropOrigin();
        }
        options.hasScaleMargins = element->HasScaleMargins();
        if (options.hasScaleMargins)
        {
            options.scaleMarginLeft = element->GetScaleMarginLeft();
            options.scaleMarginTop = element->GetScaleMarginTop();
            options.scaleMarginRight = element->GetScaleMarginRight();
            options.scaleMarginBottom = element->GetScaleMarginBottom();
        }
        options.useExifOrientation = element->GetUseExifOrientation();
        options.imageAlpha = element->GetImageAlpha();
        options.grayscale = element->IsGrayscale();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillButtonOptions(ButtonOptions &options, ButtonElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);

        options.buttonImageName = element->GetImagePath();
        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        options.hasImageCrop = element->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = element->GetImageCropX();
            options.imageCropY = element->GetImageCropY();
            options.imageCropW = element->GetImageCropW();
            options.imageCropH = element->GetImageCropH();
            options.imageCropOrigin = element->GetImageCropOrigin();
        }
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillBitmapOptions(BitmapOptions &options, BitmapElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        // Keep width/height invalid for bitmap element options.
        options.width = 0;
        options.height = 0;

        options.value = element->GetValue();
        options.bitmapImageName = element->GetImagePath();
        options.bitmapFrames = element->GetBitmapFrames();
        options.bitmapZeroFrame = element->GetBitmapZeroFrame();
        options.bitmapExtend = element->GetBitmapExtend();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();
        options.bitmapOrientation = element->GetBitmapOrientation();
        options.bitmapDigits = element->GetBitmapDigits();
        options.bitmapAlign = element->GetBitmapAlign();
        options.bitmapSeparation = element->GetBitmapSeparation();

        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillRotatorOptions(RotatorOptions &options, RotatorElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        options.value = element->GetValue();
        options.rotatorImageName = element->GetImagePath();
        options.offsetX = element->GetOffsetX();
        options.offsetY = element->GetOffsetY();
        options.startAngle = element->GetStartAngle();
        options.rotationAngle = element->GetRotationAngle();
        options.valueRemainder = element->GetValueRemainder();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();

        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillAreaGraphOptions(AreaGraphOptions &options, AreaGraphElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);
        options.data = element->GetData();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();
        options.autoRange = element->GetAutoRange();
        options.lineColor = element->GetLineColor();
        options.lineGradient = element->GetLineGradient();
        options.lineWidth = element->GetLineWidth();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.fillGradient = element->GetFillGradient();
        options.maxPoints = element->GetMaxPoints();
        options.gridColor = element->GetGridColor();
        options.gridAlpha = element->GetGridAlpha();
        options.gridGradient = element->GetGridGradient();
        options.gridVisible = element->GetGridVisible();
        options.gridX = element->GetGridXSpacing();
        options.gridY = element->GetGridYSpacing();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.flip = element->GetFlip();
    }

    void PreFillTextOptions(TextOptions &options, TextElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);

        options.text = element->GetText();
        options.fontFace = element->GetFontFace();
        options.fontSize = element->GetFontSize();
        options.fontColor = element->GetFontColor();
        options.alpha = element->GetFontAlpha();
        options.fontWeight = element->GetFontWeight();
        options.italic = element->IsItalic();
        options.textAlign = element->GetTextAlign();
        options.clip = element->GettextClip();
        options.fontPath = element->GetFontPath();
        options.shadows = element->GetShadows();
        options.fontGradient = element->GetFontGradient();
        options.letterSpacing = element->GetLetterSpacing();
        options.underLine = element->GetUnderline();
        options.strikeThrough = element->GetStrikethrough();
        options.textCase = element->GetTextCase();
    }

    void PreFillBarOptions(BarOptions &options, BarElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.value = element->GetValue();
        options.orientation = element->GetOrientation();
        options.barCornerRadius = element->GetBarCornerRadius();
        options.hasBarColor = element->HasBarColor();
        options.barColor = element->GetBarColor();
        options.barAlpha = element->GetBarAlpha();
        options.barGradient = element->GetBarGradient();
    }

    void PreFillLineOptions(LineOptions &options, LineElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }

        options.lineCount = element->GetLineCount();
        options.dataSets = element->GetDataSets();
        options.lineColors = element->GetLineColors();
        options.lineAlphas = element->GetLineAlphas();
        options.lineGradients = element->GetLineGradients();
        options.scaleValues = element->GetScaleValues();
        options.lineWidth = element->GetLineWidth();
        options.maxPoints = element->GetMaxPoints();
        options.horizontalLines = element->GetHorizontalLines();
        options.horizontalLineColor = element->GetHorizontalLineColor();
        options.horizontalLineAlpha = element->GetHorizontalLineAlpha();
        options.horizontalLineGradient = element->GetHorizontalLineGradient();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.graphHorizontalOrientation = element->GetGraphHorizontalOrientation();
        options.flip = element->GetFlip();
        options.transformStroke = element->GetStrokeTransformType();
        options.autoRange = element->GetAutoRange();
        options.scaleMin = element->GetScaleMin();
        options.scaleMax = element->GetScaleMax();
    }

    void PreFillHistogramOptions(HistogramOptions &options, HistogramElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        options.data = element->GetData();
        options.data2 = element->GetData2();
        options.autoRange = element->GetAutoRange();
        options.graphStartLeft = element->GetGraphStartLeft();
        options.graphHorizontalOrientation = element->GetGraphHorizontalOrientation();
        options.flip = element->GetFlip();

        options.primaryColor = element->GetPrimaryColor();
        options.primaryAlpha = element->GetPrimaryAlpha();
        options.secondaryColor = element->GetSecondaryColor();
        options.secondaryAlpha = element->GetSecondaryAlpha();
        options.bothColor = element->GetBothColor();
        options.bothAlpha = element->GetBothAlpha();
        options.primaryGradient = element->GetPrimaryGradient();
        options.secondaryGradient = element->GetSecondaryGradient();
        options.bothGradient = element->GetBothGradient();
    }

    void PreFillRoundLineOptions(RoundLineOptions &options, RoundLineElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.value = element->GetValue();
        options.radius = element->GetRadius();
        options.thickness = element->GetThickness();
        options.endThickness = element->GetEndThickness();
        options.startAngle = element->GetStartAngle();
        options.totalAngle = element->GetTotalAngle();
        options.clockwise = element->IsClockwise();
        options.startCap = element->GetStartCap();
        options.endCap = element->GetEndCap();
        options.dashArray = element->GetDashArray();
        options.ticks = element->GetTicks();
        options.hasLineColor = element->HasLineColor();
        options.lineColor = element->GetLineColor();
        options.lineAlpha = element->GetLineAlpha();
        options.hasLineColorBg = element->HasLineColorBg();
        options.lineColorBg = element->GetLineColorBg();
        options.lineAlphaBg = element->GetLineAlphaBg();
        options.lineGradient = element->GetLineGradient();
        options.lineGradientBg = element->GetLineGradientBg();
    }

    void PreFillShapeOptions(ShapeOptions &options, ShapeElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.strokeWidth = element->GetStrokeWidth();
        options.strokeColor = element->GetStrokeColor();
        options.strokeAlpha = element->GetStrokeAlpha();
        options.strokeGradient = element->GetStrokeGradient();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.fillGradient = element->GetFillGradient();
        options.radiusX = element->GetRadiusX();
        options.radiusY = element->GetRadiusY();
        options.startX = element->GetStartX();
        options.startY = element->GetStartY();
        options.endX = element->GetEndX();
        options.endY = element->GetEndY();
        options.curveType = element->GetCurveType();
        options.controlX = element->GetControlX();
        options.controlY = element->GetControlY();
        options.control2X = element->GetControl2X();
        options.control2Y = element->GetControl2Y();
        options.startArcAngle = element->GetStartAngle();
        options.endArcAngle = element->GetEndAngle();
        options.arcClockwise = element->IsClockwise();
        options.pathData = element->GetPathData();
        options.strokeStartCap = element->GetStrokeStartCap();
        options.strokeEndCap = element->GetStrokeEndCap();
        options.strokeDashCap = element->GetStrokeDashCap();
        options.strokeLineJoin = element->GetStrokeLineJoin();
        options.strokeDashOffset = element->GetStrokeDashOffset();
        options.strokeDashes = element->GetStrokeDashes();
    }

    void ParseGeneralImageOptions(duk_context *, GeneralImageOptions &) {}
    void ParseElementOptions(duk_context *, ElementOptions &) {}
    void ParseImageOptions(duk_context *ctx, ImageOptions &options)
    {
        ParseElementOptions(ctx, options);
        ParseGeneralImageOptions(ctx, options);
    }
    void ParseTextOptions(duk_context *, TextOptions &) {}
    void ParseBarOptions(duk_context *, BarOptions &) {}
    void ParseLineOptions(duk_context *, LineOptions &) {}
    void ParseHistogramOptions(duk_context *, HistogramOptions &) {}
    void ParseRoundLineOptions(duk_context *, RoundLineOptions &) {}
    void ParseShapeOptions(duk_context *, ShapeOptions &) {}
    void ParseButtonOptions(duk_context *ctx, ButtonOptions &options)
    {
        ParseElementOptions(ctx, options);
        ParseGeneralImageOptions(ctx, options);
    }
    void ParseBitmapOptions(duk_context *ctx, BitmapOptions &options)
    {
        ParseElementOptions(ctx, options);
        ParseGeneralImageOptions(ctx, options);
        options.width = 0;
        options.height = 0;
    }

    void ParseRotatorOptions(duk_context *ctx, RotatorOptions &options)
    {
        ParseElementOptions(ctx, options);
        ParseGeneralImageOptions(ctx, options);
    }

    void ParseAreaGraphOptions(duk_context *ctx, AreaGraphOptions &options)
    {
        ParseElementOptions(ctx, options);
    }
}

namespace novadesk::scripting::quickjs::parser
{
    void ParseWidgetWindowOptions(JSContext *ctx, JSValueConst options, WidgetWindowOptions &out)
    {
        if (!JS_IsObject(options))
        {
            return;
        }

        std::wstring id = PropertyParser::GetStringProp(ctx, options, "id");
        if (!id.empty())
            out.id = id;

        int32_t v = 0;
        JSValue widthVal = JS_GetPropertyStr(ctx, options, "width");
        if (!JS_IsUndefined(widthVal) && !JS_IsNull(widthVal) && JS_ToInt32(ctx, &v, widthVal) == 0)
        {
            out.width = static_cast<int>(v);
            out.hasWidth = true;
        }
        JS_FreeValue(ctx, widthVal);

        JSValue heightVal = JS_GetPropertyStr(ctx, options, "height");
        if (!JS_IsUndefined(heightVal) && !JS_IsNull(heightVal) && JS_ToInt32(ctx, &v, heightVal) == 0)
        {
            out.height = static_cast<int>(v);
            out.hasHeight = true;
        }
        JS_FreeValue(ctx, heightVal);

        JSValue xVal = JS_GetPropertyStr(ctx, options, "x");
        if (!JS_IsUndefined(xVal) && JS_ToInt32(ctx, &v, xVal) == 0)
        {
            out.x = static_cast<int>(v);
            out.hasX = true;
        }
        JS_FreeValue(ctx, xVal);

        JSValue yVal = JS_GetPropertyStr(ctx, options, "y");
        if (!JS_IsUndefined(yVal) && JS_ToInt32(ctx, &v, yVal) == 0)
        {
            out.y = static_cast<int>(v);
            out.hasY = true;
        }
        JS_FreeValue(ctx, yVal);

        out.hasDraggable = PropertyParser::GetBoolProp(ctx, options, "draggable", out.draggable);
        out.hasClickThrough = PropertyParser::GetBoolProp(ctx, options, "clickThrough", out.clickThrough);
        out.hasKeepOnScreen = PropertyParser::GetBoolProp(ctx, options, "keepOnScreen", out.keepOnScreen);
        out.hasSnapEdges = PropertyParser::GetBoolProp(ctx, options, "snapEdges", out.snapEdges);
        out.hasShowInToolbar = PropertyParser::GetBoolProp(ctx, options, "showInToolbar", out.showInToolbar);

        JSValue toolbarIconVal = JS_GetPropertyStr(ctx, options, "toolbarIcon");
        if (!JS_IsUndefined(toolbarIconVal) && !JS_IsNull(toolbarIconVal))
        {
            const char *s = JS_ToCString(ctx, toolbarIconVal);
            if (s)
            {
                out.toolbarIcon = Utils::ToWString(s);
                out.hasToolbarIcon = true;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, toolbarIconVal);

        JSValue toolbarTitleVal = JS_GetPropertyStr(ctx, options, "toolbarTitle");
        if (!JS_IsUndefined(toolbarTitleVal) && !JS_IsNull(toolbarTitleVal))
        {
            const char *s = JS_ToCString(ctx, toolbarTitleVal);
            if (s)
            {
                out.toolbarTitle = Utils::ToWString(s);
                out.hasToolbarTitle = true;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, toolbarTitleVal);

        out.hasShow = PropertyParser::GetBoolProp(ctx, options, "show", out.show);

        std::wstring bg = PropertyParser::GetStringProp(ctx, options, "backgroundColor");
        if (!bg.empty())
        {
            out.backgroundColor = bg;
            bool hasBg = false;
            PropertyParser::ParseGradientOrColor(bg, out.color, out.bgAlpha, out.bgGradient, hasBg);
            out.hasBackgroundColor = true;
        }

        JSValue opacityVal = JS_GetPropertyStr(ctx, options, "opacity");
        if (!JS_IsUndefined(opacityVal) && !JS_IsNull(opacityVal))
        {
            if (JS_IsString(opacityVal))
            {
                const char *s = JS_ToCString(ctx, opacityVal);
                if (s)
                {
                    std::wstring ws = Utils::ToWString(s);
                    JS_FreeCString(ctx, s);
                    ws.erase(std::remove_if(ws.begin(), ws.end(), iswspace), ws.end());
                    if (!ws.empty() && ws.back() == L'%')
                    {
                        ws.pop_back();
                        try
                        {
                            float pct = std::stof(ws);
                            pct = std::max(0.0f, std::min(100.0f, pct));
                            out.windowOpacity = static_cast<BYTE>((pct / 100.0f) * 255.0f);
                            out.hasWindowOpacity = true;
                        }
                        catch (...)
                        {
                        }
                    }
                    else
                    {
                        try
                        {
                            float val = std::stof(ws);
                            if (val <= 1.0f)
                                val *= 255.0f;
                            val = std::max(0.0f, std::min(255.0f, val));
                            out.windowOpacity = static_cast<BYTE>(val);
                            out.hasWindowOpacity = true;
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
            else
            {
                double d = 1.0;
                if (JS_ToFloat64(ctx, &d, opacityVal) == 0)
                {
                    if (d <= 1.0)
                        d *= 255.0;
                    d = std::max(0.0, std::min(255.0, d));
                    out.windowOpacity = static_cast<BYTE>(d);
                    out.hasWindowOpacity = true;
                }
            }
        }
        JS_FreeValue(ctx, opacityVal);

        std::wstring zPosStr = PropertyParser::GetStringProp(ctx, options, "zPos");
        std::transform(zPosStr.begin(), zPosStr.end(), zPosStr.begin(), ::towlower);
        if (zPosStr == L"ondesktop")
        {
            out.zPos = -2;
            out.hasZPos = true;
        }
        else if (zPosStr == L"onbottom")
        {
            out.zPos = -1;
            out.hasZPos = true;
        }
        else if (zPosStr == L"normal")
        {
            out.zPos = 0;
            out.hasZPos = true;
        }
        else if (zPosStr == L"ontop")
        {
            out.zPos = 1;
            out.hasZPos = true;
        }
        else if (zPosStr == L"ontopmost")
        {
            out.zPos = 2;
            out.hasZPos = true;
        }

        out.scriptPath = PropertyParser::GetStringProp(ctx, options, "script");
        out.hasScriptPath = !out.scriptPath.empty();
    }

    void ParseWidgetWindowSize(JSContext *ctx, JSValueConst options, int &width, int &height)
    {
        WidgetWindowOptions parsed;
        ParseWidgetWindowOptions(ctx, options, parsed);
        width = parsed.width;
        height = parsed.height;
    }
} // namespace novadesk::scripting::quickjs::parser


