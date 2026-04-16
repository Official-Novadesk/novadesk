/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "GeneralImage.h"

#include "Direct2DHelper.h"
#include "../shared/Logging.h"

#include <algorithm>
#include <cstring>
#include <d2d1effects.h>

GeneralImage::GeneralImage()
{
    m_ColorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

void GeneralImage::ResetBitmapCache()
{
    m_D2DBitmap.Reset();
}

void GeneralImage::ReloadWICBitmap()
{
    m_pWICBitmap.Reset();
    if (m_ImagePath.empty())
        return;

    const bool ok = Direct2D::LoadWICBitmapFromFile(m_ImagePath, m_pWICBitmap.ReleaseAndGetAddressOf(), m_UseExifOrientation);
    if (!ok)
    {
        Logging::Log(LogLevel::Error, L"[novadesk] failed to preload WIC image: %s", m_ImagePath.c_str());
    }
}

void GeneralImage::SetPath(const std::wstring &path)
{
    m_ImagePath = path;
    ResetBitmapCache();
    ReloadWICBitmap();
}

void GeneralImage::EnsureBitmap(ID2D1DeviceContext *context)
{
    if (!context)
        return;

    if (m_pLastTarget != (ID2D1RenderTarget *)context)
    {
        m_D2DBitmap.Reset();
        m_pLastTarget = (ID2D1RenderTarget *)context;
    }

    if (!m_D2DBitmap)
    {
        const bool ok = Direct2D::LoadBitmapFromFile(
            context,
            m_ImagePath,
            m_D2DBitmap.ReleaseAndGetAddressOf(),
            m_pWICBitmap.ReleaseAndGetAddressOf(),
            m_UseExifOrientation);
        if (!ok)
        {
            Logging::Log(LogLevel::Error, L"[novadesk] failed to load image bitmap: %s", m_ImagePath.c_str());
        }
    }
}

void GeneralImage::SetImageTint(COLORREF color, BYTE alpha)
{
    m_ImageTint = color;
    m_ImageTintAlpha = alpha;
    m_HasImageTint = (alpha > 0);
}

void GeneralImage::SetColorMatrix(const float *matrix)
{
    if (matrix)
    {
        memcpy(m_ColorMatrix.data(), matrix, sizeof(float) * 20);
        m_HasColorMatrix = true;
    }
    else
    {
        m_HasColorMatrix = false;
    }
}

void GeneralImage::SetUseExifOrientation(bool enabled)
{
    if (m_UseExifOrientation == enabled)
        return;

    m_UseExifOrientation = enabled;
    ResetBitmapCache();
    ReloadWICBitmap();
}

void GeneralImage::SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin)
{
    if (w <= 0.0f || h <= 0.0f)
    {
        m_HasImageCrop = false;
        return;
    }

    m_HasImageCrop = true;
    m_ImageCropX = x;
    m_ImageCropY = y;
    m_ImageCropW = w;
    m_ImageCropH = h;
    m_ImageCropOrigin = origin;
}

bool GeneralImage::ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const
{
    if (!m_HasImageCrop || imageWidth <= 0.0f || imageHeight <= 0.0f || m_ImageCropW <= 0.0f || m_ImageCropH <= 0.0f)
    {
        return false;
    }

    float startX = m_ImageCropX;
    float startY = m_ImageCropY;

    switch (m_ImageCropOrigin)
    {
    case IMAGE_CROP_ORIGIN_TOP_RIGHT:
        startX = imageWidth + m_ImageCropX;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_RIGHT:
        startX = imageWidth + m_ImageCropX;
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_LEFT:
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_CENTER:
        startX = (imageWidth * 0.5f) + m_ImageCropX;
        startY = (imageHeight * 0.5f) + m_ImageCropY;
        break;
    default:
        break;
    }

    float left = startX;
    float top = startY;
    float right = startX + m_ImageCropW;
    float bottom = startY + m_ImageCropH;

    left = (std::max)(0.0f, left);
    top = (std::max)(0.0f, top);
    right = (std::min)(imageWidth, right);
    bottom = (std::min)(imageHeight, bottom);

    if (right <= left || bottom <= top)
    {
        return false;
    }

    rect = D2D1::RectF(left, top, right, bottom);
    return true;
}

void GeneralImage::ApplyFlipToPixel(float &pixelX, float &pixelY, const D2D1_RECT_F &srcRect) const
{
    switch (m_ImageFlip)
    {
    case IMAGE_FLIP_HORIZONTAL:
        pixelX = srcRect.right - (pixelX - srcRect.left) - 0.001f;
        break;
    case IMAGE_FLIP_VERTICAL:
        pixelY = srcRect.bottom - (pixelY - srcRect.top) - 0.001f;
        break;
    case IMAGE_FLIP_BOTH:
        pixelX = srcRect.right - (pixelX - srcRect.left) - 0.001f;
        pixelY = srcRect.bottom - (pixelY - srcRect.top) - 0.001f;
        break;
    default:
        break;
    }
}

