/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Utils.h"
#include <Windows.h>
#include "ColorUtil.h"
#include "JSApi/JSEvents.h"
#include "Logging.h"
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "version.lib")

namespace Utils {

    /*
    ** Convert a UTF-8 encoded std::string to a wide character std::wstring.
    ** Returns an empty wstring if the input string is empty.
    ** Uses Windows MultiByteToWideChar for conversion.
    */

    std::wstring ToWString(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    /*
    ** Convert a wide character std::wstring to a UTF-8 encoded std::string.
    ** Returns an empty string if the input wstring is empty.
    */

    std::string ToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    std::vector<std::wstring> SplitByComma(const std::wstring& s) {
        std::vector<std::wstring> parts;
        int depth = 0;
        size_t last = 0;
        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == L'(') depth++;
            else if (s[i] == L')') depth--;
            else if (s[i] == L',' && depth == 0) {
                parts.push_back(s.substr(last, i - last));
                last = i + 1;
            }
        }
        parts.push_back(s.substr(last));
        for (auto& p : parts) {
            p.erase(0, p.find_first_not_of(L' '));
            p.erase(p.find_last_not_of(L' ') + 1);
        }
        return parts;
    }

    bool ParseGradientString(const std::wstring& str, GradientInfo& out) {
        if (str.empty()) return false;
        std::wstring s = str;
        s.erase(0, s.find_first_not_of(L' '));
        s.erase(s.find_last_not_of(L' ') + 1);
        
        std::wstring lowerS = s;
        std::transform(lowerS.begin(), lowerS.end(), lowerS.begin(), ::towlower);

        if (lowerS.find(L"lineargradient(") == 0) out.type = GRADIENT_LINEAR;
        else if (lowerS.find(L"radialgradient(") == 0) out.type = GRADIENT_RADIAL;
        else return false;

        size_t start = lowerS.find(L'(') + 1;
        size_t end = lowerS.find_last_of(L')');
        if (end == std::wstring::npos || end <= start) return false;

        std::wstring content = s.substr(start, end - start);
        std::vector<std::wstring> parts = SplitByComma(content);
        if (parts.empty()) return false;

        int colorStartIndex = 0;
        if (out.type == GRADIENT_LINEAR) {
            std::wstring dir = parts[0];
            std::transform(dir.begin(), dir.end(), dir.begin(), ::towlower);
            dir.erase(std::remove_if(dir.begin(), dir.end(), isspace), dir.end());

            if (!dir.empty() && (iswdigit(dir[0]) || dir[0] == L'-' || dir[0] == L'.')) {
                try { 
                    size_t pos = 0;
                    out.angle = std::stof(dir, &pos);
                    if (pos > 0) colorStartIndex = 1;
                } catch(...) {}
            }
        } else {
             std::wstring shape = parts[0];
             std::transform(shape.begin(), shape.end(), shape.begin(), ::towlower);
             shape.erase(std::remove_if(shape.begin(), shape.end(), isspace), shape.end());

             if (shape == L"circle" || shape == L"ellipse") {
                 out.shape = shape;
                 colorStartIndex = 1;
             }
        }

        out.stops.clear();
        for (size_t i = colorStartIndex; i < parts.size(); i++) {
            GradientStop stop;
            if (ColorUtil::ParseRGBA(parts[i], stop.color, stop.alpha)) {
                out.stops.push_back(stop);
            }
        }

        if (out.stops.size() < 2) return false;

        for (size_t i = 0; i < out.stops.size(); i++) {
            out.stops[i].position = (float)i / (out.stops.size() - 1);
        }

        return true;
    }

    D2D1_CAP_STYLE GetCapStyle(const std::wstring& str) {
        if (str == L"Round") return D2D1_CAP_STYLE_ROUND;
        if (str == L"Square") return D2D1_CAP_STYLE_SQUARE;
        if (str == L"Triangle") return D2D1_CAP_STYLE_TRIANGLE;
        return D2D1_CAP_STYLE_FLAT;
    }

    D2D1_LINE_JOIN GetLineJoin(const std::wstring& str) {
        if (str == L"Bevel") return D2D1_LINE_JOIN_BEVEL;
        if (str == L"Round") return D2D1_LINE_JOIN_ROUND;
        if (str == L"MiterOrBevel") return D2D1_LINE_JOIN_MITER_OR_BEVEL;
        return D2D1_LINE_JOIN_MITER;
    }

    PropertyReader::PropertyReader(duk_context* ctx) : m_Ctx(ctx) {}

    bool PropertyReader::GetString(const char* key, std::wstring& outStr) {
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
        
    bool PropertyReader::GetInt(const char* key, int& outInt) {
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

    bool PropertyReader::GetFloat(const char* key, float& outFloat) {
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

    bool PropertyReader::GetBool(const char* key, bool& outBool) {
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

    bool PropertyReader::GetColor(const char* key, COLORREF& outColor, BYTE& outAlpha) {
        std::wstring colorStr;
        if (GetString(key, colorStr)) {
            return ColorUtil::ParseRGBA(colorStr, outColor, outAlpha);
        }
        return false;
    }

    bool PropertyReader::GetGradientOrColor(const char* key, COLORREF& outColor, BYTE& outAlpha, GradientInfo& outGradient) {
        std::wstring colorStr;
        if (GetString(key, colorStr)) {
            if (ParseGradientString(colorStr, outGradient)) {
                return true;
            }
            return ColorUtil::ParseRGBA(colorStr, outColor, outAlpha);
        }
        return false;
    }

    bool PropertyReader::GetFloatArray(const char* key, std::vector<float>& outArray, int minSize) {
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

    void PropertyReader::GetEvent(const char* key, int& outId) {
        if (duk_get_prop_string(m_Ctx, -1, key)) {
            if (duk_is_function(m_Ctx, -1)) {
                outId = JSApi::RegisterEventCallback(m_Ctx, -1);
            }
        }
        duk_pop(m_Ctx);
    }

    bool PropertyReader::ParseShadow(TextShadow& shadow) {
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

}
