/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ARC_SHAPE_H__
#define __NOVADESK_ARC_SHAPE_H__

#include "ShapeElement.h"

class ArcShape : public ShapeElement
{
public:
    ArcShape(const std::wstring& id, int x, int y, int width, int height);
    virtual ~ArcShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    virtual bool CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const override;
    
    virtual void SetRadii(float rx, float ry) override { m_RadiusX = rx; m_RadiusY = ry; }
    virtual void SetArcParams(float startAngle, float endAngle, bool clockwise) override {
        m_StartAngle = startAngle;
        m_EndAngle = endAngle;
        m_Clockwise = clockwise;
    }

private:
    float m_RadiusX = 0.0f;
    float m_RadiusY = 0.0f;
    float m_StartAngle = 0.0f;
    float m_EndAngle = 90.0f;
    bool m_Clockwise = true;

    D2D1_POINT_2F CheckPoint(float angle, float rx, float ry, float cx, float cy) const;
    bool CreateArcGeometry(ID2D1Factory* factory, ID2D1PathGeometry** outGeometry, float& outRx, float& outRy, float& outCx, float& outCy) const;
};

#endif
