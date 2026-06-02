/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cwctype>
#include <string>
namespace PropertyParser
{
    using namespace Js;
    void ParseElementOptions(JSContext *ctx, JSValueConst obj, ElementOptions &options, const std::wstring &baseDir)
    {
        if (!JS_IsObject(obj))
            return;

        options.id = GetStringProp(ctx, obj, "id");
        GetIntProp(ctx, obj, "x", options.x);
        GetIntProp(ctx, obj, "y", options.y);
        GetIntProp(ctx, obj, "width", options.width);
        GetIntProp(ctx, obj, "height", options.height);
        GetFloatProp(ctx, obj, "rotate", options.rotate);

        std::wstring bg = GetStringProp(ctx, obj, "backgroundColor");
        ParseGradientOrColor(bg, options.solidColor, options.solidAlpha, options.solidGradient, options.hasSolidColor);
        GetIntProp(ctx, obj, "backgroundColorRadius", options.solidColorRadius);

        std::wstring bevelType = GetStringProp(ctx, obj, "bevelType");
        if (bevelType == L"raised")
            options.bevelType = 1;
        else if (bevelType == L"sunken")
            options.bevelType = 2;
        else if (bevelType == L"emboss")
            options.bevelType = 3;
        else if (bevelType == L"pillow")
            options.bevelType = 4;
        else if (!bevelType.empty())
            options.bevelType = 0;

        GetIntProp(ctx, obj, "bevelWidth", options.bevelWidth);
        std::wstring b1 = GetStringProp(ctx, obj, "bevelColor");
        if (!b1.empty())
        {
            bool hasBevelColor = false;
            ParseGradientOrColor(b1, options.bevelColor, options.bevelAlpha, options.bevelGradient, hasBevelColor);
        }
        std::wstring b2 = GetStringProp(ctx, obj, "bevelColor2");
        if (!b2.empty())
        {
            bool hasBevelColor2 = false;
            ParseGradientOrColor(b2, options.bevelColor2, options.bevelAlpha2, options.bevelGradient2, hasBevelColor2);
        }

        JSValue pad = JS_GetPropertyStr(ctx, obj, "padding");
        if (JS_IsNumber(pad))
        {
            int32_t p = 0;
            if (JS_ToInt32(ctx, &p, pad) == 0)
            {
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = p;
            }
        }
        else if (JS_IsArray(pad))
        {
            std::vector<float> arr;
            if (GetFloatArrayProp(ctx, obj, "padding", arr, 1))
            {
                if (arr.size() >= 4)
                {
                    options.paddingLeft = static_cast<int>(arr[0]);
                    options.paddingTop = static_cast<int>(arr[1]);
                    options.paddingRight = static_cast<int>(arr[2]);
                    options.paddingBottom = static_cast<int>(arr[3]);
                }
                else if (arr.size() >= 2)
                {
                    options.paddingLeft = options.paddingRight = static_cast<int>(arr[0]);
                    options.paddingTop = options.paddingBottom = static_cast<int>(arr[1]);
                }
            }
        }
        JS_FreeValue(ctx, pad);

        GetBoolProp(ctx, obj, "antiAlias", options.antialias);
        if (GetBoolProp(ctx, obj, "pixelHitTest", options.pixelHitTest))
            options.hasPixelHitTest = true;
        GetBoolProp(ctx, obj, "show", options.show);
        std::wstring containerId = GetStringProp(ctx, obj, "container");
        if (!containerId.empty())
            options.containerId = containerId;

        std::wstring groupId = GetStringProp(ctx, obj, "group");
        if (!groupId.empty())
            options.groupId = groupId;

        GetBoolProp(ctx, obj, "mouseEventCursor", options.mouseEventCursor);

        std::wstring mouseEventCursorName = GetStringProp(ctx, obj, "mouseEventCursorName");
        if (!mouseEventCursorName.empty())
            options.mouseEventCursorName = mouseEventCursorName;
        std::wstring cursorsDir = GetStringProp(ctx, obj, "cursorsDir");
        if (!cursorsDir.empty())
        {
            options.cursorsDir = PathUtils::ResolvePath(cursorsDir, baseDir);
        }

        if (GetFloatArrayProp(ctx, obj, "transformMatrix", options.transformMatrix, 6))
        {
            options.hasTransformMatrix = true;
        }

        GetEventCallbackProp(ctx, obj, "onLeftMouseUp", options.onLeftMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftMouseDown", options.onLeftMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftDoubleClick", options.onLeftDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseUp", options.onRightMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseDown", options.onRightMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightDoubleClick", options.onRightDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseUp", options.onMiddleMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseDown", options.onMiddleMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleDoubleClick", options.onMiddleDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseUp", options.onX1MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseDown", options.onX1MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1DoubleClick", options.onX1DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseUp", options.onX2MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseDown", options.onX2MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2DoubleClick", options.onX2DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollUp", options.onScrollUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollDown", options.onScrollDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollLeft", options.onScrollLeftCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollRight", options.onScrollRightCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseOver", options.onMouseOverCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseLeave", options.onMouseLeaveCallbackId);
        GetEventCallbackProp(ctx, obj, "onDragStart", options.onDragStartCallbackId);
        GetEventCallbackProp(ctx, obj, "onDrag", options.onDragCallbackId);
        GetEventCallbackProp(ctx, obj, "onDragEnd", options.onDragEndCallbackId);

        std::wstring tooltipText = GetStringProp(ctx, obj, "tooltipText");
        if (!tooltipText.empty())
            options.tooltipText = tooltipText;

        std::wstring tooltipTitle = GetStringProp(ctx, obj, "tooltipTitle");
        if (!tooltipTitle.empty())
            options.tooltipTitle = tooltipTitle;

        std::wstring tooltipIcon = GetStringProp(ctx, obj, "tooltipIcon");
        if (!tooltipIcon.empty())
            options.tooltipIcon = tooltipIcon;

        GetIntProp(ctx, obj, "tooltipMaxWidth", options.tooltipMaxWidth);
        GetIntProp(ctx, obj, "tooltipMaxHeight", options.tooltipMaxHeight);
        GetBoolProp(ctx, obj, "tooltipBalloon", options.tooltipBalloon);
        GetBoolProp(ctx, obj, "tooltipDisabled", options.tooltipDisabled);
    }
    void ApplyElementOptions(Element *element, const ElementOptions &options)
    {
        if (!element)
            return;
        element->SetPosition(options.x, options.y);
        element->SetSize(options.width, options.height);
        element->SetRotate(options.rotate);
        element->SetAntiAlias(options.antialias);
        if (options.hasPixelHitTest)
            element->SetPixelHitTest(options.pixelHitTest);
        element->SetShow(options.show);
        element->SetContainerId(options.containerId);
        element->SetGroupId(options.groupId);
        element->SetMouseEventCursor(options.mouseEventCursor);
        element->SetMouseEventCursorName(options.mouseEventCursorName);
        element->SetCursorsDir(options.cursorsDir);
        element->SetCornerRadius(options.solidColorRadius);
        element->SetPadding(options.paddingLeft, options.paddingTop, options.paddingRight, options.paddingBottom);

        if (options.solidGradient.type != GRADIENT_NONE)
        {
            element->SetSolidGradient(options.solidGradient);
        }
        else if (options.hasSolidColor)
        {
            element->SetSolidColor(options.solidColor, options.solidAlpha);
        }

        if (options.bevelType > 0)
        {
            element->SetBevel(options.bevelType, options.bevelWidth, options.bevelColor, options.bevelAlpha, options.bevelColor2, options.bevelAlpha2);
            element->SetBevelGradient(options.bevelGradient);
            element->SetBevelGradient2(options.bevelGradient2);
        }
        else
        {
            element->SetBevel(0, 0, 0, 0, 0, 0);
            element->SetBevelGradient(GradientInfo());
            element->SetBevelGradient2(GradientInfo());
        }

        if (options.hasTransformMatrix && options.transformMatrix.size() >= 6)
        {
            element->SetTransformMatrix(options.transformMatrix.data());
        }

        if (options.onLeftMouseUpCallbackId != -1)
            element->m_OnLeftMouseUpCallbackId = options.onLeftMouseUpCallbackId;
        if (options.onLeftMouseDownCallbackId != -1)
            element->m_OnLeftMouseDownCallbackId = options.onLeftMouseDownCallbackId;
        if (options.onLeftDoubleClickCallbackId != -1)
            element->m_OnLeftDoubleClickCallbackId = options.onLeftDoubleClickCallbackId;
        if (options.onRightMouseUpCallbackId != -1)
            element->m_OnRightMouseUpCallbackId = options.onRightMouseUpCallbackId;
        if (options.onRightMouseDownCallbackId != -1)
            element->m_OnRightMouseDownCallbackId = options.onRightMouseDownCallbackId;
        if (options.onRightDoubleClickCallbackId != -1)
            element->m_OnRightDoubleClickCallbackId = options.onRightDoubleClickCallbackId;
        if (options.onMiddleMouseUpCallbackId != -1)
            element->m_OnMiddleMouseUpCallbackId = options.onMiddleMouseUpCallbackId;
        if (options.onMiddleMouseDownCallbackId != -1)
            element->m_OnMiddleMouseDownCallbackId = options.onMiddleMouseDownCallbackId;
        if (options.onMiddleDoubleClickCallbackId != -1)
            element->m_OnMiddleDoubleClickCallbackId = options.onMiddleDoubleClickCallbackId;
        if (options.onX1MouseUpCallbackId != -1)
            element->m_OnX1MouseUpCallbackId = options.onX1MouseUpCallbackId;
        if (options.onX1MouseDownCallbackId != -1)
            element->m_OnX1MouseDownCallbackId = options.onX1MouseDownCallbackId;
        if (options.onX1DoubleClickCallbackId != -1)
            element->m_OnX1DoubleClickCallbackId = options.onX1DoubleClickCallbackId;
        if (options.onX2MouseUpCallbackId != -1)
            element->m_OnX2MouseUpCallbackId = options.onX2MouseUpCallbackId;
        if (options.onX2MouseDownCallbackId != -1)
            element->m_OnX2MouseDownCallbackId = options.onX2MouseDownCallbackId;
        if (options.onX2DoubleClickCallbackId != -1)
            element->m_OnX2DoubleClickCallbackId = options.onX2DoubleClickCallbackId;
        if (options.onScrollUpCallbackId != -1)
            element->m_OnScrollUpCallbackId = options.onScrollUpCallbackId;
        if (options.onScrollDownCallbackId != -1)
            element->m_OnScrollDownCallbackId = options.onScrollDownCallbackId;
        if (options.onScrollLeftCallbackId != -1)
            element->m_OnScrollLeftCallbackId = options.onScrollLeftCallbackId;
        if (options.onScrollRightCallbackId != -1)
            element->m_OnScrollRightCallbackId = options.onScrollRightCallbackId;
        if (options.onMouseOverCallbackId != -1)
            element->m_OnMouseOverCallbackId = options.onMouseOverCallbackId;
        if (options.onMouseLeaveCallbackId != -1)
            element->m_OnMouseLeaveCallbackId = options.onMouseLeaveCallbackId;
        if (options.onDragStartCallbackId != -1)
            element->m_OnDragStartCallbackId = options.onDragStartCallbackId;
        if (options.onDragCallbackId != -1)
            element->m_OnDragCallbackId = options.onDragCallbackId;
        if (options.onDragEndCallbackId != -1)
            element->m_OnDragEndCallbackId = options.onDragEndCallbackId;

        if (!options.tooltipText.empty())
        {
            element->SetToolTip(
                options.tooltipText,
                options.tooltipTitle,
                options.tooltipIcon,
                options.tooltipMaxWidth,
                options.tooltipMaxHeight,
                options.tooltipBalloon);
        }
        element->SetToolTipDisabled(options.tooltipDisabled);
    }
    void PreFillElementOptions(ElementOptions &options, Element *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->IsWDefined() ? (element->GetWidth() - element->GetPaddingLeft() - element->GetPaddingRight()) : 0;
        options.height = element->IsHDefined() ? (element->GetHeight() - element->GetPaddingTop() - element->GetPaddingBottom()) : 0;

        options.show = element->IsVisible();
        options.containerId = element->GetContainerId();
        options.groupId = element->GetGroupId();
        options.mouseEventCursor = element->GetMouseEventCursor();
        options.mouseEventCursorName = element->GetMouseEventCursorName();
        options.cursorsDir = element->GetCursorsDir();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        options.hasPixelHitTest = true;
        options.pixelHitTest = element->GetPixelHitTest();
        options.solidColorRadius = element->GetCornerRadius();

        options.paddingLeft = element->GetPaddingLeft();
        options.paddingTop = element->GetPaddingTop();
        options.paddingRight = element->GetPaddingRight();
        options.paddingBottom = element->GetPaddingBottom();

        if (element->HasSolidColor())
        {
            options.hasSolidColor = true;
            options.solidColor = element->GetSolidColor();
            options.solidAlpha = element->GetSolidAlpha();
            options.solidGradient = element->GetSolidGradient();
        }

        options.bevelType = element->GetBevelType();
        options.bevelWidth = element->GetBevelWidth();
        options.bevelColor = element->GetBevelColor();
        options.bevelAlpha = element->GetBevelAlpha();
        options.bevelGradient = element->GetBevelGradient();
        options.bevelColor2 = element->GetBevelColor2();
        options.bevelAlpha2 = element->GetBevelAlpha2();
        options.bevelGradient2 = element->GetBevelGradient2();

        if (element->HasTransformMatrix())
        {
            options.hasTransformMatrix = true;
            const float *m = element->GetTransformMatrix();
            for (int i = 0; i < 6; ++i) options.transformMatrix[i] = m[i];
        }
        options.tooltipDisabled = element->GetToolTipDisabled();
    }
}
