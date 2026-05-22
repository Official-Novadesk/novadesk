#include "ElementLayoutBox.h"

#include "Direct2DHelper.h"

ElementLayoutBox::ElementLayoutBox(const std::wstring &id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height, ELEMENT_LAYOUT_BOX)
{
}

bool ElementLayoutBox::HitTestLocal(const D2D1_POINT_2F &point)
{
    ID2D1Factory1 *factory = Direct2D::GetFactory();
    if (!factory)
        return false;

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry))
        return false;

    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0)
    {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit)
            return true;
    }

    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0)
    {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit)
            return true;
    }

    return false;
}

bool ElementLayoutBox::CreateGeometry(ID2D1Factory *factory, Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const
{
    if (!factory)
        return false;
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded;
    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;

    if (FAILED(factory->CreateRoundedRectangleGeometry(rect, rounded.GetAddressOf())))
        return false;
    geometry = rounded;
    return true;
}

void ElementLayoutBox::Render(ID2D1DeviceContext *context)
{
    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> strokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
    TryCreateStrokeBrush(context, strokeBrush);
    TryCreateFillBrush(context, fillBrush);

    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;

    if (fillBrush)
    {
        context->FillRoundedRectangle(rect, fillBrush.Get());
    }
    if (strokeBrush)
    {
        UpdateStrokeStyle(context);
        D2D1_ROUNDED_RECT borderRect = rect;
        float radiusX = m_RadiusX;
        float radiusY = m_RadiusY;

        if (m_BorderPosition == BorderPosition::Outside)
        {
            const float half = m_StrokeWidth * 0.5f;
            borderRect.rect.left -= half;
            borderRect.rect.top -= half;
            borderRect.rect.right += half;
            borderRect.rect.bottom += half;
            radiusX += half;
            radiusY += half;
        }
        else if (m_BorderPosition == BorderPosition::Inside)
        {
            const float half = m_StrokeWidth * 0.5f;
            borderRect.rect.left += half;
            borderRect.rect.top += half;
            borderRect.rect.right -= half;
            borderRect.rect.bottom -= half;
            radiusX -= half;
            radiusY -= half;
            if (radiusX < 0.0f) radiusX = 0.0f;
            if (radiusY < 0.0f) radiusY = 0.0f;
        }

        borderRect.radiusX = radiusX;
        borderRect.radiusY = radiusY;
        context->DrawRoundedRectangle(borderRect, strokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
