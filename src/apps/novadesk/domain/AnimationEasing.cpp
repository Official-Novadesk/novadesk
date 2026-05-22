/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "AnimationEasing.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace AnimationEasing
{
    float Evaluate(float t, const std::wstring &easing)
    {
        std::wstring e = easing;
        std::transform(e.begin(), e.end(), e.begin(), ::towlower);

        if (e == L"easeoutcubic")
        {
            const float inv = 1.0f - t;
            return 1.0f - inv * inv * inv;
        }
        if (e == L"easeinoutcubic")
        {
            return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        }
        if (e == L"easeoutquad")
        {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }
        if (e == L"easeinquad")
        {
            return t * t;
        }
        return t;
    }
}

