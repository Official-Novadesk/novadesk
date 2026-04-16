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
#include <unordered_map>

// Helper macros for color extraction from COLORREF (0x00BBGGRR)
#ifndef GetRValue
#define GetRValue(rgb) (LOBYTE(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb) (LOBYTE((WORD)(rgb) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))
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
    static bool ParseRGBA(const std::wstring &rgbaStr, COLORREF &color, BYTE &alpha)
    {
        if (rgbaStr.empty())
            return false;

        // Strip whitespace for easier matching
        std::wstring s = rgbaStr;
        s.erase(std::remove_if(s.begin(), s.end(), isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);

        // 0. Named CSS colors (case-insensitive)
        static const std::unordered_map<std::wstring, COLORREF> kNamedColors = {
            {L"aliceblue", RGB(0xF0, 0xF8, 0xFF)},
            {L"antiquewhite", RGB(0xFA, 0xEB, 0xD7)},
            {L"aqua", RGB(0x00, 0xFF, 0xFF)},
            {L"aquamarine", RGB(0x7F, 0xFF, 0xD4)},
            {L"azure", RGB(0xF0, 0xFF, 0xFF)},
            {L"beige", RGB(0xF5, 0xF5, 0xDC)},
            {L"bisque", RGB(0xFF, 0xE4, 0xC4)},
            {L"black", RGB(0x00, 0x00, 0x00)},
            {L"blanchedalmond", RGB(0xFF, 0xEB, 0xCD)},
            {L"blue", RGB(0x00, 0x00, 0xFF)},
            {L"blueviolet", RGB(0x8A, 0x2B, 0xE2)},
            {L"brown", RGB(0xA5, 0x2A, 0x2A)},
            {L"burlywood", RGB(0xDE, 0xB8, 0x87)},
            {L"cadetblue", RGB(0x5F, 0x9E, 0xA0)},
            {L"chartreuse", RGB(0x7F, 0xFF, 0x00)},
            {L"chocolate", RGB(0xD2, 0x69, 0x1E)},
            {L"coral", RGB(0xFF, 0x7F, 0x50)},
            {L"cornflowerblue", RGB(0x64, 0x95, 0xED)},
            {L"cornsilk", RGB(0xFF, 0xF8, 0xDC)},
            {L"crimson", RGB(0xDC, 0x14, 0x3C)},
            {L"cyan", RGB(0x00, 0xFF, 0xFF)},
            {L"darkblue", RGB(0x00, 0x00, 0x8B)},
            {L"darkcyan", RGB(0x00, 0x8B, 0x8B)},
            {L"darkgoldenrod", RGB(0xB8, 0x86, 0x0B)},
            {L"darkgray", RGB(0xA9, 0xA9, 0xA9)},
            {L"darkgrey", RGB(0xA9, 0xA9, 0xA9)},
            {L"darkgreen", RGB(0x00, 0x64, 0x00)},
            {L"darkkhaki", RGB(0xBD, 0xB7, 0x6B)},
            {L"darkmagenta", RGB(0x8B, 0x00, 0x8B)},
            {L"darkolivegreen", RGB(0x55, 0x6B, 0x2F)},
            {L"darkorange", RGB(0xFF, 0x8C, 0x00)},
            {L"darkorchid", RGB(0x99, 0x32, 0xCC)},
            {L"darkred", RGB(0x8B, 0x00, 0x00)},
            {L"darksalmon", RGB(0xE9, 0x96, 0x7A)},
            {L"darkseagreen", RGB(0x8F, 0xBC, 0x8F)},
            {L"darkslateblue", RGB(0x48, 0x3D, 0x8B)},
            {L"darkslategray", RGB(0x2F, 0x4F, 0x4F)},
            {L"darkslategrey", RGB(0x2F, 0x4F, 0x4F)},
            {L"darkturquoise", RGB(0x00, 0xCE, 0xD1)},
            {L"darkviolet", RGB(0x94, 0x00, 0xD3)},
            {L"deeppink", RGB(0xFF, 0x14, 0x93)},
            {L"deepskyblue", RGB(0x00, 0xBF, 0xFF)},
            {L"dimgray", RGB(0x69, 0x69, 0x69)},
            {L"dimgrey", RGB(0x69, 0x69, 0x69)},
            {L"dodgerblue", RGB(0x1E, 0x90, 0xFF)},
            {L"firebrick", RGB(0xB2, 0x22, 0x22)},
            {L"floralwhite", RGB(0xFF, 0xFA, 0xF0)},
            {L"forestgreen", RGB(0x22, 0x8B, 0x22)},
            {L"fuchsia", RGB(0xFF, 0x00, 0xFF)},
            {L"gainsboro", RGB(0xDC, 0xDC, 0xDC)},
            {L"ghostwhite", RGB(0xF8, 0xF8, 0xFF)},
            {L"gold", RGB(0xFF, 0xD7, 0x00)},
            {L"goldenrod", RGB(0xDA, 0xA5, 0x20)},
            {L"gray", RGB(0x80, 0x80, 0x80)},
            {L"grey", RGB(0x80, 0x80, 0x80)},
            {L"green", RGB(0x00, 0x80, 0x00)},
            {L"greenyellow", RGB(0xAD, 0xFF, 0x2F)},
            {L"honeydew", RGB(0xF0, 0xFF, 0xF0)},
            {L"hotpink", RGB(0xFF, 0x69, 0xB4)},
            {L"indianred", RGB(0xCD, 0x5C, 0x5C)},
            {L"indigo", RGB(0x4B, 0x00, 0x82)},
            {L"ivory", RGB(0xFF, 0xFF, 0xF0)},
            {L"khaki", RGB(0xF0, 0xE6, 0x8C)},
            {L"lavender", RGB(0xE6, 0xE6, 0xFA)},
            {L"lavenderblush", RGB(0xFF, 0xF0, 0xF5)},
            {L"lawngreen", RGB(0x7C, 0xFC, 0x00)},
            {L"lemonchiffon", RGB(0xFF, 0xFA, 0xCD)},
            {L"lightblue", RGB(0xAD, 0xD8, 0xE6)},
            {L"lightcoral", RGB(0xF0, 0x80, 0x80)},
            {L"lightcyan", RGB(0xE0, 0xFF, 0xFF)},
            {L"lightgoldenrodyellow", RGB(0xFA, 0xFA, 0xD2)},
            {L"lightgray", RGB(0xD3, 0xD3, 0xD3)},
            {L"lightgrey", RGB(0xD3, 0xD3, 0xD3)},
            {L"lightgreen", RGB(0x90, 0xEE, 0x90)},
            {L"lightpink", RGB(0xFF, 0xB6, 0xC1)},
            {L"lightsalmon", RGB(0xFF, 0xA0, 0x7A)},
            {L"lightseagreen", RGB(0x20, 0xB2, 0xAA)},
            {L"lightskyblue", RGB(0x87, 0xCE, 0xFA)},
            {L"lightslategray", RGB(0x77, 0x88, 0x99)},
            {L"lightslategrey", RGB(0x77, 0x88, 0x99)},
            {L"lightsteelblue", RGB(0xB0, 0xC4, 0xDE)},
            {L"lightyellow", RGB(0xFF, 0xFF, 0xE0)},
            {L"lime", RGB(0x00, 0xFF, 0x00)},
            {L"limegreen", RGB(0x32, 0xCD, 0x32)},
            {L"linen", RGB(0xFA, 0xF0, 0xE6)},
            {L"magenta", RGB(0xFF, 0x00, 0xFF)},
            {L"maroon", RGB(0x80, 0x00, 0x00)},
            {L"mediumaquamarine", RGB(0x66, 0xCD, 0xAA)},
            {L"mediumblue", RGB(0x00, 0x00, 0xCD)},
            {L"mediumorchid", RGB(0xBA, 0x55, 0xD3)},
            {L"mediumpurple", RGB(0x93, 0x70, 0xDB)},
            {L"mediumseagreen", RGB(0x3C, 0xB3, 0x71)},
            {L"mediumslateblue", RGB(0x7B, 0x68, 0xEE)},
            {L"mediumspringgreen", RGB(0x00, 0xFA, 0x9A)},
            {L"mediumturquoise", RGB(0x48, 0xD1, 0xCC)},
            {L"mediumvioletred", RGB(0xC7, 0x15, 0x85)},
            {L"midnightblue", RGB(0x19, 0x19, 0x70)},
            {L"mintcream", RGB(0xF5, 0xFF, 0xFA)},
            {L"mistyrose", RGB(0xFF, 0xE4, 0xE1)},
            {L"moccasin", RGB(0xFF, 0xE4, 0xB5)},
            {L"navajowhite", RGB(0xFF, 0xDE, 0xAD)},
            {L"navy", RGB(0x00, 0x00, 0x80)},
            {L"oldlace", RGB(0xFD, 0xF5, 0xE6)},
            {L"olive", RGB(0x80, 0x80, 0x00)},
            {L"olivedrab", RGB(0x6B, 0x8E, 0x23)},
            {L"orange", RGB(0xFF, 0xA5, 0x00)},
            {L"orangered", RGB(0xFF, 0x45, 0x00)},
            {L"orchid", RGB(0xDA, 0x70, 0xD6)},
            {L"palegoldenrod", RGB(0xEE, 0xE8, 0xAA)},
            {L"palegreen", RGB(0x98, 0xFB, 0x98)},
            {L"paleturquoise", RGB(0xAF, 0xEE, 0xEE)},
            {L"palevioletred", RGB(0xDB, 0x70, 0x93)},
            {L"papayawhip", RGB(0xFF, 0xEF, 0xD5)},
            {L"peachpuff", RGB(0xFF, 0xDA, 0xB9)},
            {L"peru", RGB(0xCD, 0x85, 0x3F)},
            {L"pink", RGB(0xFF, 0xC0, 0xCB)},
            {L"plum", RGB(0xDD, 0xA0, 0xDD)},
            {L"powderblue", RGB(0xB0, 0xE0, 0xE6)},
            {L"purple", RGB(0x80, 0x00, 0x80)},
            {L"rebeccapurple", RGB(0x66, 0x33, 0x99)},
            {L"red", RGB(0xFF, 0x00, 0x00)},
            {L"rosybrown", RGB(0xBC, 0x8F, 0x8F)},
            {L"royalblue", RGB(0x41, 0x69, 0xE1)},
            {L"saddlebrown", RGB(0x8B, 0x45, 0x13)},
            {L"salmon", RGB(0xFA, 0x80, 0x72)},
            {L"sandybrown", RGB(0xF4, 0xA4, 0x60)},
            {L"seagreen", RGB(0x2E, 0x8B, 0x57)},
            {L"seashell", RGB(0xFF, 0xF5, 0xEE)},
            {L"sienna", RGB(0xA0, 0x52, 0x2D)},
            {L"silver", RGB(0xC0, 0xC0, 0xC0)},
            {L"skyblue", RGB(0x87, 0xCE, 0xEB)},
            {L"slateblue", RGB(0x6A, 0x5A, 0xCD)},
            {L"slategray", RGB(0x70, 0x80, 0x90)},
            {L"slategrey", RGB(0x70, 0x80, 0x90)},
            {L"snow", RGB(0xFF, 0xFA, 0xFA)},
            {L"springgreen", RGB(0x00, 0xFF, 0x7F)},
            {L"steelblue", RGB(0x46, 0x82, 0xB4)},
            {L"tan", RGB(0xD2, 0xB4, 0x8C)},
            {L"teal", RGB(0x00, 0x80, 0x80)},
            {L"thistle", RGB(0xD8, 0xBF, 0xD8)},
            {L"tomato", RGB(0xFF, 0x63, 0x47)},
            {L"turquoise", RGB(0x40, 0xE0, 0xD0)},
            {L"violet", RGB(0xEE, 0x82, 0xEE)},
            {L"wheat", RGB(0xF5, 0xDE, 0xB3)},
            {L"white", RGB(0xFF, 0xFF, 0xFF)},
            {L"whitesmoke", RGB(0xF5, 0xF5, 0xF5)},
            {L"yellow", RGB(0xFF, 0xFF, 0x00)},
            {L"yellowgreen", RGB(0x9A, 0xCD, 0x32)}
        };
        if (s == L"transparent")
        {
            color = RGB(0, 0, 0);
            alpha = 0;
            return true;
        }
        auto named = kNamedColors.find(s);
        if (named != kNamedColors.end())
        {
            color = named->second;
            alpha = 255;
            return true;
        }

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
            if (af > 1.1f)
                alpha = (BYTE)std::min(255.0f, af);
            else
                alpha = (BYTE)std::max(0.0f, std::min(255.0f, af * 255.0f));
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
