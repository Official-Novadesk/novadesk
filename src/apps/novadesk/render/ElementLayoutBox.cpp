/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "ElementLayoutBox.h"

#include "Direct2DHelper.h"
#include <algorithm>
#include <cmath>
#include <d2d1effects.h>

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

    Microsoft::WRL::ComPtr<ID2D1Brush> strokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
    TryCreateStrokeBrush(context, strokeBrush);
    TryCreateFillBrush(context, fillBrush);

    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;

    for (const auto &shadow : m_BoxShadows)
    {
        if (!shadow.inset)
            RenderSingleShadow(context, rect, shadow);
    }

    if (fillBrush)
    {
        context->FillRoundedRectangle(rect, fillBrush.Get());
    }

    for (const auto &shadow : m_BoxShadows)
    {
        if (shadow.inset)
            RenderSingleShadow(context, rect, shadow);
    }

    if (strokeBrush)
    {
        D2D1_ROUNDED_RECT borderRect = rect;
        float radiusX = m_RadiusX;
        float radiusY = m_RadiusY;

        if (m_BorderPosition == BorderPosition::Outside)
        {
            const float half = m_StrokeWidth * 0.5f;
            borderRect.rect.left   -= half;
            borderRect.rect.top    -= half;
            borderRect.rect.right  += half;
            borderRect.rect.bottom += half;
            // Only expand the corner radius when the element actually has one.
            // Adding half when m_RadiusX == 0 would make DrawRoundedRectangle
            // curve the outer corners of an otherwise square-cornered box.
            if (m_RadiusX > 0.0f) radiusX += half;
            if (m_RadiusY > 0.0f) radiusY += half;
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
        RenderBorderWithStyle(context, borderRect, strokeBrush.Get(), m_StrokeWidth);
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

void ElementLayoutBox::RenderSingleShadow(ID2D1DeviceContext *context, const D2D1_ROUNDED_RECT &baseRect, const BoxShadow &shadow)
{
    if (!context || shadow.alpha == 0)
        return;

    const float width = baseRect.rect.right - baseRect.rect.left;
    const float height = baseRect.rect.bottom - baseRect.rect.top;

    const float pad = std::max(2.0f, std::ceil(std::max(0.0f, shadow.blur) + std::fabs(shadow.spread) + 4.0f));

    D2D1_ROUNDED_RECT shadowRect = D2D1::RoundedRect(
        D2D1::RectF(pad, pad, pad + width, pad + height),
        baseRect.radiusX,
        baseRect.radiusY);
    if (shadow.spread != 0.0f)
    {
        shadowRect.rect.left -= shadow.spread;
        shadowRect.rect.top -= shadow.spread;
        shadowRect.rect.right += shadow.spread;
        shadowRect.rect.bottom += shadow.spread;
        shadowRect.radiusX = std::max(0.0f, baseRect.radiusX + shadow.spread);
        shadowRect.radiusY = std::max(0.0f, baseRect.radiusY + shadow.spread);

        const float minExtent = 1.0f;
        if ((shadowRect.rect.right - shadowRect.rect.left) < minExtent)
        {
            const float cx = (shadowRect.rect.left + shadowRect.rect.right) * 0.5f;
            shadowRect.rect.left = cx - (minExtent * 0.5f);
            shadowRect.rect.right = cx + (minExtent * 0.5f);
        }
        if ((shadowRect.rect.bottom - shadowRect.rect.top) < minExtent)
        {
            const float cy = (shadowRect.rect.top + shadowRect.rect.bottom) * 0.5f;
            shadowRect.rect.top = cy - (minExtent * 0.5f);
            shadowRect.rect.bottom = cy + (minExtent * 0.5f);
        }
    }

    Microsoft::WRL::ComPtr<ID2D1CommandList> commandList;
    if (FAILED(context->CreateCommandList(&commandList)))
        return;

    Microsoft::WRL::ComPtr<ID2D1Device> device;
    context->GetDevice(&device);
    if (!device)
        return;

    Microsoft::WRL::ComPtr<ID2D1DeviceContext> recordingContext;
    if (FAILED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &recordingContext)))
        return;

    recordingContext->SetTarget(commandList.Get());
    recordingContext->BeginDraw();
    recordingContext->Clear(D2D1::ColorF(0, 0.0f));
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    if (Direct2D::CreateSolidBrush(recordingContext.Get(), RGB(255, 255, 255), 1.0f, &whiteBrush))
    {
        if (shadow.inset)
        {
            float insetThickness = std::max(
                1.0f,
                (shadow.blur * 1.15f) +
                (std::max(0.0f, shadow.spread) * 1.6f) +
                (std::max(std::fabs(shadow.x), std::fabs(shadow.y)) * 0.45f));
            if (shadow.spread < 0.0f)
            {
                insetThickness = std::max(1.0f, insetThickness + shadow.spread);
            }
            recordingContext->DrawRoundedRectangle(shadowRect, whiteBrush.Get(), insetThickness);
        }
        else
        {
            recordingContext->FillRoundedRectangle(shadowRect, whiteBrush.Get());
        }
    }
    recordingContext->EndDraw();
    commandList->Close();

    Microsoft::WRL::ComPtr<ID2D1Effect> shadowEffect;
    Microsoft::WRL::ComPtr<ID2D1Effect> colorMatrixEffect;
    if (FAILED(context->CreateEffect(CLSID_D2D1Shadow, &shadowEffect)) ||
        FAILED(context->CreateEffect(CLSID_D2D1ColorMatrix, &colorMatrixEffect)))
    {
        return;
    }

    shadowEffect->SetInput(0, commandList.Get());
    shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, std::max(0.0f, shadow.blur));

    colorMatrixEffect->SetInputEffect(0, shadowEffect.Get());
    D2D1_COLOR_F c = Direct2D::ColorToD2D(shadow.color, shadow.alpha / 255.0f);
    D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        c.r * c.a, c.g * c.a, c.b * c.a, c.a,
        0, 0, 0, 0);
    colorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);

    const D2D1_POINT_2F shadowOffset = D2D1::Point2F(
        baseRect.rect.left + shadow.x - pad,
        baseRect.rect.top + shadow.y - pad);
    if (shadow.inset)
    {
        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> clipGeometry;
        ID2D1Factory1 *factory = Direct2D::GetFactory();
        if (!factory || FAILED(factory->CreateRoundedRectangleGeometry(baseRect, &clipGeometry)))
            return;

        D2D1_LAYER_PARAMETERS1 layerParams = D2D1::LayerParameters1(
            baseRect.rect,
            clipGeometry.Get(),
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS1_NONE);
        context->PushLayer(layerParams, nullptr);
        context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
        context->PopLayer();
        return;
    }

    context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
}

