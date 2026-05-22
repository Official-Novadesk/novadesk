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
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float c1 = 1.70158f;
        constexpr float c2 = c1 * 1.525f;
        constexpr float c3 = c1 + 1.0f;
        constexpr float c4 = (2.0f * kPi) / 3.0f;
        constexpr float c5 = (2.0f * kPi) / 4.5f;

        float Clamp01(float x)
        {
            if (x < 0.0f) return 0.0f;
            if (x > 1.0f) return 1.0f;
            return x;
        }

        float BounceOut(float x)
        {
            constexpr float n1 = 7.5625f;
            constexpr float d1 = 2.75f;

            if (x < 1.0f / d1)
                return n1 * x * x;
            if (x < 2.0f / d1)
            {
                x -= 1.5f / d1;
                return n1 * x * x + 0.75f;
            }
            if (x < 2.5f / d1)
            {
                x -= 2.25f / d1;
                return n1 * x * x + 0.9375f;
            }
            x -= 2.625f / d1;
            return n1 * x * x + 0.984375f;
        }
    }

    float Evaluate(float t, const std::wstring &easing)
    {
        t = Clamp01(t);
        std::wstring e = easing;
        std::transform(e.begin(), e.end(), e.begin(), ::towlower);

        if (e == L"linear")
            return t;

        if (e == L"easeinquad")
            return t * t;
        if (e == L"easeoutquad")
            return 1.0f - (1.0f - t) * (1.0f - t);
        if (e == L"easeinoutquad")
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        if (e == L"easeincubic")
            return t * t * t;
        if (e == L"easeoutcubic")
            return 1.0f - std::pow(1.0f - t, 3.0f);
        if (e == L"easeinoutcubic")
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

        if (e == L"easeinquart")
            return t * t * t * t;
        if (e == L"easeoutquart")
            return 1.0f - std::pow(1.0f - t, 4.0f);
        if (e == L"easeinoutquart")
            return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;

        if (e == L"easeinquint")
            return t * t * t * t * t;
        if (e == L"easeoutquint")
            return 1.0f - std::pow(1.0f - t, 5.0f);
        if (e == L"easeinoutquint")
            return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;

        if (e == L"easeinsine")
            return 1.0f - std::cos((t * kPi) / 2.0f);
        if (e == L"easeoutsine")
            return std::sin((t * kPi) / 2.0f);
        if (e == L"easeinoutsine")
            return -(std::cos(kPi * t) - 1.0f) / 2.0f;

        if (e == L"easeinexpo")
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
        if (e == L"easeoutexpo")
            return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
        if (e == L"easeinoutexpo")
            return t == 0.0f
                       ? 0.0f
                       : t == 1.0f
                             ? 1.0f
                             : t < 0.5f
                                   ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                                   : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

        if (e == L"easeincirc")
            return 1.0f - std::sqrt(1.0f - std::pow(t, 2.0f));
        if (e == L"easeoutcirc")
            return std::sqrt(1.0f - std::pow(t - 1.0f, 2.0f));
        if (e == L"easeinoutcirc")
            return t < 0.5f
                       ? (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f
                       : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;

        if (e == L"easeinback")
            return c3 * t * t * t - c1 * t * t;
        if (e == L"easeoutback")
            return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
        if (e == L"easeinoutback")
            return t < 0.5f
                       ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                       : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;

        if (e == L"easeinelastic")
            return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
        if (e == L"easeoutelastic")
            return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        if (e == L"easeinoutelastic")
            return t == 0.0f
                       ? 0.0f
                       : t == 1.0f
                             ? 1.0f
                             : t < 0.5f
                                   ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
                                   : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;

        if (e == L"easeinbounce")
            return 1.0f - BounceOut(1.0f - t);
        if (e == L"easeoutbounce")
            return BounceOut(t);
        if (e == L"easeinoutbounce")
            return t < 0.5f ? (1.0f - BounceOut(1.0f - 2.0f * t)) / 2.0f : (1.0f + BounceOut(2.0f * t - 1.0f)) / 2.0f;

        return t;
    }
}
