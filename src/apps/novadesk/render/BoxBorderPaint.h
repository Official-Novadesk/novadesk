/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "BoxBorderTypes.h"
#include <d2d1_1.h>

struct BoxBorderPaintParams
{
    BoxBorder::Position position = BoxBorder::Position::Inside;
    BoxBorder::Style styleTop = BoxBorder::Style::Solid;
    BoxBorder::Style styleRight = BoxBorder::Style::Solid;
    BoxBorder::Style styleBottom = BoxBorder::Style::Solid;
    BoxBorder::Style styleLeft = BoxBorder::Style::Solid;
    float elementRadiusX = 0.0f;
    float elementRadiusY = 0.0f;
    float strokeWidth = 0.0f;
    COLORREF strokeColor = RGB(0, 0, 0);
    BYTE strokeAlpha = 255;
    D2D1_CAP_STYLE strokeStartCap = D2D1_CAP_STYLE_FLAT;
    D2D1_CAP_STYLE strokeEndCap = D2D1_CAP_STYLE_FLAT;
    D2D1_CAP_STYLE strokeDashCap = D2D1_CAP_STYLE_FLAT;
    D2D1_LINE_JOIN strokeLineJoin = D2D1_LINE_JOIN_MITER;
    float strokeDashOffset = 0.0f;
};

class BoxBorderPaint
{
public:
    static D2D1_ROUNDED_RECT BuildBorderGeometryRect(const D2D1_ROUNDED_RECT& elementRect,
        const BoxBorderPaintParams& params);

    static void Paint(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& borderRect,
        const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush);

    static void PaintForElement(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& elementRect,
        const BoxBorderPaintParams& params, ID2D1Brush* strokeBrush);
};