bool GeneralImage::BuildFlipTransform(const D2D1_RECT_F &dstRect, D2D1_MATRIX_3X2_F &outTransform) const
{
    if (m_ImageFlip == IMAGE_FLIP_NONE)
        return false;

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (m_ImageFlip == IMAGE_FLIP_HORIZONTAL || m_ImageFlip == IMAGE_FLIP_BOTH)
        scaleX = -1.0f;
    if (m_ImageFlip == IMAGE_FLIP_VERTICAL || m_ImageFlip == IMAGE_FLIP_BOTH)
        scaleY = -1.0f;

    const float centerX = (dstRect.left + dstRect.right) * 0.5f;
    const float centerY = (dstRect.top + dstRect.bottom) * 0.5f;
    outTransform =
        D2D1::Matrix3x2F::Translation(-centerX, -centerY) *
        D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
        D2D1::Matrix3x2F::Translation(centerX, centerY);
    return true;
}

bool GeneralImage::BuildProcessedImage(ID2D1DeviceContext *context, Microsoft::WRL::ComPtr<ID2D1Image> &outImage) const
{
    if (!context || !m_D2DBitmap)
        return false;

    outImage = m_D2DBitmap.Get();

    if (!(m_Grayscale || m_HasImageTint || m_HasColorMatrix || m_ImageAlpha < 255))
    {
        return true;
    }

    Microsoft::WRL::ComPtr<ID2D1Image> current = m_D2DBitmap.Get();

    if (m_Grayscale)
    {
        Microsoft::WRL::ComPtr<ID2D1Effect> grayEffect;
        if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, grayEffect.GetAddressOf())))
        {
            D2D1_MATRIX_5X4_F grayMatrix = D2D1::Matrix5x4F(
                0.299f, 0.299f, 0.299f, 0.0f,
                0.587f, 0.587f, 0.587f, 0.0f,
                0.114f, 0.114f, 0.114f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f);
            grayEffect->SetInput(0, current.Get());
            grayEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, grayMatrix);
            grayEffect->GetOutput(&current);
        }
    }

    Microsoft::WRL::ComPtr<ID2D1Effect> colorEffect;
    if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, colorEffect.GetAddressOf())))
    {
        D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        if (m_HasColorMatrix)
        {
            memcpy(&matrix, m_ColorMatrix.data(), sizeof(float) * 20);
        }
        else
        {
            if (m_HasImageTint)
            {
                matrix.m[0][0] = GetRValue(m_ImageTint) / 255.0f;
                matrix.m[1][1] = GetGValue(m_ImageTint) / 255.0f;
                matrix.m[2][2] = GetBValue(m_ImageTint) / 255.0f;
                matrix.m[3][3] = m_ImageTintAlpha / 255.0f;
            }
            matrix.m[3][3] *= (m_ImageAlpha / 255.0f);
        }

        colorEffect->SetInput(0, current.Get());
        colorEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
        colorEffect->GetOutput(&current);
    }

    outImage = current;
    return true;
}

int GeneralImage::GetAutoWidth() const
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
            return (int)(cropRect.right - cropRect.left);
        return (int)w;
    }
    if (m_D2DBitmap)
    {
        D2D1_SIZE_F size = m_D2DBitmap->GetSize();
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect(size.width, size.height, cropRect))
            return (int)(cropRect.right - cropRect.left);
        return (int)size.width;
    }
    return 0;
}

int GeneralImage::GetAutoHeight() const
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
            return (int)(cropRect.bottom - cropRect.top);
        return (int)h;
    }
    if (m_D2DBitmap)
    {
        D2D1_SIZE_F size = m_D2DBitmap->GetSize();
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect(size.width, size.height, cropRect))
            return (int)(cropRect.bottom - cropRect.top);
        return (int)size.height;
    }
    return 0;
}

BYTE GeneralImage::GetPixelAlpha(int x, int y) const
{
    if (!m_pWICBitmap)
        return 0;

    UINT width = 0, height = 0;
    m_pWICBitmap->GetSize(&width, &height);
    if (x < 0 || x >= (int)width || y < 0 || y >= (int)height)
        return 0;

    Microsoft::WRL::ComPtr<IWICBitmapLock> pLock;
    WICRect rcLock = { x, y, 1, 1 };
    if (SUCCEEDED(m_pWICBitmap->Lock(&rcLock, WICBitmapLockRead, pLock.GetAddressOf())))
    {
        UINT cbBufferSize = 0;
        BYTE* pv = nullptr;
        if (SUCCEEDED(pLock->GetDataPointer(&cbBufferSize, &pv)) && cbBufferSize >= 4)
        {
            // 32bppPBGRA format used in Direct2DHelper
            return pv[3];
        }
    }
    return 0;
}
