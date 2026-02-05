/* Copyright (C) 2026 OfficialNovadesk 
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

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual bool HitTest(int x, int y) override;

    float GetValue() const { return m_Value; }
    BarOrientation GetOrientation() const { return m_Orientation; }

    void SetValue(float value) { m_Value = value; }
    void SetOrientation(BarOrientation orientation) { m_Orientation = orientation; }
    
    void SetBarCornerRadius(int radius) { m_BarCornerRadius = radius; }
    int GetBarCornerRadius() const { return m_BarCornerRadius; }

    const GradientInfo& GetBarGradient() const { return m_BarGradient; }

    void SetBarColor(COLORREF color, BYTE alpha) { 
        m_BarColor = color; 
        m_BarAlpha = alpha; 
        m_HasBarColor = true;
    }
    
    void SetBarGradient(const GradientInfo& gradient) {
        m_BarGradient = gradient;
    }

    bool HasBarColor() const { return m_HasBarColor; }
    COLORREF GetBarColor() const { return m_BarColor; }
    BYTE GetBarAlpha() const { return m_BarAlpha; }

private:
    float m_Value; // 0.0 to 1.0
    BarOrientation m_Orientation;

    int m_BarCornerRadius = 0;

    bool m_HasBarColor = false;
    COLORREF m_BarColor = RGB(0, 255, 0);
    BYTE m_BarAlpha = 255;

    GradientInfo m_BarGradient;
};

#endif
