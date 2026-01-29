/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_COLORUTIL_H__
#define __NOVADESK_COLORUTIL_H__

#include <windows.h>
#undef min
#undef max
#include <string>
#include <cstdio>
#include <algorithm>
#include <cwctype>

// Helper macros for color extraction from COLORREF (0x00BBGGRR)
#ifndef GetRValue
#define GetRValue(rgb)      (LOBYTE(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb)      (LOBYTE((WORD)(rgb) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb)      (LOBYTE((rgb) >> 16))
#endif

class ColorUtil
{
public:
    /*
    ** Parse a color string and extract color and alpha values.
    ** Supports formats: "rgba(r,g,b,a)", "#RRGGBB", "#RRGGBBAA", etc.
    ** Values: r,g,b are 0-255, alpha is 0-255 (defaults to 255 if not specified).
    ** Returns true on successful parse, false otherwise.
    */
    static bool ParseRGBA(const std::wstring& rgbaStr, COLORREF& color, BYTE& alpha)
    {
        if (rgbaStr.empty()) return false;

        // Strip whitespace for easier matching
        std::wstring s = rgbaStr;
        s.erase(std::remove_if(s.begin(), s.end(), isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);

        // 1. Handle Hex format: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
        if (s[0] == L'#')
        {
            std::wstring hex = s.substr(1);
            if (hex.length() == 3) // #RGB
            {
                int r, g, b;
                if (swscanf_s(hex.c_str(), L"%1x%1x%1x", &r, &g, &b) == 3)
                {
                    color = RGB(r * 17, g * 17, b * 17);
                    alpha = 255;
                    return true;
                }
            }
            else if (hex.length() == 4) // #RGBA
            {
                int r, g, b, a;
                if (swscanf_s(hex.c_str(), L"%1x%1x%1x%1x", &r, &g, &b, &a) == 4)
                {
                    color = RGB(r * 17, g * 17, b * 17);
                    alpha = (BYTE)(a * 17);
                    return true;
                }
            }
            else if (hex.length() == 6) // #RRGGBB
            {
                int r, g, b;
                if (swscanf_s(hex.c_str(), L"%2x%2x%2x", &r, &g, &b) == 3)
                {
                    color = RGB(r, g, b);
                    alpha = 255;
                    return true;
                }
            }
            else if (hex.length() == 8) // #RRGGBBAA
            {
                unsigned int r, g, b, a;
                if (swscanf_s(hex.c_str(), L"%2x%2x%2x%2x", &r, &g, &b, &a) == 4)
                {
                    color = RGB(r, g, b);
                    alpha = (BYTE)a;
                    return true;
                }
            }
            return false;
        }

        // 2. Handle rgba(r,g,b,alpha) or rgb(r,g,b)
        int r = 0, g = 0, b = 0;
        float af = 1.0f;
        
        if (swscanf_s(s.c_str(), L"rgba(%d,%d,%d,%f)", &r, &g, &b, &af) == 4)
        {
            color = RGB(r, g, b);
            // If af > 1.0, treat it as 0-255. 
            // This is a common pattern in desk-widgets where transparency is often 0-255.
            if (af > 1.1f) alpha = (BYTE)std::min(255.0f, af);
            else alpha = (BYTE)std::max(0.0f, std::min(255.0f, af * 255.0f));
            return true;
        }
        else if (swscanf_s(s.c_str(), L"rgb(%d,%d,%d)", &r, &g, &b) == 3)
        {
            color = RGB(r, g, b);
            alpha = 255;
            return true;
        }
        
        return false;
    }

    /*
    ** Convert a COLORREF and alpha value to an "rgba(r,g,b,a)" string.
    */
    static std::wstring ToRGBAString(COLORREF color, BYTE alpha)
    {
        wchar_t buf[64];
        swprintf_s(buf, L"rgba(%d,%d,%d,%.2f)", GetRValue(color), GetGValue(color), GetBValue(color), alpha / 255.0f);
        return std::wstring(buf);
    }
};

#endif
