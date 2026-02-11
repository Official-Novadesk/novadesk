/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LINE_SHAPE_H__
#define __NOVADESK_LINE_SHAPE_H__

#include "ShapeElement.h"

class LineShape : public ShapeElement
{
public:
    LineShape(const std::wstring& id, int x, int y, int width, int height); 
    virtual ~LineShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual GfxRect GetBounds() override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    virtual bool CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const override;
    virtual void SetLinePoints(float x1, float y1, float x2, float y2) override { 
        m_StartX = x1; m_StartY = y1;
        m_EndX = x2; m_EndY = y2;
    }

private:
    float m_StartX = 0.0f;
    float m_StartY = 0.0f;
    float m_EndX = 0.0f;
    float m_EndY = 0.0f;
};

#endif
