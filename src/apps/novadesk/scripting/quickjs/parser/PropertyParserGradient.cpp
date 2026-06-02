/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"

#include <algorithm>

#include "../../../shared/ColorUtil.h"

namespace PropertyParser
{
    using namespace Js;
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
}
