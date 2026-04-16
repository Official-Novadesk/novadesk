/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_GENERAL_IMAGE_H__
#define __NOVADESK_GENERAL_IMAGE_H__

#include <array>
#include <string>

#include <d2d1_1.h>
#include <wincodec.h>
#include <wrl/client.h>

enum ImageFlipMode
{
    IMAGE_FLIP_NONE = 0,
    IMAGE_FLIP_HORIZONTAL,
    IMAGE_FLIP_VERTICAL,
    IMAGE_FLIP_BOTH
};

enum ImageCropOrigin
{
    IMAGE_CROP_ORIGIN_TOP_LEFT = 0,
    IMAGE_CROP_ORIGIN_TOP_RIGHT = 1,
    IMAGE_CROP_ORIGIN_BOTTOM_RIGHT = 2,
    IMAGE_CROP_ORIGIN_BOTTOM_LEFT = 3,
    IMAGE_CROP_ORIGIN_CENTER = 4
};

class GeneralImage
{
public:
    GeneralImage();

    void SetPath(const std::wstring &path);
    const std::wstring &GetPath() const { return m_ImagePath; }

    void EnsureBitmap(ID2D1DeviceContext *context);
    bool IsLoaded() const { return m_D2DBitmap != nullptr; }
    ID2D1Bitmap *GetBitmap() const { return m_D2DBitmap.Get(); }
    IWICBitmap *GetWICBitmap() const { return m_pWICBitmap.Get(); }

    BYTE GetPixelAlpha(int x, int y) const;

    void SetImageTint(COLORREF color, BYTE alpha);
    bool HasImageTint() const { return m_HasImageTint; }
    COLORREF GetImageTint() const { return m_ImageTint; }
    BYTE GetImageTintAlpha() const { return m_ImageTintAlpha; }

    void SetImageAlpha(BYTE alpha) { m_ImageAlpha = alpha; }
    BYTE GetImageAlpha() const { return m_ImageAlpha; }

    void SetGrayscale(bool enable) { m_Grayscale = enable; }
    bool IsGrayscale() const { return m_Grayscale; }

    void SetColorMatrix(const float *matrix);
    bool HasColorMatrix() const { return m_HasColorMatrix; }
    const float *GetColorMatrix() const { return m_ColorMatrix.data(); }

    void SetImageFlip(ImageFlipMode flip) { m_ImageFlip = flip; }
    ImageFlipMode GetImageFlip() const { return m_ImageFlip; }

    void SetUseExifOrientation(bool enabled);
    bool GetUseExifOrientation() const { return m_UseExifOrientation; }

    void SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin);
    void ClearImageCrop() { m_HasImageCrop = false; }
    bool HasImageCrop() const { return m_HasImageCrop; }
    float GetImageCropX() const { return m_ImageCropX; }
    float GetImageCropY() const { return m_ImageCropY; }
    float GetImageCropW() const { return m_ImageCropW; }
    float GetImageCropH() const { return m_ImageCropH; }
    ImageCropOrigin GetImageCropOrigin() const { return m_ImageCropOrigin; }

    bool ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const;
    void ApplyFlipToPixel(float &pixelX, float &pixelY, const D2D1_RECT_F &srcRect) const;
    bool BuildFlipTransform(const D2D1_RECT_F &dstRect, D2D1_MATRIX_3X2_F &outTransform) const;

    bool BuildProcessedImage(ID2D1DeviceContext *context, Microsoft::WRL::ComPtr<ID2D1Image> &outImage) const;

    int GetAutoWidth() const;
    int GetAutoHeight() const;

private:
    void ReloadWICBitmap();
    void ResetBitmapCache();

private:
    std::wstring m_ImagePath;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_D2DBitmap;
    Microsoft::WRL::ComPtr<IWICBitmap> m_pWICBitmap;
    ID2D1RenderTarget *m_pLastTarget = nullptr;

    bool m_HasImageTint = false;
    COLORREF m_ImageTint = RGB(0, 0, 0);
    BYTE m_ImageTintAlpha = 255;
    BYTE m_ImageAlpha = 255;
    bool m_Grayscale = false;
    bool m_HasColorMatrix = false;
    std::array<float, 20> m_ColorMatrix{};
    ImageFlipMode m_ImageFlip = IMAGE_FLIP_NONE;
    bool m_UseExifOrientation = false;

    bool m_HasImageCrop = false;
    float m_ImageCropX = 0.0f;
    float m_ImageCropY = 0.0f;
    float m_ImageCropW = 0.0f;
    float m_ImageCropH = 0.0f;
    ImageCropOrigin m_ImageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
};

#endif
