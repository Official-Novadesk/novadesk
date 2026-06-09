/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include "BoxBorderPaint.h"
#include "ShapeElement.h"

class ElementLayoutBox : public ShapeElement
{
public:
    enum class DisplayType
    {
        Flex,
        None
    };

    struct BoxShadow
    {
        float x = 0.0f;
        float y = 0.0f;
        float blur = 0.0f;
        float spread = 0.0f;
        COLORREF color = RGB(0, 0, 0);
        BYTE alpha = 255;
        bool inset = false;
    };

    using BorderStyle = BoxBorder::Style;

    ElementLayoutBox(const std::wstring &id, int x, int y, int width, int height);
    ~ElementLayoutBox() override = default;

    void Render(ID2D1DeviceContext *context) override;
    bool HitTestLocal(const D2D1_POINT_2F &point) override;
    bool CreateGeometry(ID2D1Factory *factory, Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

    int GetAutoWidth() override;
    int GetAutoHeight() override;

    void SetRadii(float rx, float ry) override { m_RadiusX = rx; m_RadiusY = ry; }
    float GetRadiusX() const override { return m_RadiusX; }
    float GetRadiusY() const override { return m_RadiusY; }
    void SetBoxShadows(const std::vector<BoxShadow> &shadows) { m_BoxShadows = shadows; }
    const std::vector<BoxShadow> &GetBoxShadows() const { return m_BoxShadows; }

    void SetBorderStyle(BorderStyle top, BorderStyle right, BorderStyle bottom, BorderStyle left)
    {
        m_BorderStyleTop = top;
        m_BorderStyleRight = right;
        m_BorderStyleBottom = bottom;
        m_BorderStyleLeft = left;
    }
    BorderStyle GetBorderStyleTop() const { return m_BorderStyleTop; }
    BorderStyle GetBorderStyleRight() const { return m_BorderStyleRight; }
    BorderStyle GetBorderStyleBottom() const { return m_BorderStyleBottom; }
    BorderStyle GetBorderStyleLeft() const { return m_BorderStyleLeft; }

    void SetDisplayType(DisplayType display) { m_DisplayType = display; }
    DisplayType GetDisplayType() const { return m_DisplayType; }

    // Set layout configuration for auto-sizing calculations
    void SetLayoutDirection(const std::wstring& flexDir) { m_FlexDirection = flexDir; }
    void SetLayoutGap(int gap) { m_LayoutGap = gap; }
    const std::wstring& GetLayoutDirection() const { return m_FlexDirection; }
    int GetLayoutGap() const { return m_LayoutGap; }

private:
    void RenderSingleShadow(ID2D1DeviceContext *context, const D2D1_ROUNDED_RECT &baseRect, const BoxShadow &shadow);
    BoxBorderPaintParams BuildBorderPaintParams() const;

    float m_RadiusX = 0.0f;
    float m_RadiusY = 0.0f;
    std::vector<BoxShadow> m_BoxShadows;
    BorderStyle m_BorderStyleTop = BorderStyle::Solid;
    BorderStyle m_BorderStyleRight = BorderStyle::Solid;
    BorderStyle m_BorderStyleBottom = BorderStyle::Solid;
    BorderStyle m_BorderStyleLeft = BorderStyle::Solid;
    DisplayType m_DisplayType = DisplayType::Flex;
    std::wstring m_FlexDirection = L"row";
    int m_LayoutGap = 0;
};