void ElementLayoutBox::RenderBorderWithStyle(ID2D1DeviceContext *context, const D2D1_ROUNDED_RECT &rect,
                                             ID2D1Brush *strokeBrush, float strokeWidth)
{
    if (m_BorderStyleTop == BorderStyle::None && m_BorderStyleRight == BorderStyle::None &&
        m_BorderStyleBottom == BorderStyle::None && m_BorderStyleLeft == BorderStyle::None)
        return;
    if (m_BorderStyleTop == BorderStyle::Hidden && m_BorderStyleRight == BorderStyle::Hidden &&
        m_BorderStyleBottom == BorderStyle::Hidden && m_BorderStyleLeft == BorderStyle::Hidden)
        return;

    bool sameStyle = (m_BorderStyleTop == m_BorderStyleRight &&
                      m_BorderStyleRight == m_BorderStyleBottom &&
                      m_BorderStyleBottom == m_BorderStyleLeft);

    auto getStrokeStyle = [&](BorderStyle style, ID2D1StrokeStyle1** outStyle) {
        *outStyle = nullptr;
        ID2D1Factory1* factory = Direct2D::GetFactory();
        if (!factory) return;

        D2D1_STROKE_STYLE_PROPERTIES1 props = {};
        props.startCap = m_StrokeStartCap;
        props.endCap = m_StrokeEndCap;
        props.dashCap = m_StrokeDashCap;
        props.lineJoin = m_StrokeLineJoin;
        props.miterLimit = 10.0f;
        props.dashOffset = m_StrokeDashOffset;
        props.transformType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;

        if (style == BorderStyle::Dotted) {
            props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
            props.startCap = D2D1_CAP_STYLE_ROUND;
            props.endCap = D2D1_CAP_STYLE_ROUND;
            props.dashCap = D2D1_CAP_STYLE_ROUND;
            float dashes[] = { 0.0f, 2.0f };
            factory->CreateStrokeStyle(props, dashes, 2, outStyle);
        } else if (style == BorderStyle::Dashed) {
            props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
            float dashes[] = { 3.0f, 1.0f };
            factory->CreateStrokeStyle(props, dashes, 2, outStyle);
        } else {
            props.dashStyle = D2D1_DASH_STYLE_SOLID;
            factory->CreateStrokeStyle(props, nullptr, 0, outStyle);
        }
    };

    auto drawStyleRect = [&](const D2D1_ROUNDED_RECT& r, BorderStyle style, ID2D1Brush* brush, float width) {
        if (style == BorderStyle::None || style == BorderStyle::Hidden) return;
        
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> sstyle;
        getStrokeStyle(style, &sstyle);

        if (style == BorderStyle::Double) {
            float third = width / 3.0f;
            D2D1_ROUNDED_RECT r1 = r;
            D2D1_ROUNDED_RECT r2 = r;
            r1.rect.left -= third; r1.rect.top -= third; r1.rect.right += third; r1.rect.bottom += third;
            r2.rect.left += third; r2.rect.top += third; r2.rect.right -= third; r2.rect.bottom -= third;
            if (r.radiusX == 0.0f && r.radiusY == 0.0f) {
                context->DrawRectangle(r1.rect, brush, third, sstyle.Get());
                context->DrawRectangle(r2.rect, brush, third, sstyle.Get());
            } else {
                r1.radiusX = r.radiusX + third;
                r1.radiusY = r.radiusY + third;
                r2.radiusX = std::max(0.0f, r.radiusX - third);
                r2.radiusY = std::max(0.0f, r.radiusY - third);
                context->DrawRoundedRectangle(r1, brush, third, sstyle.Get());
                context->DrawRoundedRectangle(r2, brush, third, sstyle.Get());
            }
        } else if (style == BorderStyle::Groove || style == BorderStyle::Ridge ||
                   style == BorderStyle::Inset  || style == BorderStyle::Outset) {

            COLORREF baseColor = m_StrokeColor;
            BYTE rC = GetRValue(baseColor), gC = GetGValue(baseColor), bC = GetBValue(baseColor);
            COLORREF lightC = RGB(std::min(255, rC + 50), std::min(255, gC + 50), std::min(255, bC + 50));
            COLORREF darkC  = RGB(std::max(0,   rC - 50), std::max(0,   gC - 50), std::max(0,   bC - 50));

            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> lightBrush, darkBrush;
            context->CreateSolidColorBrush(Direct2D::ColorToD2D(lightC, m_StrokeAlpha / 255.0f), &lightBrush);
            context->CreateSolidColorBrush(Direct2D::ColorToD2D(darkC,  m_StrokeAlpha / 255.0f), &darkBrush);

            ID2D1Brush* topLeftBrush    = nullptr;
            ID2D1Brush* bottomRightBrush = nullptr;
            if (style == BorderStyle::Groove) { topLeftBrush = darkBrush.Get();  bottomRightBrush = lightBrush.Get(); }
            else if (style == BorderStyle::Ridge)  { topLeftBrush = lightBrush.Get(); bottomRightBrush = darkBrush.Get();  }
            else if (style == BorderStyle::Inset)  { topLeftBrush = darkBrush.Get();  bottomRightBrush = lightBrush.Get(); }
            else if (style == BorderStyle::Outset) { topLeftBrush = lightBrush.Get(); bottomRightBrush = darkBrush.Get();  }

            if (m_RadiusX == 0.0f && m_RadiusY == 0.0f) {
                D2D1_ANTIALIAS_MODE oldMode = context->GetAntialiasMode();
                context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

                // Sharp-corner path: draw perfectly mitered trapezoids for the border sides.
                float L = r.rect.left,  T = r.rect.top;
                float R = r.rect.right, B = r.rect.bottom;
                float w = width;

                auto drawPolygon = [&](const D2D1_POINT_2F* points, int count, ID2D1Brush* pBrush) {
                    if (!pBrush) return;
                    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
                    context->GetFactory(&factory);
                    if (!factory) return;
                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeom;
                    if (SUCCEEDED(factory->CreatePathGeometry(&pathGeom))) {
                        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                        if (SUCCEEDED(pathGeom->Open(&sink))) {
                            sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
                            sink->AddLines(points + 1, count - 1);
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            sink->Close();
                            context->FillGeometry(pathGeom.Get(), pBrush);
                        }
                    }
                };

                if (style == BorderStyle::Inset || style == BorderStyle::Outset) {
                    ID2D1Brush* tbBrush = topLeftBrush;
                    ID2D1Brush* lrBrush = topLeftBrush;
                    ID2D1Brush* bbBrush = bottomRightBrush;
                    ID2D1Brush* rrBrush = bottomRightBrush;

                    // Top trapezoid
                    D2D1_POINT_2F topPts[] = {
                        D2D1::Point2F(L, T),
                        D2D1::Point2F(R, T),
                        D2D1::Point2F(R - w, T + w),
                        D2D1::Point2F(L + w, T + w)
                    };
                    drawPolygon(topPts, 4, tbBrush);

                    // Left trapezoid
                    D2D1_POINT_2F leftPts[] = {
                        D2D1::Point2F(L, T),
                        D2D1::Point2F(L + w, T + w),
                        D2D1::Point2F(L + w, B - w),
                        D2D1::Point2F(L, B)
                    };
                    drawPolygon(leftPts, 4, lrBrush);

                    // Bottom trapezoid
                    D2D1_POINT_2F bottomPts[] = {
                        D2D1::Point2F(L + w, B - w),
                        D2D1::Point2F(R - w, B - w),
                        D2D1::Point2F(R, B),
                        D2D1::Point2F(L, B)
                    };
                    drawPolygon(bottomPts, 4, bbBrush);

                    // Right trapezoid
                    D2D1_POINT_2F rightPts[] = {
                        D2D1::Point2F(R - w, T + w),
                        D2D1::Point2F(R, T),
                        D2D1::Point2F(R, B),
                        D2D1::Point2F(R - w, B - w)
                    };
                    drawPolygon(rightPts, 4, rrBrush);
                }
                else if (style == BorderStyle::Groove || style == BorderStyle::Ridge) {
                    float h = w * 0.5f;

                    ID2D1Brush* outerTopLeftBrush  = (style == BorderStyle::Groove) ? darkBrush.Get()  : lightBrush.Get();
                    ID2D1Brush* innerTopLeftBrush  = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                    ID2D1Brush* outerBotRightBrush = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                    ID2D1Brush* innerBotRightBrush = (style == BorderStyle::Groove) ? darkBrush.Get()  : lightBrush.Get();

                    // --- TOP SIDE ---
                    // Outer top
                    D2D1_POINT_2F topOuterPts[] = {
                        D2D1::Point2F(L, T),
                        D2D1::Point2F(R, T),
                        D2D1::Point2F(R - h, T + h),
                        D2D1::Point2F(L + h, T + h)
                    };
                    drawPolygon(topOuterPts, 4, outerTopLeftBrush);

                    // Inner top
                    D2D1_POINT_2F topInnerPts[] = {
                        D2D1::Point2F(L + h, T + h),
                        D2D1::Point2F(R - h, T + h),
                        D2D1::Point2F(R - w, T + w),
                        D2D1::Point2F(L + w, T + w)
                    };
                    drawPolygon(topInnerPts, 4, innerTopLeftBrush);

                    // --- LEFT SIDE ---
                    // Outer left
                    D2D1_POINT_2F leftOuterPts[] = {
                        D2D1::Point2F(L, T),
                        D2D1::Point2F(L + h, T + h),
                        D2D1::Point2F(L + h, B - h),
                        D2D1::Point2F(L, B)
                    };
                    drawPolygon(leftOuterPts, 4, outerTopLeftBrush);

                    // Inner left
                    D2D1_POINT_2F leftInnerPts[] = {
                        D2D1::Point2F(L + h, T + h),
                        D2D1::Point2F(L + w, T + w),
                        D2D1::Point2F(L + w, B - w),
                        D2D1::Point2F(L + h, B - h)
                    };
                    drawPolygon(leftInnerPts, 4, innerTopLeftBrush);

                    // --- BOTTOM SIDE ---
                    // Outer bottom
                    D2D1_POINT_2F bottomOuterPts[] = {
                        D2D1::Point2F(L + h, B - h),
                        D2D1::Point2F(R - h, B - h),
                        D2D1::Point2F(R, B),
                        D2D1::Point2F(L, B)
                    };
                    drawPolygon(bottomOuterPts, 4, outerBotRightBrush);

                    // Inner bottom
                    D2D1_POINT_2F bottomInnerPts[] = {
                        D2D1::Point2F(L + w, B - w),
                        D2D1::Point2F(R - w, B - w),
                        D2D1::Point2F(R - h, B - h),
                        D2D1::Point2F(L + h, B - h)
                    };
                    drawPolygon(bottomInnerPts, 4, innerBotRightBrush);

                    // --- RIGHT SIDE ---
                    // Outer right
                    D2D1_POINT_2F rightOuterPts[] = {
                        D2D1::Point2F(R - h, T + h),
                        D2D1::Point2F(R, T),
                        D2D1::Point2F(R, B),
                        D2D1::Point2F(R - h, B - h)
                    };
                    drawPolygon(rightOuterPts, 4, outerBotRightBrush);

                    // Inner right
                    D2D1_POINT_2F rightInnerPts[] = {
                        D2D1::Point2F(R - w, T + w),
                        D2D1::Point2F(R - h, T + h),
                        D2D1::Point2F(R - h, B - h),
                        D2D1::Point2F(R - w, B - w)
                    };
                    drawPolygon(rightInnerPts, 4, innerBotRightBrush);
                }
                context->SetAntialiasMode(oldMode);
            } else {
                // Rounded-corner: diagonal-clip approach for CSS-accurate rendering.
                // A diagonal from (L,T) to (R,B) splits the border into two colour zones:
                //   top-left  half → shade A   (shadow side)
                //   bottom-right half → shade B (highlight side)
                // For inset/outset: single full-width ring, split diagonally.
                // For groove/ridge: two concentric half-width rings, each split diagonally.
                // To avoid clipping the outer rounded border itself, the clip regions extend
                // far beyond the outer boundary of the border, but meet precisely at the
                // extended diagonal joining (R, T) and (L, B).
                float half = width / 2.0f;
                float L = r.rect.left,  T = r.rect.top;
                float R = r.rect.right, B = r.rect.bottom;

                Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
                context->GetFactory(&d2dFactory);

                if (d2dFactory) {
                    float pad = width * 4.0f + 100.0f;
                    float dx = R - L;
                    float dy = T - B;
                    float len = sqrtf(dx * dx + dy * dy);
                    
                    D2D1_POINT_2F pTR_ext = D2D1::Point2F(R, T);
                    D2D1_POINT_2F pBL_ext = D2D1::Point2F(L, B);
                    if (len > 0.001f) {
                        float ux = dx / len;
                        float uy = dy / len;
                        pTR_ext = D2D1::Point2F(R + ux * pad, T + uy * pad);
                        pBL_ext = D2D1::Point2F(L - ux * pad, B - uy * pad);
                    }

                    // Top-left clip quad: pBL_ext -> pTR_ext -> (pTR_ext.x - pad, pTR_ext.y - pad) -> (pBL_ext.x - pad, pBL_ext.y - pad)
                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> tlClip;
                    {
                        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                        if (SUCCEEDED(d2dFactory->CreatePathGeometry(&tlClip)) &&
                            SUCCEEDED(tlClip->Open(&sink))) {
                            sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                            sink->AddLine(pTR_ext);
                            sink->AddLine(D2D1::Point2F(pTR_ext.x - pad, pTR_ext.y - pad));
                            sink->AddLine(D2D1::Point2F(pBL_ext.x - pad, pBL_ext.y - pad));
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            sink->Close();
                        }
                    }
                    // Bottom-right clip quad: pBL_ext -> pTR_ext -> (pTR_ext.x + pad, pTR_ext.y + pad) -> (pBL_ext.x + pad, pBL_ext.y + pad)
                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> brClip;
                    {
                        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                        if (SUCCEEDED(d2dFactory->CreatePathGeometry(&brClip)) &&
                            SUCCEEDED(brClip->Open(&sink))) {
                            sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                            sink->AddLine(pTR_ext);
                            sink->AddLine(D2D1::Point2F(pTR_ext.x + pad, pTR_ext.y + pad));
                            sink->AddLine(D2D1::Point2F(pBL_ext.x + pad, pBL_ext.y + pad));
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            sink->Close();
                        }
                    }

                    // Helper: draw one rounded-rect ring split into two colour halves
                    auto drawSplitRing = [&](const D2D1_ROUNDED_RECT& rr, float sw,
                                             ID2D1Brush* tlBrush, ID2D1Brush* brBrush) {
                        if (tlClip && tlBrush) {
                            auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), tlClip.Get());
                            context->PushLayer(lp, nullptr);
                            context->DrawRoundedRectangle(rr, tlBrush, sw, sstyle.Get());
                            context->PopLayer();
                        }
                        if (brClip && brBrush) {
                            auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), brClip.Get());
                            context->PushLayer(lp, nullptr);
                            context->DrawRoundedRectangle(rr, brBrush, sw, sstyle.Get());
                            context->PopLayer();
                        }
                    };

                    if (style == BorderStyle::Inset || style == BorderStyle::Outset) {
                        // Single full-width ring, colour split by diagonal:
                        //   inset : topLeft=dark,  botRight=light
                        //   outset: topLeft=light, botRight=dark
                        drawSplitRing(r, width, topLeftBrush, bottomRightBrush);
                    } else {
                        // Groove / Ridge: two concentric half-width rings, each split.
                        D2D1_ROUNDED_RECT rOuter = r, rInner = r;
                        rOuter.rect.left -= half/2; rOuter.rect.top -= half/2;
                        rOuter.rect.right += half/2; rOuter.rect.bottom += half/2;
                        rInner.rect.left += half/2; rInner.rect.top += half/2;
                        rInner.rect.right -= half/2; rInner.rect.bottom -= half/2;
                        rOuter.radiusX = r.radiusX + half/2;
                        rOuter.radiusY = r.radiusY + half/2;
                        rInner.radiusX = std::max(0.0f, r.radiusX - half/2);
                        rInner.radiusY = std::max(0.0f, r.radiusY - half/2);

                        // groove: outer=dark/light, inner=light/dark
                        // ridge : outer=light/dark, inner=dark/light
                        ID2D1Brush* outerTl = (style == BorderStyle::Groove) ? darkBrush.Get()  : lightBrush.Get();
                        ID2D1Brush* outerBr = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* innerTl = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* innerBr = (style == BorderStyle::Groove) ? darkBrush.Get()  : lightBrush.Get();

                        drawSplitRing(rOuter, half, outerTl, outerBr);
                        drawSplitRing(rInner, half, innerTl, innerBr);
                    }
                }
            }
        } else if (style == BorderStyle::Dotted && m_RadiusX == 0.0f && m_RadiusY == 0.0f) {
            float left = r.rect.left;
            float top = r.rect.top;
            float right = r.rect.right;
            float bottom = r.rect.bottom;
            float r_dot = width / 2.0f;

            auto drawSideDotted = [&](D2D1_POINT_2F p0, D2D1_POINT_2F p1) {
                float dx = p1.x - p0.x;
                float dy = p1.y - p0.y;
                float len = sqrtf(dx*dx + dy*dy);
                if (len < 0.001f) return;

                float ideal_spacing = width * 2.0f;
                int num_dots = static_cast<int>(len / ideal_spacing + 0.5f) + 1;
                if (num_dots < 2) num_dots = 2;

                for (int i = 0; i < num_dots; ++i) {
                    float t = static_cast<float>(i) / (num_dots - 1);
                    D2D1_POINT_2F pt = D2D1::Point2F(p0.x + dx * t, p0.y + dy * t);
                    context->FillEllipse(D2D1::Ellipse(pt, r_dot, r_dot), brush);
                }
            };

            drawSideDotted(D2D1::Point2F(left, top), D2D1::Point2F(right, top));
            drawSideDotted(D2D1::Point2F(right, top), D2D1::Point2F(right, bottom));
            drawSideDotted(D2D1::Point2F(right, bottom), D2D1::Point2F(left, bottom));
            drawSideDotted(D2D1::Point2F(left, bottom), D2D1::Point2F(left, top));
        } else if (style == BorderStyle::Dashed && m_RadiusX == 0.0f && m_RadiusY == 0.0f) {
            // CSS-style dashed border:
            //   - L-shaped filled-rectangle dashes at each corner
            //   - Evenly-spaced rectangular dashes along each straight side
            float left   = r.rect.left;
            float top    = r.rect.top;
            float right  = r.rect.right;
            float bottom = r.rect.bottom;

            // Dash length ≈ 3× stroke width, gap ≈ 1× stroke width (CSS ratio 3:1)
            float dashLen = width * 3.0f;
            float gap     = width * 1.0f;
            float period  = dashLen + gap;

            // ----------------------------------------------------------------
            // Corner L-shapes
            // Each corner: one horizontal arm + one vertical arm, each dashLen long,
            // both strokeWidth thick, forming an L inside the border band.
            // ----------------------------------------------------------------
            // Top-left corner
            context->FillRectangle(D2D1::RectF(left, top, left + dashLen, top + width), brush);
            context->FillRectangle(D2D1::RectF(left, top, left + width, top + dashLen), brush);

            // Top-right corner
            context->FillRectangle(D2D1::RectF(right - dashLen, top, right, top + width), brush);
            context->FillRectangle(D2D1::RectF(right - width, top, right, top + dashLen), brush);

            // Bottom-right corner
            context->FillRectangle(D2D1::RectF(right - dashLen, bottom - width, right, bottom), brush);
            context->FillRectangle(D2D1::RectF(right - width, bottom - dashLen, right, bottom), brush);

            // Bottom-left corner
            context->FillRectangle(D2D1::RectF(left, bottom - width, left + dashLen, bottom), brush);
            context->FillRectangle(D2D1::RectF(left, bottom - dashLen, left + width, bottom), brush);

            // ----------------------------------------------------------------
            // Straight dashes between corners (excluding the dashLen already used)
            // CSS pattern: [L-corner] GAP [dash GAP]×n [L-corner]
            // → n dashes, (n+1) equal gaps; dashes never touch the corner arms.
            // ----------------------------------------------------------------

            // isHoriz == true  → horizontal band; edgeCoord is the band's top-left y
            // isHoriz == false → vertical band;   edgeCoord is the band's top-left x
            auto fillDashes = [&](float runStart, float runEnd, bool isHoriz, float edgeCoord) {
                float runLen = runEnd - runStart;
                if (runLen <= 0.0f) return;

                // CSS layout: n dashes with (n+1) gaps
                // Minimum to fit 1 dash: dashLen + 2*gap
                // General: n*(dashLen+gap) + gap <= runLen
                //          n <= (runLen - gap) / period
                int n = (runLen >= gap + dashLen)
                        ? static_cast<int>((runLen - gap) / period)
                        : 0;
                if (n < 1) return;

                // Distribute the leftover space equally into the (n+1) gaps
                // while keeping each dash exactly dashLen long.
                float gapActual = (runLen - static_cast<float>(n) * dashLen)
                                   / static_cast<float>(n + 1);

                for (int i = 0; i < n; ++i) {
                    float s = runStart + gapActual
                              + static_cast<float>(i) * (dashLen + gapActual);
                    float e = s + dashLen;
                    if (isHoriz)
                        context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + width), brush);
                    else
                        context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + width, e), brush);
                }
            };

            // Top side:    left+dashLen  →  right-dashLen,  band top=top
            fillDashes(left + dashLen, right - dashLen, true, top);
            // Bottom side: left+dashLen  →  right-dashLen,  band top=bottom-width
            fillDashes(left + dashLen, right - dashLen, true, bottom - width);
            // Left side:   top+dashLen   →  bottom-dashLen, band left=left
            fillDashes(top + dashLen, bottom - dashLen, false, left);
            // Right side:  top+dashLen   →  bottom-dashLen, band left=right-width
            fillDashes(top + dashLen, bottom - dashLen, false, right - width);
        } else {
            if (r.radiusX == 0.0f && r.radiusY == 0.0f) {
                context->DrawRectangle(r.rect, brush, width, sstyle.Get());
            } else {
                context->DrawRoundedRectangle(r, brush, width, sstyle.Get());
            }
        }
    };

    if (sameStyle) {
        drawStyleRect(rect, m_BorderStyleTop, strokeBrush, strokeWidth);
    } else {
        auto drawSide = [&](BorderStyle style, D2D1_POINT_2F p0, D2D1_POINT_2F p1) {
            if (style == BorderStyle::None || style == BorderStyle::Hidden) return;

            if (style == BorderStyle::Dotted) {
                float dx = p1.x - p0.x;
                float dy = p1.y - p0.y;
                float len = sqrtf(dx*dx + dy*dy);
                if (len < 0.001f) return;

                float ideal_spacing = strokeWidth * 2.0f;
                int num_dots = static_cast<int>(len / ideal_spacing + 0.5f) + 1;
                if (num_dots < 2) num_dots = 2;

                float r_dot = strokeWidth / 2.0f;
                for (int i = 0; i < num_dots; ++i) {
                    float t = static_cast<float>(i) / (num_dots - 1);
                    D2D1_POINT_2F pt = D2D1::Point2F(p0.x + dx * t, p0.y + dy * t);
                    context->FillEllipse(D2D1::Ellipse(pt, r_dot, r_dot), strokeBrush);
                }
                return;
            }

            if (style == BorderStyle::Dashed) {
                // Straight dash run (per-side path, no L-corners)
                float dx = p1.x - p0.x;
                float dy = p1.y - p0.y;
                float len = sqrtf(dx*dx + dy*dy);
                if (len < 0.001f) return;

                float dashL = strokeWidth * 3.0f;
                float gapL  = strokeWidth * 1.0f;
                float period = dashL + gapL;
                int count = std::max(1, static_cast<int>(len / period + 0.5f));
                float actualPeriod = len / static_cast<float>(count);
                float actualDash   = actualPeriod * (dashL / period);

                float ux = dx / len, uy = dy / len;
                // Perpendicular (left of direction)
                float px = -uy * (strokeWidth * 0.5f);
                float py =  ux * (strokeWidth * 0.5f);

                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                ID2D1Factory1* factory = Direct2D::GetFactory();
                if (!factory) return;

                for (int i = 0; i < count; ++i) {
                    float s = i * actualPeriod;
                    float e = s + actualDash;
                    D2D1_POINT_2F a = D2D1::Point2F(p0.x + ux*s + px, p0.y + uy*s + py);
                    D2D1_POINT_2F b = D2D1::Point2F(p0.x + ux*e + px, p0.y + uy*e + py);
                    D2D1_POINT_2F c = D2D1::Point2F(p0.x + ux*e - px, p0.y + uy*e - py);
                    D2D1_POINT_2F d = D2D1::Point2F(p0.x + ux*s - px, p0.y + uy*s - py);

                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> dashPath;
                    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                    if (SUCCEEDED(factory->CreatePathGeometry(&dashPath)) &&
                        SUCCEEDED(dashPath->Open(&sink))) {
                        sink->BeginFigure(a, D2D1_FIGURE_BEGIN_FILLED);
                        sink->AddLine(b);
                        sink->AddLine(c);
                        sink->AddLine(d);
                        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                        sink->Close();
                        context->FillGeometry(dashPath.Get(), strokeBrush);
                    }
                }
                return;
            }

            Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> sstyle;
            getStrokeStyle(style, &sstyle);

            if (style == BorderStyle::Double) {
                context->DrawLine(p0, p1, strokeBrush, strokeWidth, sstyle.Get());
            } else {
                context->DrawLine(p0, p1, strokeBrush, strokeWidth, sstyle.Get());
            }
        };

        drawSide(m_BorderStyleTop,    D2D1::Point2F(rect.rect.left,  rect.rect.top),    D2D1::Point2F(rect.rect.right, rect.rect.top));
        drawSide(m_BorderStyleRight,  D2D1::Point2F(rect.rect.right, rect.rect.top),    D2D1::Point2F(rect.rect.right, rect.rect.bottom));
        drawSide(m_BorderStyleBottom, D2D1::Point2F(rect.rect.right, rect.rect.bottom), D2D1::Point2F(rect.rect.left,  rect.rect.bottom));
        drawSide(m_BorderStyleLeft,   D2D1::Point2F(rect.rect.left,  rect.rect.bottom), D2D1::Point2F(rect.rect.left,  rect.rect.top));
    }
}
