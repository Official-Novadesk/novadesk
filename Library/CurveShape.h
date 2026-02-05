/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_CURVE_SHAPE_H__
#define __NOVADESK_CURVE_SHAPE_H__

#include "ShapeElement.h"

class CurveShape : public ShapeElement
{
public:
    CurveShape(const std::wstring& id, int x, int y, int width, int height);
    virtual ~CurveShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    virtual GfxRect GetBounds() override;
    virtual bool CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const override;

    virtual void SetCurveParams(float startX, float startY, float controlX, float controlY, float control2X, float control2Y, float endX, float endY, const std::wstring& curveType) override {
        m_StartX = startX;
        m_StartY = startY;
        m_ControlX = controlX;
        m_ControlY = controlY;
        m_Control2X = control2X;
        m_Control2Y = control2Y;
        m_EndX = endX;
        m_EndY = endY;
        m_IsCubic = (_wcsicmp(curveType.c_str(), L"cubic") == 0);
    }

    virtual float GetStartX() const override { return m_StartX; }
    virtual float GetStartY() const override { return m_StartY; }
    virtual float GetEndX() const override { return m_EndX; }
    virtual float GetEndY() const override { return m_EndY; }
    virtual float GetControlX() const override { return m_ControlX; }
    virtual float GetControlY() const override { return m_ControlY; }
    virtual float GetControl2X() const override { return m_Control2X; }
    virtual float GetControl2Y() const override { return m_Control2Y; }
    virtual std::wstring GetCurveType() const override { return m_IsCubic ? L"cubic" : L"quadratic"; }

private:
    float m_StartX = 0.0f;
    float m_StartY = 0.0f;
    float m_ControlX = 0.0f;
    float m_ControlY = 0.0f;
    float m_Control2X = 0.0f;
    float m_Control2Y = 0.0f;
    float m_EndX = 0.0f;
    float m_EndY = 0.0f;
    bool m_IsCubic = false;

    bool CreateCurveGeometry(ID2D1Factory* factory, ID2D1PathGeometry** outGeometry) const;
};

#endif
