/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SHAPE_ELEMENT_H__
#define __NOVADESK_SHAPE_ELEMENT_H__

#include "Element.h"
#include <wrl/client.h>

class ShapeElement : public Element
{
public:
    ShapeElement(const std::wstring& id, int x, int y, int width, int height);
    virtual ~ShapeElement();

    virtual void Render(ID2D1DeviceContext* context) = 0;

    void SetStroke(float width, COLORREF color, BYTE alpha) {
        m_StrokeWidth = width;
        m_StrokeColor = color;
        m_StrokeAlpha = alpha;
        m_HasStroke = true;
        m_HasStrokeGradient = false;
    }

    void SetStrokeGradient(const GradientInfo& gradient) {
        m_StrokeGradient = gradient;
        m_HasStrokeGradient = true;
        m_HasStroke = true;
    }

    void SetFill(COLORREF color, BYTE alpha) {
        m_FillColor = color;
        m_FillAlpha = alpha;
        m_HasFill = true;
        m_HasFillGradient = false;
    }

    void SetFillGradient(const GradientInfo& gradient) {
        m_FillGradient = gradient;
        m_HasFillGradient = true;
        m_HasFill = true;
    }

    // Getters
    bool HasStroke() const { return m_HasStroke; }
    float GetStrokeWidth() const { return m_StrokeWidth; }
    COLORREF GetStrokeColor() const { return m_StrokeColor; }
    BYTE GetStrokeAlpha() const { return m_StrokeAlpha; }
    bool HasStrokeGradient() const { return m_HasStrokeGradient; }
    const GradientInfo& GetStrokeGradient() const { return m_StrokeGradient; }

    bool HasFill() const { return m_HasFill; }
    COLORREF GetFillColor() const { return m_FillColor; }
    BYTE GetFillAlpha() const { return m_FillAlpha; }
    bool HasFillGradient() const { return m_HasFillGradient; }
    const GradientInfo& GetFillGradient() const { return m_FillGradient; }

    D2D1_CAP_STYLE GetStrokeStartCap() const { return m_StrokeStartCap; }
    D2D1_CAP_STYLE GetStrokeEndCap() const { return m_StrokeEndCap; }
    D2D1_CAP_STYLE GetStrokeDashCap() const { return m_StrokeDashCap; }
    D2D1_LINE_JOIN GetStrokeLineJoin() const { return m_StrokeLineJoin; }
    float GetStrokeDashOffset() const { return m_StrokeDashOffset; }
    const std::vector<float>& GetStrokeDashes() const { return m_StrokeDashes; }

    // Virtual Getters for specialized shapes (optional or dynamic_cast in PreFill)
    virtual float GetRadiusX() const { return 0; }
    virtual float GetRadiusY() const { return 0; }
    virtual float GetStartX() const { return 0; }
    virtual float GetStartY() const { return 0; }
    virtual float GetEndX() const { return 0; }
    virtual float GetEndY() const { return 0; }
    virtual float GetStartAngle() const { return 0; }
    virtual float GetEndAngle() const { return 0; }
    virtual bool IsClockwise() const { return true; }
    virtual std::wstring GetPathData() const { return L""; }
    GfxRect GetBackgroundBounds() override;

    void SetStrokeStyle(D2D1_CAP_STYLE start, D2D1_CAP_STYLE end, D2D1_CAP_STYLE dash, D2D1_LINE_JOIN join, float offset, const std::vector<float>& dashes) {
        m_StrokeStartCap = start;
        m_StrokeEndCap = end;
        m_StrokeDashCap = dash;
        m_StrokeLineJoin = join;
        m_StrokeDashOffset = offset;
        m_StrokeDashes = dashes;
        m_UpdateStrokeStyle = true;
    }

    virtual void SetRadii(float rx, float ry) {}
    virtual void SetLinePoints(float x1, float y1, float x2, float y2) {}
    virtual void SetArcParams(float startAngle, float endAngle, bool clockwise) {}

    virtual void SetPathData(const std::wstring& pathData) {}

protected:

    bool m_HasStroke = false;
    float m_StrokeWidth = 1.0f;
    COLORREF m_StrokeColor = RGB(0, 0, 0);
    BYTE m_StrokeAlpha = 255;

    bool m_HasStrokeGradient = false;
    GradientInfo m_StrokeGradient;

    bool m_HasFill = false;
    COLORREF m_FillColor = RGB(255, 255, 255);
    BYTE m_FillAlpha = 255;

    bool m_HasFillGradient = false;
    GradientInfo m_FillGradient;

    D2D1_CAP_STYLE m_StrokeStartCap = D2D1_CAP_STYLE_FLAT;
    D2D1_CAP_STYLE m_StrokeEndCap = D2D1_CAP_STYLE_FLAT;
    D2D1_CAP_STYLE m_StrokeDashCap = D2D1_CAP_STYLE_FLAT;
    D2D1_LINE_JOIN m_StrokeLineJoin = D2D1_LINE_JOIN_MITER;
    float m_StrokeDashOffset = 0.0f;
    std::vector<float> m_StrokeDashes;

    bool m_UpdateStrokeStyle = false;
    ID2D1StrokeStyle1* m_StrokeStyle = nullptr;

    void CreateBrush(ID2D1DeviceContext* context, ID2D1Brush** ppBrush, bool isStroke);
    void UpdateStrokeStyle(ID2D1DeviceContext* context);

    bool TryCreateStrokeBrush(ID2D1DeviceContext* context, Microsoft::WRL::ComPtr<ID2D1Brush>& outBrush);
    bool TryCreateFillBrush(ID2D1DeviceContext* context, Microsoft::WRL::ComPtr<ID2D1Brush>& outBrush);
};

#endif
