/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_IMAGE_ELEMENT_H__
#define __NOVADESK_IMAGE_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"
#include <wrl/client.h>
#include <d2d1.h>

enum ImageAspectRatio
{
    IMAGE_ASPECT_STRETCH,  // 0
    IMAGE_ASPECT_PRESERVE, // 1
    IMAGE_ASPECT_CROP      // 2
};

class ImageElement : public Element
{
public:
    ImageElement(const std::wstring &id, int x, int y, int w, int h,
                 const std::wstring &path);

    virtual ~ImageElement();

    virtual void Render(ID2D1DeviceContext *context) override;

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    // Returns true if image loaded successfully
    bool IsLoaded() const { return m_GeneralImage.IsLoaded(); }

    void UpdateImage(const std::wstring &path);

    virtual bool HitTest(int x, int y) override;

    void SetPreserveAspectRatio(ImageAspectRatio mode) { m_PreserveAspectRatio = mode; }
    void SetImageTint(COLORREF color, BYTE alpha) { m_GeneralImage.SetImageTint(color, alpha); }
    void SetImageAlpha(BYTE alpha) { m_GeneralImage.SetImageAlpha(alpha); }
    void SetGrayscale(bool enable) { m_GeneralImage.SetGrayscale(enable); }
    void SetColorMatrix(const float *matrix) { m_GeneralImage.SetColorMatrix(matrix); }
    void SetTile(bool tile) { m_Tile = tile; }
    void SetImageFlip(ImageFlipMode flip) { m_GeneralImage.SetImageFlip(flip); }
    void SetUseExifOrientation(bool enabled) { m_GeneralImage.SetUseExifOrientation(enabled); }
    void SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin) { m_GeneralImage.SetImageCrop(x, y, w, h, origin); }
    void ClearImageCrop() { m_GeneralImage.ClearImageCrop(); }
    void SetScaleMargins(float left, float top, float right, float bottom);
    void ClearScaleMargins() { m_HasScaleMargins = false; }

    const std::wstring &GetImagePath() const { return m_GeneralImage.GetPath(); }
    ImageAspectRatio GetPreserveAspectRatio() const { return m_PreserveAspectRatio; }
    bool HasImageTint() const { return m_GeneralImage.HasImageTint(); }
    COLORREF GetImageTint() const { return m_GeneralImage.GetImageTint(); }
    BYTE GetImageTintAlpha() const { return m_GeneralImage.GetImageTintAlpha(); }
    BYTE GetImageAlpha() const { return m_GeneralImage.GetImageAlpha(); }
    bool IsGrayscale() const { return m_GeneralImage.IsGrayscale(); }
    bool IsTile() const { return m_Tile; }
    ImageFlipMode GetImageFlip() const { return m_GeneralImage.GetImageFlip(); }
    bool GetUseExifOrientation() const { return m_GeneralImage.GetUseExifOrientation(); }
    bool HasImageCrop() const { return m_GeneralImage.HasImageCrop(); }
    float GetImageCropX() const { return m_GeneralImage.GetImageCropX(); }
    float GetImageCropY() const { return m_GeneralImage.GetImageCropY(); }
    float GetImageCropW() const { return m_GeneralImage.GetImageCropW(); }
    float GetImageCropH() const { return m_GeneralImage.GetImageCropH(); }
    ImageCropOrigin GetImageCropOrigin() const { return m_GeneralImage.GetImageCropOrigin(); }
    bool HasScaleMargins() const { return m_HasScaleMargins; }
    float GetScaleMarginLeft() const { return m_ScaleMarginLeft; }
    float GetScaleMarginTop() const { return m_ScaleMarginTop; }
    float GetScaleMarginRight() const { return m_ScaleMarginRight; }
    float GetScaleMarginBottom() const { return m_ScaleMarginBottom; }
    bool HasColorMatrix() const { return m_GeneralImage.HasColorMatrix(); }
    const float *GetColorMatrix() const { return m_GeneralImage.GetColorMatrix(); }

private:
    struct ImageLayout
    {
        int contentX = 0;
        int contentY = 0;
        int contentW = 0;
        int contentH = 0;
        D2D1_RECT_F finalRect = D2D1::RectF(0, 0, 0, 0);
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, 0, 0);
    };

    GeneralImage m_GeneralImage;
    ImageAspectRatio m_PreserveAspectRatio = IMAGE_ASPECT_STRETCH;
    bool m_Tile = false;
    bool m_HasScaleMargins = false;
    float m_ScaleMarginLeft = 0.0f;
    float m_ScaleMarginTop = 0.0f;
    float m_ScaleMarginRight = 0.0f;
    float m_ScaleMarginBottom = 0.0f;

    bool ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const;
    bool ComputeImageLayout(float imageWidth, float imageHeight, ImageLayout &layout);
    bool MapPointToImagePixel(float targetX, float targetY, UINT imageWidth, UINT imageHeight, float &pixelX, float &pixelY);
};

#endif
