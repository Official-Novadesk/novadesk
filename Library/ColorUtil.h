/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_COLORUTIL_H__
#define __NOVADESK_COLORUTIL_H__

#include <windows.h>
#include <string>
#include <cstdio>

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

        // 1. Handle Hex format: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
        if (rgbaStr[0] == L'#')
        {
            std::wstring hex = rgbaStr.substr(1);
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

        // 2. Handle rgba(r, g, b, alpha) or rgb(r, g, b)
        int r, g, b, a = 255;
        if (swscanf_s(rgbaStr.c_str(), L"rgba ( %d , %d , %d , %d )", &r, &g, &b, &a) == 4 ||
            swscanf_s(rgbaStr.c_str(), L"rgba(%d,%d,%d,%d)", &r, &g, &b, &a) == 4)
        {
            color = RGB(r, g, b);
            alpha = (BYTE)a;
            return true;
        }
        else if (swscanf_s(rgbaStr.c_str(), L"rgb ( %d , %d , %d )", &r, &g, &b) == 3 ||
                 swscanf_s(rgbaStr.c_str(), L"rgb(%d,%d,%d)", &r, &g, &b) == 3)
        {
            color = RGB(r, g, b);
            alpha = 255;
            return true;
        }
        return false;
    }
};

#endif
