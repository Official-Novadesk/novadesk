/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LINE_ELEMENT_H__
#define __NOVADESK_LINE_ELEMENT_H__

#include "Element.h"

#include <vector>

class LineElement : public Element
{
public:
    LineElement(const std::wstring& id, int x, int y, int w, int h);
    virtual ~LineElement() {}

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual bool HitTest(int x, int y) override;
    virtual int GetAutoWidth() override { return 0; }
    virtual int GetAutoHeight() override { return 0; }

    void SetLineCount(int count);
    int GetLineCount() const { return m_LineCount; }

    void SetDataSets(const std::vector<std::vector<float>>& dataSets);
    const std::vector<std::vector<float>>& GetDataSets() const { return m_DataSets; }

    void SetLineColors(const std::vector<COLORREF>& colors, const std::vector<BYTE>& alphas);
    const std::vector<COLORREF>& GetLineColors() const { return m_LineColors; }
    const std::vector<BYTE>& GetLineAlphas() const { return m_LineAlphas; }
    void SetLineGradients(const std::vector<GradientInfo>& gradients);
    const std::vector<GradientInfo>& GetLineGradients() const { return m_LineGradients; }
    void SetScaleValues(const std::vector<float>& scaleValues);
    const std::vector<float>& GetScaleValues() const { return m_ScaleValues; }

    void SetLineWidth(float width);
    float GetLineWidth() const { return m_LineWidth; }
    void SetMaxPoints(int maxPoints);
    int GetMaxPoints() const { return m_MaxPoints; }

    void SetHorizontalLines(bool enabled) { m_HorizontalLines = enabled; }
    bool GetHorizontalLines() const { return m_HorizontalLines; }

    void SetHorizontalLineColor(COLORREF color, BYTE alpha)
    {
        m_HorizontalLineColor = color;
        m_HorizontalLineAlpha = alpha;
    }
    COLORREF GetHorizontalLineColor() const { return m_HorizontalLineColor; }
    BYTE GetHorizontalLineAlpha() const { return m_HorizontalLineAlpha; }
    void SetHorizontalLineGradient(const GradientInfo& gradient) { m_HorizontalLineGradient = gradient; }
    const GradientInfo& GetHorizontalLineGradient() const { return m_HorizontalLineGradient; }

    void SetGraphStartLeft(bool left) { m_GraphStartLeft = left; }
    bool GetGraphStartLeft() const { return m_GraphStartLeft; }

    void SetGraphHorizontalOrientation(bool horizontal) { m_GraphHorizontalOrientation = horizontal; }
    bool GetGraphHorizontalOrientation() const { return m_GraphHorizontalOrientation; }

    void SetFlip(bool flip) { m_Flip = flip; }
    bool GetFlip() const { return m_Flip; }

    void SetStrokeTransformType(D2D1_STROKE_TRANSFORM_TYPE type) { m_StrokeType = type; }
    D2D1_STROKE_TRANSFORM_TYPE GetStrokeTransformType() const { return m_StrokeType; }

    void SetAutoRange(bool enabled) { m_AutoRange = enabled; }
    bool GetAutoRange() const { return m_AutoRange; }

    void SetScaleRange(float minValue, float maxValue);
    float GetScaleMin() const { return m_ScaleMin; }
    float GetScaleMax() const { return m_ScaleMax; }

private:
    void EnsureStorage();
    bool BuildAutoRange(float& outMin, float& outMax) const;
    bool MapPoint(int dataIndex, int pointIndex, int totalPoints, int capacityPoints, float minValue, float maxValue, D2D1_POINT_2F& outPoint);
    static float DistancePointToSegment(const D2D1_POINT_2F& p, const D2D1_POINT_2F& a, const D2D1_POINT_2F& b);

private:
    int m_LineCount = 1;
    std::vector<std::vector<float>> m_DataSets;
    std::vector<COLORREF> m_LineColors;
    std::vector<BYTE> m_LineAlphas;
    std::vector<GradientInfo> m_LineGradients;
    std::vector<float> m_ScaleValues;
    float m_LineWidth = 1.0f;
    int m_MaxPoints = 0;
    bool m_HorizontalLines = false;
    COLORREF m_HorizontalLineColor = RGB(0, 0, 0);
    BYTE m_HorizontalLineAlpha = 255;
    GradientInfo m_HorizontalLineGradient;
    bool m_GraphStartLeft = false;
    bool m_GraphHorizontalOrientation = false;
    bool m_Flip = false;
    D2D1_STROKE_TRANSFORM_TYPE m_StrokeType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
    bool m_AutoRange = false;
    float m_ScaleMin = 0.0f;
    float m_ScaleMax = 100.0f;
};

#endif
