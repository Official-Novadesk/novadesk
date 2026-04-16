/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ROTATOR_ELEMENT_H__
#define __NOVADESK_ROTATOR_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"

class RotatorElement : public Element
{
public:
    RotatorElement(const std::wstring &id, int x, int y, const std::wstring &path);
    virtual ~RotatorElement();

    virtual void Render(ID2D1DeviceContext *context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    bool IsLoaded() const { return m_RotatorImage.IsLoaded(); }
    void UpdateImage(const std::wstring &path) { m_RotatorImage.SetPath(path); }

    void SetValue(double value) { m_Value = value; }
    void SetOffsetX(double offsetX) { m_OffsetX = offsetX; }
    void SetOffsetY(double offsetY) { m_OffsetY = offsetY; }
    void SetStartAngle(double startAngle) { m_StartAngle = startAngle; }
    void SetRotationAngle(double rotationAngle) { m_RotationAngle = rotationAngle; }
    void SetValueRemainder(int valueRemainder) { m_ValueRemainder = (valueRemainder < 0) ? 0 : valueRemainder; }
    void SetMinValue(double minValue) { m_MinValue = minValue; }
    void SetMaxValue(double maxValue) { m_MaxValue = (maxValue > m_MinValue) ? maxValue : (m_MinValue + 0.001); }

    void SetImageTint(COLORREF color, BYTE alpha) { m_RotatorImage.SetImageTint(color, alpha); }
    void SetImageAlpha(BYTE alpha) { m_RotatorImage.SetImageAlpha(alpha); }
    void SetGrayscale(bool enable) { m_RotatorImage.SetGrayscale(enable); }
    void SetColorMatrix(const float *matrix) { m_RotatorImage.SetColorMatrix(matrix); }
    void SetUseExifOrientation(bool enabled) { m_RotatorImage.SetUseExifOrientation(enabled); }
    void SetImageFlip(ImageFlipMode flip) { m_RotatorImage.SetImageFlip(flip); }

    const std::wstring &GetImagePath() const { return m_RotatorImage.GetPath(); }
    double GetValue() const { return m_Value; }
    double GetOffsetX() const { return m_OffsetX; }
    double GetOffsetY() const { return m_OffsetY; }
    double GetStartAngle() const { return m_StartAngle; }
    double GetRotationAngle() const { return m_RotationAngle; }
    int GetValueRemainder() const { return m_ValueRemainder; }
    double GetMinValue() const { return m_MinValue; }
    double GetMaxValue() const { return m_MaxValue; }

    bool HasImageTint() const { return m_RotatorImage.HasImageTint(); }
    COLORREF GetImageTint() const { return m_RotatorImage.GetImageTint(); }
    BYTE GetImageTintAlpha() const { return m_RotatorImage.GetImageTintAlpha(); }
    BYTE GetImageAlpha() const { return m_RotatorImage.GetImageAlpha(); }
    bool IsGrayscale() const { return m_RotatorImage.IsGrayscale(); }
    bool GetUseExifOrientation() const { return m_RotatorImage.GetUseExifOrientation(); }
    bool HasColorMatrix() const { return m_RotatorImage.HasColorMatrix(); }
    const float *GetColorMatrix() const { return m_RotatorImage.GetColorMatrix(); }
    ImageFlipMode GetImageFlip() const { return m_RotatorImage.GetImageFlip(); }

private:
    GeneralImage m_RotatorImage;

    double m_Value = 0.0;
    double m_OffsetX = 0.0;
    double m_OffsetY = 0.0;
    double m_StartAngle = 0.0;
    double m_RotationAngle = 6.283185307179586; // 2 * PI
    int m_ValueRemainder = 0;
    double m_MinValue = 0.0;
    double m_MaxValue = 1.0;
};

#endif
