#pragma once

#include "ShapeElement.h"

class ElementLayoutBox : public ShapeElement
{
public:
    enum class BorderPosition
    {
        Outside,
        Center,
        Inside
    };

    ElementLayoutBox(const std::wstring &id, int x, int y, int width, int height);
    ~ElementLayoutBox() override = default;

    void Render(ID2D1DeviceContext *context) override;
    bool HitTestLocal(const D2D1_POINT_2F &point) override;
    bool CreateGeometry(ID2D1Factory *factory, Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

    void SetRadii(float rx, float ry) override { m_RadiusX = rx; m_RadiusY = ry; }
    float GetRadiusX() const override { return m_RadiusX; }
    float GetRadiusY() const override { return m_RadiusY; }
    void SetBorderPosition(BorderPosition position) { m_BorderPosition = position; }
    BorderPosition GetBorderPosition() const { return m_BorderPosition; }

private:
    float m_RadiusX = 0.0f;
    float m_RadiusY = 0.0f;
    BorderPosition m_BorderPosition = BorderPosition::Outside;
};
