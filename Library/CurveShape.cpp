/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "CurveShape.h"
#include "Direct2DHelper.h"
#include <cmath>

CurveShape::CurveShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

CurveShape::~CurveShape()
{
}

bool CurveShape::CreateCurveGeometry(ID2D1Factory* factory, ID2D1PathGeometry** outGeometry) const
{
    if (!factory) return false;
    if (FAILED(factory->CreatePathGeometry(outGeometry)) || !*outGeometry) return false;

    ID2D1GeometrySink* sink = nullptr;
    (*outGeometry)->Open(&sink);
    if (!sink) {
        (*outGeometry)->Release();
        *outGeometry = nullptr;
        return false;
    }

    sink->BeginFigure(D2D1::Point2F(m_StartX, m_StartY),
        m_HasFill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

    if (m_IsCubic) {
        sink->AddBezier(D2D1::BezierSegment(
            D2D1::Point2F(m_ControlX, m_ControlY),
            D2D1::Point2F(m_Control2X, m_Control2Y),
            D2D1::Point2F(m_EndX, m_EndY)
        ));
    } else {
        sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(
            D2D1::Point2F(m_ControlX, m_ControlY),
            D2D1::Point2F(m_EndX, m_EndY)
        ));
    }

    sink->EndFigure(m_HasFill ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
    sink->Close();
    sink->Release();

    return true;
}

GfxRect CurveShape::GetBounds()
{
    float minX = m_StartX;
    float maxX = m_StartX;
    float minY = m_StartY;
    float maxY = m_StartY;

    auto consider = [&](float x, float y) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    };

    consider(m_ControlX, m_ControlY);
    consider(m_EndX, m_EndY);
    if (m_IsCubic) {
        consider(m_Control2X, m_Control2Y);
    }

    float pad = (m_StrokeWidth > 0.0f) ? (m_StrokeWidth / 2.0f) : 0.0f;
    int x = (int)floorf(minX - pad);
    int y = (int)floorf(minY - pad);
    int w = (int)ceilf((maxX - minX) + pad * 2.0f);
    int h = (int)ceilf((maxY - minY) + pad * 2.0f);

    return GfxRect(x, y, w, h);
}

bool CurveShape::HitTestLocal(const D2D1_POINT_2F& point)
{
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return false;

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry)) return false;

    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0) {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit) {
            geometry->Release();
            return true;
        }
    }

    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0) {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit) {
            geometry->Release();
            return true;
        }
    }

    return false;
}

bool CurveShape::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    ID2D1PathGeometry* path = nullptr;
    if (!CreateCurveGeometry(factory, &path)) return false;
    geometry.Attach(path);
    return true;
}

void CurveShape::Render(ID2D1DeviceContext* context)
{
    if (IsConsumed()) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> pStrokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> pFillBrush;
    TryCreateStrokeBrush(context, pStrokeBrush);
    TryCreateFillBrush(context, pFillBrush);

    if (!pStrokeBrush && !pFillBrush) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    ID2D1Factory* factory = nullptr;
    context->GetFactory(&factory);
    if (!factory) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (CreateGeometry(factory, geometry)) {
        if (pFillBrush) {
            context->FillGeometry(geometry.Get(), pFillBrush.Get());
        }
        if (pStrokeBrush) {
            UpdateStrokeStyle(context);
            context->DrawGeometry(geometry.Get(), pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
