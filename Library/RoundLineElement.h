/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ROUNDLINE_ELEMENT_H__
#define __NOVADESK_ROUNDLINE_ELEMENT_H__

#include "Element.h"

enum RoundLineCap {
    ROUNDLINE_CAP_FLAT = 0,
    ROUNDLINE_CAP_ROUND = 1
};

class RoundLineElement : public Element {
public:
    RoundLineElement(const std::wstring& id, int x, int y, int w, int h, float value);
    virtual ~RoundLineElement() {}

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual bool HitTest(int x, int y) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    float GetValue() const { return m_Value; }
    void SetValue(float value) { m_Value = value; }

    int GetRadius() const { return m_Radius; }
    void SetRadius(int radius) { m_Radius = radius; }

    int GetThickness() const { return m_Thickness; }
    void SetThickness(int thickness) { m_Thickness = thickness; }

    float GetStartAngle() const { return m_StartAngle; }
    void SetStartAngle(float angle) { m_StartAngle = angle; }

    float GetTotalAngle() const { return m_TotalAngle; }
    void SetTotalAngle(float angle) { m_TotalAngle = angle; }

    bool IsClockwise() const { return m_Clockwise; }
    void SetClockwise(bool clockwise) { m_Clockwise = clockwise; }

    RoundLineCap GetCapType() const { return m_StartCap; }
    void SetCapType(RoundLineCap cap) { m_StartCap = m_EndCap = cap; }

    RoundLineCap GetStartCap() const { return m_StartCap; }
    void SetStartCap(RoundLineCap cap) { m_StartCap = cap; }

    RoundLineCap GetEndCap() const { return m_EndCap; }
    void SetEndCap(RoundLineCap cap) { m_EndCap = cap; }

    const std::vector<float>& GetDashArray() const { return m_DashArray; }
    void SetDashArray(const std::vector<float>& dashArray) { m_DashArray = dashArray; }

    int GetEndThickness() const { return m_EndThickness; }
    void SetEndThickness(int thickness) { m_EndThickness = thickness; }

    int GetTicks() const { return m_Ticks; }
    void SetTicks(int ticks) { m_Ticks = ticks; }

    void SetLineColor(COLORREF color, BYTE alpha) { 
        m_LineColor = color; 
        m_LineAlpha = alpha; 
        m_HasLineColor = true;
    }

    void SetLineColorBg(COLORREF color, BYTE alpha) {
        m_LineColorBg = color;
        m_LineAlphaBg = alpha;
        m_HasLineColorBg = true;
    }

    void SetLineGradientBg(const GradientInfo& gradient) {
        m_LineGradientBg = gradient;
    }
    
    void SetLineGradient(const GradientInfo& gradient) {
        m_LineGradient = gradient;
    }

    const GradientInfo& GetLineGradient() const { return m_LineGradient; }
    const GradientInfo& GetLineGradientBg() const { return m_LineGradientBg; }

    bool HasLineColor() const { return m_HasLineColor; }
    COLORREF GetLineColor() const { return m_LineColor; }
    BYTE GetLineAlpha() const { return m_LineAlpha; }

    bool HasLineColorBg() const { return m_HasLineColorBg; }
    COLORREF GetLineColorBg() const { return m_LineColorBg; }
    BYTE GetLineAlphaBg() const { return m_LineAlphaBg; }


private:
    float m_Value; // 0.0 to 1.0
    int m_Radius = 0;
    int m_Thickness = 2;
    int m_EndThickness = -1; // -1 means same as m_Thickness
    float m_StartAngle = 0.0f;
    float m_TotalAngle = 360.0f;
    bool m_Clockwise = true;

    RoundLineCap m_StartCap = ROUNDLINE_CAP_FLAT;
    RoundLineCap m_EndCap = ROUNDLINE_CAP_FLAT;
    std::vector<float> m_DashArray;
    int m_Ticks = 0;

    bool m_HasLineColor = false;
    COLORREF m_LineColor = RGB(0, 255, 0);
    BYTE m_LineAlpha = 255;

    bool m_HasLineColorBg = false;
    COLORREF m_LineColorBg = RGB(50, 50, 50);
    BYTE m_LineAlphaBg = 255;

    GradientInfo m_LineGradient;
    GradientInfo m_LineGradientBg;
};

#endif
