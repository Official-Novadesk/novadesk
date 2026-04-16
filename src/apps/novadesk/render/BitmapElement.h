/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BITMAP_ELEMENT_H__
#define __NOVADESK_BITMAP_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"

enum BitmapAlign
{
    BITMAP_ALIGN_LEFT = 0,
    BITMAP_ALIGN_CENTER = 1,
    BITMAP_ALIGN_RIGHT = 2
};

class BitmapElement : public Element
{
public:
    BitmapElement(const std::wstring &id, int x, int y, const std::wstring &path);
    virtual ~BitmapElement();

    virtual void Render(ID2D1DeviceContext *context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    bool IsLoaded() const { return m_BitmapImage.IsLoaded(); }
    void UpdateImage(const std::wstring &path) { m_BitmapImage.SetPath(path); }

    void SetValue(double value);
    void SetBitmapFrames(int frameCount);
    void SetBitmapZeroFrame(bool zeroFrame) { m_ZeroFrame = zeroFrame; }
    void SetBitmapExtend(bool extend) { m_Extend = extend; }
    void SetMinValue(double minValue) { m_MinValue = minValue; }
    void SetMaxValue(double maxValue) { m_MaxValue = (maxValue > m_MinValue) ? maxValue : (m_MinValue + 0.001); }
    void SetBitmapDigits(int digits) { m_Digits = (digits < 0) ? 0 : digits; }
    void SetBitmapOrientation(const std::wstring &orientation);
    void SetBitmapAlign(BitmapAlign align) { m_Align = align; }
    void SetBitmapSeparation(int separation) { m_Separation = separation; }

    void SetImageTint(COLORREF color, BYTE alpha) { m_BitmapImage.SetImageTint(color, alpha); }
    void SetImageAlpha(BYTE alpha) { m_BitmapImage.SetImageAlpha(alpha); }
    void SetGrayscale(bool enable) { m_BitmapImage.SetGrayscale(enable); }
    void SetColorMatrix(const float *matrix) { m_BitmapImage.SetColorMatrix(matrix); }
    void SetUseExifOrientation(bool enabled) { m_BitmapImage.SetUseExifOrientation(enabled); }
    void SetImageFlip(ImageFlipMode flip) { m_BitmapImage.SetImageFlip(flip); }

    const std::wstring &GetImagePath() const { return m_BitmapImage.GetPath(); }
    double GetValue() const { return m_Value; }
    int GetBitmapFrames() const { return m_FrameCount; }
    bool GetBitmapZeroFrame() const { return m_ZeroFrame; }
    bool GetBitmapExtend() const { return m_Extend; }
    double GetMinValue() const { return m_MinValue; }
    double GetMaxValue() const { return m_MaxValue; }
    int GetBitmapDigits() const { return m_Digits; }
    std::wstring GetBitmapOrientation() const;
    BitmapAlign GetBitmapAlign() const { return m_Align; }
    int GetBitmapSeparation() const { return m_Separation; }

    bool HasImageTint() const { return m_BitmapImage.HasImageTint(); }
    COLORREF GetImageTint() const { return m_BitmapImage.GetImageTint(); }
    BYTE GetImageTintAlpha() const { return m_BitmapImage.GetImageTintAlpha(); }
    BYTE GetImageAlpha() const { return m_BitmapImage.GetImageAlpha(); }
    bool IsGrayscale() const { return m_BitmapImage.IsGrayscale(); }
    bool GetUseExifOrientation() const { return m_BitmapImage.GetUseExifOrientation(); }
    bool HasColorMatrix() const { return m_BitmapImage.HasColorMatrix(); }
    const float *GetColorMatrix() const { return m_BitmapImage.GetColorMatrix(); }
    ImageFlipMode GetImageFlip() const { return m_BitmapImage.GetImageFlip(); }

private:
    bool ComputeFrameGeometry(float imageWidth, float imageHeight, bool &verticalFrames, float &frameWidth, float &frameHeight) const;
    int GetRealFrameCount() const;
    int GetDigitCountForValue(long long value, int realFrames) const;
    int ResolveFrameForNormalizedValue(double value) const;
    int ResolveFrameForDigit(int digit);

private:
    GeneralImage m_BitmapImage;

    double m_Value = 0.0;
    int m_FrameCount = 1;
    bool m_ZeroFrame = false;
    bool m_Extend = false;
    double m_MinValue = 0.0;
    double m_MaxValue = 1.0;
    int m_Digits = 0;
    BitmapAlign m_Align = BITMAP_ALIGN_LEFT;
    int m_Separation = 0;
    bool m_Vertical = false;
    bool m_AutoOrientation = true;
};

#endif
