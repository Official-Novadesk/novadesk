/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ShapeElement.h"
#include "Utils.h"
#include "Logging.h"
#include "Direct2DHelper.h"
#include <d2d1effects.h>
#include <cmath>

ShapeElement::ShapeElement(const std::wstring& id, int x, int y, int width, int height)
    : Element(ELEMENT_SHAPE, id, x, y, width, height)
{
}

ShapeElement::~ShapeElement()
{
    if (m_StrokeStyle) {
        m_StrokeStyle->Release();
        m_StrokeStyle = nullptr;
    }
}

GfxRect ShapeElement::GetBackgroundBounds()
{
    GfxRect bounds = GetBounds();
    if (!m_HasStroke || m_StrokeWidth <= 0.0f) return bounds;

    // Expand background to account for stroke width so it isn't clipped.
    int pad = (int)ceilf(m_StrokeWidth / 2.0f);
    return GfxRect(bounds.X - pad, bounds.Y - pad, bounds.Width + pad * 2, bounds.Height + pad * 2);
}

void ShapeElement::CreateBrush(ID2D1DeviceContext* context, ID2D1Brush** ppBrush, bool isStroke)
{
    bool hasGradient = isStroke ? m_HasStrokeGradient : m_HasFillGradient;

    D2D1_RECT_F rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    const GradientInfo* gradInfo = nullptr;
    if (hasGradient) {
        gradInfo = isStroke ? &m_StrokeGradient : &m_FillGradient;
    }

    COLORREF c = isStroke ? m_StrokeColor : m_FillColor;
    BYTE a = isStroke ? m_StrokeAlpha : m_FillAlpha;

    Direct2D::CreateBrushFromGradientOrColor(context, rect, gradInfo, c, a / 255.0f, ppBrush);
}

bool ShapeElement::TryCreateStrokeBrush(ID2D1DeviceContext* context, Microsoft::WRL::ComPtr<ID2D1Brush>& outBrush)
{
    if (!m_HasStroke || m_StrokeWidth <= 0) return false;
    CreateBrush(context, outBrush.ReleaseAndGetAddressOf(), true);
    return outBrush != nullptr;
}

bool ShapeElement::TryCreateFillBrush(ID2D1DeviceContext* context, Microsoft::WRL::ComPtr<ID2D1Brush>& outBrush)
{
    if (!m_HasFill) return false;
    CreateBrush(context, outBrush.ReleaseAndGetAddressOf(), false);
    return outBrush != nullptr;
}

void ShapeElement::UpdateStrokeStyle(ID2D1DeviceContext* context)
{
    if (m_UpdateStrokeStyle || !m_StrokeStyle) {
        if (m_StrokeStyle) {
            m_StrokeStyle->Release();
            m_StrokeStyle = nullptr;
        }

        ID2D1Factory* factory = nullptr;
        context->GetFactory(&factory);
        ID2D1Factory1* factory1 = nullptr;

        if (factory) {
            factory->QueryInterface(&factory1);
            factory->Release();
        }

        if (factory1) {
            D2D1_STROKE_STYLE_PROPERTIES1 props = D2D1::StrokeStyleProperties1(
                m_StrokeStartCap,
                m_StrokeEndCap,
                m_StrokeDashCap,
                m_StrokeLineJoin,
                10.0f,

                (m_StrokeDashes.empty() ? D2D1_DASH_STYLE_SOLID : D2D1_DASH_STYLE_CUSTOM),
                m_StrokeDashOffset
            );

            factory1->CreateStrokeStyle(
                props,
                m_StrokeDashes.data(),
                (UINT32)m_StrokeDashes.size(),
                &m_StrokeStyle
            );

            factory1->Release();
        }
        m_UpdateStrokeStyle = false;
    }
}

void ShapeElement::EnsureStrokeStyle()
{
    if (!m_UpdateStrokeStyle && m_StrokeStyle) return;

    if (m_StrokeStyle) {
        m_StrokeStyle->Release();
        m_StrokeStyle = nullptr;
    }

    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return;

    D2D1_STROKE_STYLE_PROPERTIES1 props = D2D1::StrokeStyleProperties1(
        m_StrokeStartCap,
        m_StrokeEndCap,
        m_StrokeDashCap,
        m_StrokeLineJoin,
        10.0f,
        (m_StrokeDashes.empty() ? D2D1_DASH_STYLE_SOLID : D2D1_DASH_STYLE_CUSTOM),
        m_StrokeDashOffset
    );

    factory->CreateStrokeStyle(
        props,
        m_StrokeDashes.data(),
        (UINT32)m_StrokeDashes.size(),
        &m_StrokeStyle
    );

    m_UpdateStrokeStyle = false;
}

bool ShapeElement::HitTest(int x, int y)
{
    if (IsConsumed()) return false;

    // Transform the point into local (unrotated) space if needed.
    GfxRect bounds = GetBackgroundBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    D2D1::Matrix3x2F matrix;
    bool needsTransform = (m_HasTransformMatrix || m_Rotate != 0.0f);

    if (needsTransform) {
        if (m_HasTransformMatrix) {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]
            );
        } else {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (!matrix.Invert()) return false;
    }

    D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
    if (needsTransform) {
        p = matrix.TransformPoint(p);
    }

    // Quick reject using bounds before geometry hit-test.
    if (p.x < bounds.X || p.x >= bounds.X + bounds.Width ||
        p.y < bounds.Y || p.y >= bounds.Y + bounds.Height) {
        return false;
    }

    return HitTestLocal(p);
}

D2D1_MATRIX_3X2_F ShapeElement::GetRenderTransformMatrix() const
{
    if (m_HasTransformMatrix) {
        return D2D1::Matrix3x2F(
            m_TransformMatrix[0], m_TransformMatrix[1],
            m_TransformMatrix[2], m_TransformMatrix[3],
            m_TransformMatrix[4], m_TransformMatrix[5]
        );
    }

    if (m_Rotate == 0.0f) {
        return D2D1::Matrix3x2F::Identity();
    }

    GfxRect bounds = const_cast<ShapeElement*>(this)->GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;
    return D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
}
