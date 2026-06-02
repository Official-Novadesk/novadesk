/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>

#include "Widget.h"

class WidgetAnimationHelper
{
public:
    static constexpr int kTimerId = 5;

    static void StartElementAnimation(
        Widget &widget,
        const std::wstring &id,
        const Widget::AnimationTarget &to,
        const Widget::AnimationTarget &from,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StartElementKeyframeAnimation(
        Widget &widget,
        const std::wstring &id,
        const std::vector<Widget::AnimationKeyframe> &keyframes,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StepAnimations(Widget &widget);
    static void RemoveAnimationsForElement(Widget &widget, const std::wstring &id);
    static void ClearAllAnimations(Widget &widget);

private:
    WidgetAnimationHelper() = delete;
};
