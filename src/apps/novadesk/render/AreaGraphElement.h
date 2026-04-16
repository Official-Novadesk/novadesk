/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_AREA_GRAPH_ELEMENT_H__
#define __NOVADESK_AREA_GRAPH_ELEMENT_H__

#include "Element.h"
#include <vector>

class AreaGraphElement : public Element
{
public:
    AreaGraphElement(const std::wstring &id, int x, int y, int w, int h);
    virtual ~AreaGraphElement() {}

    virtual void Render(ID2D1DeviceContext *context) override;
    virtual int GetAutoWidth() override { return 0; }
    virtual int GetAutoHeight() override { return 0; }

    void SetData(const std::vector<float> &data);
    const std::vector<float> &GetData() const { return m_Data; }

    void SetMinValue(float minValue) { m_MinValue = minValue; }
    float GetMinValue() const { return m_MinValue; }
    void SetMaxValue(float maxValue) { m_MaxValue = maxValue; }
    float GetMaxValue() const { return m_MaxValue; }
    void SetAutoRange(bool enable) { m_AutoRange = enable; }
    bool GetAutoRange() const { return m_AutoRange; }

    void SetLineColor(COLORREF color) { m_LineColor = color; }
    COLORREF GetLineColor() const { return m_LineColor; }
    void SetLineGradient(const GradientInfo &gradient) { m_LineGradient = gradient; }
    const GradientInfo &GetLineGradient() const { return m_LineGradient; }
    
    void SetFillColor(COLORREF color, BYTE alpha) { m_FillColor = color; m_FillAlpha = alpha; }
    COLORREF GetFillColor() const { return m_FillColor; }
    BYTE GetFillAlpha() const { return m_FillAlpha; }
    void SetFillGradient(const GradientInfo &gradient) { m_FillGradient = gradient; }
    const GradientInfo &GetFillGradient() const { return m_FillGradient; }

    void SetGridColor(COLORREF color, BYTE alpha) { m_GridColor = color; m_GridAlpha = alpha; }
    COLORREF GetGridColor() const { return m_GridColor; }
    BYTE GetGridAlpha() const { return m_GridAlpha; }
    void SetGridGradient(const GradientInfo &gradient) { m_GridGradient = gradient; }
    const GradientInfo &GetGridGradient() const { return m_GridGradient; }
    void SetGridVisible(bool visible) { m_GridVisible = visible; }
    bool GetGridVisible() const { return m_GridVisible; }

    void SetLineWidth(float width) { m_LineWidth = (width < 0.1f) ? 0.1f : width; }
    float GetLineWidth() const { return m_LineWidth; }

    void SetMaxPoints(int maxPoints) { m_MaxPoints = (maxPoints < 0) ? 0 : maxPoints; }
    int GetMaxPoints() const { return m_MaxPoints; }

    void SetGridXSpacing(int spacing) { m_GridXSpacing = spacing; }
    int GetGridXSpacing() const { return m_GridXSpacing; }
    void SetGridYSpacing(int spacing) { m_GridYSpacing = spacing; }
    int GetGridYSpacing() const { return m_GridYSpacing; }

    void SetGraphStartLeft(bool left) { m_GraphStartLeft = left; }
    bool GetGraphStartLeft() const { return m_GraphStartLeft; }
    void SetFlip(bool flip) { m_Flip = flip; }
    bool GetFlip() const { return m_Flip; }

private:
    bool BuildAutoRange(float &outMin, float &outMax) const;

private:
    std::vector<float> m_Data;
    float m_MinValue = 0.0f;
    float m_MaxValue = 1.0f;
    bool m_AutoRange = false;
    int m_MaxPoints = 0;

    COLORREF m_LineColor = RGB(0, 180, 255);
    float m_LineWidth = 1.0f;
    GradientInfo m_LineGradient;

    COLORREF m_FillColor = RGB(0, 180, 255);
    BYTE m_FillAlpha = 50;
    GradientInfo m_FillGradient;

    COLORREF m_GridColor = RGB(100, 100, 100);
    BYTE m_GridAlpha = 100;
    GradientInfo m_GridGradient;
    bool m_GridVisible = true;

    int m_GridXSpacing = 20;
    int m_GridYSpacing = 20;

    bool m_GraphStartLeft = false; // newest on the right
    bool m_Flip = false;
};

#endif
