/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_PATH_SHAPE_H__
#define __NOVADESK_PATH_SHAPE_H__

#include "ShapeElement.h"
#include <string>

class PathShape : public ShapeElement
{
public:
    struct CombineOp {
        std::wstring id;
        D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_UNION;
        bool consume = false;
    };

    PathShape(const std::wstring& id, int x, int y, int width, int height);
    virtual ~PathShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual GfxRect GetBounds() override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    virtual bool CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const override;
    
    virtual void SetPathData(const std::wstring& pathData) override;

    void SetCombinedGeometry(Microsoft::WRL::ComPtr<ID2D1Geometry> geometry, const D2D1_RECT_F& bounds);
    void ClearCombinedGeometry();
    bool HasCombinedGeometry() const { return m_HasCombinedGeometry; }

    void SetCombineData(const std::wstring& baseId, const std::vector<CombineOp>& ops, bool consumeBase);
    void GetCombineData(std::wstring& baseId, std::vector<CombineOp>& ops, bool& consumeBase) const;
    bool IsCombineShape() const { return m_IsCombineShape; }

private:
    std::wstring m_PathData;
    bool m_HasPathBounds = false;
    float m_PathMinX = 0.0f;
    float m_PathMinY = 0.0f;
    float m_PathMaxX = 0.0f;
    float m_PathMaxY = 0.0f;

    bool m_IsCombineShape = false;
    std::wstring m_CombineBaseId;
    std::vector<CombineOp> m_CombineOps;
    bool m_CombineConsumeBase = false;

    bool m_HasCombinedGeometry = false;
    Microsoft::WRL::ComPtr<ID2D1Geometry> m_CombinedGeometry;
    GfxRect m_CombinedBounds;

    void CreatePathGeometry(ID2D1Factory* factory, ID2D1PathGeometry** ppGeometry) const;
    void UpdatePathBounds();
};

#endif
