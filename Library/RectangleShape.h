/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_RECTANGLE_SHAPE_H__
#define __NOVADESK_RECTANGLE_SHAPE_H__

#include "ShapeElement.h"

class RectangleShape : public ShapeElement
{
public:
    RectangleShape(const std::wstring& id, int x, int y, int width, int height);
    virtual ~RectangleShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    virtual bool CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const override;
    virtual void SetRadii(float rx, float ry) override { m_RadiusX = rx; m_RadiusY = ry; }
    virtual float GetRadiusX() const override { return m_RadiusX; }
    virtual float GetRadiusY() const override { return m_RadiusY; }

private:
    float m_RadiusX = 0.0f;
    float m_RadiusY = 0.0f;
};

#endif
