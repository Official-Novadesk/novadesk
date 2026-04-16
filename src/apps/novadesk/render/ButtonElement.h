/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BUTTON_ELEMENT_H__
#define __NOVADESK_BUTTON_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"
#include <wrl/client.h>
#include <d2d1.h>
#include <string>

enum ButtonState
{
    BUTTON_STATE_NORMAL = 0,
    BUTTON_STATE_CLICKED = 1,
    BUTTON_STATE_HOVERED = 2
};

class ButtonElement : public Element
{
public:
    ButtonElement(const std::wstring &id, int x, int y, const std::wstring &path);
    virtual ~ButtonElement();

    virtual void Render(ID2D1DeviceContext *context) override;

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    bool IsLoaded() const { return m_ButtonImage.IsLoaded(); }

    void UpdateImage(const std::wstring &path);

    virtual bool HitTest(int x, int y) override;
    
    // Transparent area is ignored
    virtual bool IsTransparentHit() const override { return true; }

    void SetImageTint(COLORREF color, BYTE alpha) { m_ButtonImage.SetImageTint(color, alpha); }
    void SetImageAlpha(BYTE alpha) { m_ButtonImage.SetImageAlpha(alpha); }
    void SetGrayscale(bool enable) { m_ButtonImage.SetGrayscale(enable); }
    void SetColorMatrix(const float *matrix) { m_ButtonImage.SetColorMatrix(matrix); }
    void SetUseExifOrientation(bool enabled) { m_ButtonImage.SetUseExifOrientation(enabled); }
    void SetImageFlip(ImageFlipMode flip) { m_ButtonImage.SetImageFlip(flip); }
    void SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin) { m_ButtonImage.SetImageCrop(x, y, w, h, origin); }
    void ClearImageCrop() { m_ButtonImage.ClearImageCrop(); }

    const std::wstring &GetImagePath() const { return m_ButtonImage.GetPath(); }
    bool HasImageTint() const { return m_ButtonImage.HasImageTint(); }
    COLORREF GetImageTint() const { return m_ButtonImage.GetImageTint(); }
    BYTE GetImageTintAlpha() const { return m_ButtonImage.GetImageTintAlpha(); }
    BYTE GetImageAlpha() const { return m_ButtonImage.GetImageAlpha(); }
    bool IsGrayscale() const { return m_ButtonImage.IsGrayscale(); }
    bool GetUseExifOrientation() const { return m_ButtonImage.GetUseExifOrientation(); }
    bool HasColorMatrix() const { return m_ButtonImage.HasColorMatrix(); }
    const float *GetColorMatrix() const { return m_ButtonImage.GetColorMatrix(); }
    ImageFlipMode GetImageFlip() const { return m_ButtonImage.GetImageFlip(); }
    bool HasImageCrop() const { return m_ButtonImage.HasImageCrop(); }
    float GetImageCropX() const { return m_ButtonImage.GetImageCropX(); }
    float GetImageCropY() const { return m_ButtonImage.GetImageCropY(); }
    float GetImageCropW() const { return m_ButtonImage.GetImageCropW(); }
    float GetImageCropH() const { return m_ButtonImage.GetImageCropH(); }
    ImageCropOrigin GetImageCropOrigin() const { return m_ButtonImage.GetImageCropOrigin(); }

    void SetButtonState(ButtonState state) { m_State = state; }
    ButtonState GetButtonState() const { return m_State; }

protected:
    ButtonState m_State = BUTTON_STATE_NORMAL;
    GeneralImage m_ButtonImage;
};

#endif
