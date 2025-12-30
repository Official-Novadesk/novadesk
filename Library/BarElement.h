/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BAR_ELEMENT_H__
#define __NOVADESK_BAR_ELEMENT_H__

#include "Element.h"

enum BarOrientation {
    BAR_HORIZONTAL = 0,
    BAR_VERTICAL = 1
};

class BarElement : public Element {
public:
    BarElement(const std::wstring& id, int x, int y, int w, int h, float value, BarOrientation orientation);
    virtual ~BarElement() {}

    virtual void Render(Gdiplus::Graphics& graphics) override;

    float GetValue() const { return m_Value; }
    BarOrientation GetOrientation() const { return m_Orientation; }

    void SetValue(float value) { m_Value = value; }
    void SetOrientation(BarOrientation orientation) { m_Orientation = orientation; }
    
    void SetBarCornerRadius(int radius) { m_BarCornerRadius = radius; }
    int GetBarCornerRadius() const { return m_BarCornerRadius; }

    float GetBarGradientAngle() const { return m_BarGradientAngle; }

    void SetBarColor(COLORREF color, BYTE alpha) { 
        m_BarColor = color; 
        m_BarAlpha = alpha; 
        m_HasBarColor = true;
    }
    
    void SetBarColor2(COLORREF color, BYTE alpha, float angle) {
        m_BarColor2 = color;
        m_BarAlpha2 = alpha;
        m_BarGradientAngle = angle;
        m_HasBarGradient = true;
    }

private:
    float m_Value; // 0.0 to 1.0
    BarOrientation m_Orientation;

    int m_BarCornerRadius = 0;

    bool m_HasBarColor = false;
    COLORREF m_BarColor = RGB(0, 255, 0);
    BYTE m_BarAlpha = 255;

    bool m_HasBarGradient = false;
    COLORREF m_BarColor2 = 0;
    BYTE m_BarAlpha2 = 255;
    float m_BarGradientAngle = 0.0f;
};

#endif
