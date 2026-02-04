/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ArcShape.h"
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

D2D1_POINT_2F ArcShape::CheckPoint(float angle, float rx, float ry, float cx, float cy)
{
    float rad = angle * (float)(M_PI / 180.0f);

    return D2D1::Point2F(cx + rx * cos(rad), cy + ry * sin(rad));
}

void ArcShape::Render(ID2D1DeviceContext* context)
{
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

    float rx = (m_RadiusX > 0) ? m_RadiusX : (m_Width / 2.0f);
    float ry = (m_RadiusY > 0) ? m_RadiusY : (m_Height / 2.0f);

    float cx = m_X + m_Width / 2.0f;
    float cy = m_Y + m_Height / 2.0f;

    ID2D1Factory* pFactory = nullptr;
    context->GetFactory(&pFactory);

    if (!pFactory) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    ID2D1PathGeometry* pPathGeometry = nullptr;
    pFactory->CreatePathGeometry(&pPathGeometry);

    if (pPathGeometry) {
        ID2D1GeometrySink* pSink = nullptr;
        pPathGeometry->Open(&pSink);

        if (pSink) {
            D2D1_POINT_2F startPoint = CheckPoint(m_StartAngle, rx, ry, cx, cy);
            D2D1_POINT_2F endPoint = CheckPoint(m_EndAngle, rx, ry, cx, cy);

            pSink->BeginFigure(startPoint, m_HasFill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

            D2D1_SWEEP_DIRECTION sweep = m_Clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

            float diff = abs(m_EndAngle - m_StartAngle);
            if (!m_Clockwise) {

            }

            bool largeArc = (diff > 180.0f);

            float sweepAngle = m_EndAngle - m_StartAngle;
            if (m_Clockwise && sweepAngle < 0) sweepAngle += 360.0f;
            if (!m_Clockwise && sweepAngle > 0) sweepAngle -= 360.0f;

            if (m_Clockwise) {
                largeArc = (sweepAngle > 180.0f);
            }
            else {
                largeArc = (sweepAngle < -180.0f);
            }

            pSink->AddArc(D2D1::ArcSegment(
                endPoint,
                D2D1::SizeF(rx, ry),
                0.0f,
                sweep,
                largeArc ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
            ));

            pSink->EndFigure(m_HasFill ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
            pSink->Close();
            pSink->Release();
        }

        if (pFillBrush) {
            context->FillGeometry(pPathGeometry, pFillBrush.Get());
        }
        if (pStrokeBrush) {
            UpdateStrokeStyle(context);
            context->DrawGeometry(pPathGeometry, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
        }

        pPathGeometry->Release();
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
