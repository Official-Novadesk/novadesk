/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PathShape.h"
#include "Utils.h"

#define NANOSVG_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244)
#include "nanosvg/nanosvg.h"
#pragma warning(pop)

#include <vector>
#include <string>

PathShape::PathShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

PathShape::~PathShape()
{
}

void PathShape::CreatePathGeometry(ID2D1Factory* factory, ID2D1PathGeometry** ppGeometry)
{
    if (m_PathData.empty()) return;

    std::string pathDataStr = Utils::ToString(m_PathData);

    std::string svgContent = "<svg><path d=\"" + pathDataStr + "\" /></svg>";

    std::vector<char> inputBuffer(svgContent.begin(), svgContent.end());
    inputBuffer.push_back('\0');

    NSVGimage* image = nsvgParse(inputBuffer.data(), "px", 96.0f);
    if (!image) return;

    factory->CreatePathGeometry(ppGeometry);
    if (!*ppGeometry) {
        nsvgDelete(image);
        return;
    }

    ID2D1GeometrySink* pSink = nullptr;
    (*ppGeometry)->Open(&pSink);
    if (!pSink) {
        (*ppGeometry)->Release();
        *ppGeometry = nullptr;
        nsvgDelete(image);
        return;
    }

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {

            if (path->npts < 2) continue;

            float* p = &path->pts[0];
            D2D1_POINT_2F startPt = D2D1::Point2F(m_X + p[0], m_Y + p[1]);

            pSink->BeginFigure(startPt, m_HasFill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

            for (int i = 0; i < path->npts - 1; i += 3) {
                float* pt = &path->pts[i * 2];

                D2D1_POINT_2F cp1 = D2D1::Point2F(m_X + pt[2], m_Y + pt[3]);
                D2D1_POINT_2F cp2 = D2D1::Point2F(m_X + pt[4], m_Y + pt[5]);
                D2D1_POINT_2F end = D2D1::Point2F(m_X + pt[6], m_Y + pt[7]);

                pSink->AddBezier(D2D1::BezierSegment(cp1, cp2, end));
            }

            pSink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        }
    }

    pSink->Close();
    pSink->Release();
    nsvgDelete(image);
}

void PathShape::Render(ID2D1DeviceContext* context)
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

    ID2D1Factory* pFactory = nullptr;
    context->GetFactory(&pFactory);
    if (!pFactory) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    ID2D1PathGeometry* pGeometry = nullptr;
    CreatePathGeometry(pFactory, &pGeometry);

    if (pGeometry) {
        if (pFillBrush) {
            context->FillGeometry(pGeometry, pFillBrush.Get());
        }
        if (pStrokeBrush) {
            UpdateStrokeStyle(context);
            context->DrawGeometry(pGeometry, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
        }
        pGeometry->Release();
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

