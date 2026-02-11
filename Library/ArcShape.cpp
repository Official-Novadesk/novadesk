/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ArcShape.h"
#include "Direct2DHelper.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ArcShape::ArcShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

ArcShape::~ArcShape()
{
}

int ArcShape::GetAutoWidth()
{
    if (m_RadiusX > 0.0f) return (int)ceilf(m_RadiusX * 2.0f);
    if (m_RadiusY > 0.0f) return (int)ceilf(m_RadiusY * 2.0f);
    return 0;
}

int ArcShape::GetAutoHeight()
{
    if (m_RadiusY > 0.0f) return (int)ceilf(m_RadiusY * 2.0f);
    if (m_RadiusX > 0.0f) return (int)ceilf(m_RadiusX * 2.0f);
    return 0;
}

D2D1_POINT_2F ArcShape::CheckPoint(float angle, float rx, float ry, float cx, float cy) const
{
    float rad = angle * (float)(M_PI / 180.0f);

    return D2D1::Point2F(cx + rx * cos(rad), cy + ry * sin(rad));
}

bool ArcShape::CreateArcGeometry(ID2D1Factory* factory, ID2D1PathGeometry** outGeometry, float& outRx, float& outRy, float& outCx, float& outCy) const
{
    if (!factory) return false;
    if (FAILED(factory->CreatePathGeometry(outGeometry)) || !*outGeometry) return false;

    outRx = (m_RadiusX > 0) ? m_RadiusX : (m_Width / 2.0f);
    outRy = (m_RadiusY > 0) ? m_RadiusY : (m_Height / 2.0f);
    outCx = m_X + m_Width / 2.0f;
    outCy = m_Y + m_Height / 2.0f;

    ID2D1GeometrySink* pSink = nullptr;
    (*outGeometry)->Open(&pSink);
    if (!pSink) {
        (*outGeometry)->Release();
        *outGeometry = nullptr;
        return false;
    }

    D2D1_POINT_2F startPoint = CheckPoint(m_StartAngle, outRx, outRy, outCx, outCy);
    D2D1_POINT_2F endPoint = CheckPoint(m_EndAngle, outRx, outRy, outCx, outCy);

    pSink->BeginFigure(startPoint, m_HasFill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

    D2D1_SWEEP_DIRECTION sweep = m_Clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

    float sweepAngle = m_EndAngle - m_StartAngle;
    if (m_Clockwise && sweepAngle < 0) sweepAngle += 360.0f;
    if (!m_Clockwise && sweepAngle > 0) sweepAngle -= 360.0f;

    bool largeArc = m_Clockwise ? (sweepAngle > 180.0f) : (sweepAngle < -180.0f);

    pSink->AddArc(D2D1::ArcSegment(
        endPoint,
        D2D1::SizeF(outRx, outRy),
        0.0f,
        sweep,
        largeArc ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
    ));

    pSink->EndFigure(m_HasFill ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
    pSink->Close();
    pSink->Release();

    return true;
}

bool ArcShape::HitTestLocal(const D2D1_POINT_2F& point)
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

bool ArcShape::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    ID2D1PathGeometry* path = nullptr;
    float rx = 0.0f, ry = 0.0f, cx = 0.0f, cy = 0.0f;
    if (!CreateArcGeometry(factory, &path, rx, ry, cx, cy)) return false;
    geometry.Attach(path);
    return true;
}

void ArcShape::Render(ID2D1DeviceContext* context)
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

    ID2D1Factory* pFactory = nullptr;
    context->GetFactory(&pFactory);

    if (!pFactory) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (CreateGeometry(pFactory, geometry)) {
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
