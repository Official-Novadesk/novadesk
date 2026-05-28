/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
#include "PropertyParserJs.h"

#include <algorithm>
#include <string>

#include "../../../shared/ColorUtil.h"
#include "../../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "PropertyParser.h"
#include "PropertyParserTypes.h"

namespace PropertyParser
{
namespace Js
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
        void ParseTextShadows(JSContext *ctx, JSValueConst obj, std::vector<TextShadow> &shadows)
        {
            JSValue val = JS_GetPropertyStr(ctx, obj, "fontShadow");
            if (JS_IsException(val) || JS_IsUndefined(val) || JS_IsNull(val))
            {
                JS_FreeValue(ctx, val);
                return;
            }

            shadows.clear();

            if (JS_IsArray(val))
            {
                uint32_t len = 0;
                JSValue lenV = JS_GetPropertyStr(ctx, val, "length");
                JS_ToUint32(ctx, &len, lenV);
                JS_FreeValue(ctx, lenV);

                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue item = JS_GetPropertyUint32(ctx, val, i);
                    if (JS_IsObject(item))
                    {
                        TextShadow s;
                        GetFloatProp(ctx, item, "x", s.offsetX);
                        GetFloatProp(ctx, item, "y", s.offsetY);
                        GetFloatProp(ctx, item, "blur", s.blur);
                        std::wstring colorStr = GetStringProp(ctx, item, "color");
                        if (!colorStr.empty())
                        {
                            ColorUtil::ParseRGBA(colorStr, s.color, s.alpha);
                        }
                        shadows.push_back(s);
                    }
                    JS_FreeValue(ctx, item);
                }
            }
            else if (JS_IsObject(val))
            {
                TextShadow s;
                GetFloatProp(ctx, val, "x", s.offsetX);
                GetFloatProp(ctx, val, "y", s.offsetY);
                GetFloatProp(ctx, val, "blur", s.blur);
                std::wstring colorStr = GetStringProp(ctx, val, "color");
                if (!colorStr.empty())
                {
                    ColorUtil::ParseRGBA(colorStr, s.color, s.alpha);
                }
                shadows.push_back(s);
            }

            JS_FreeValue(ctx, val);
        }
} // namespace Js
} // namespace PropertyParser

