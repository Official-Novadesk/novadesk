/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ImageElement.h"
#include <algorithm>

ImageElement::ImageElement(const std::wstring &id, int x, int y, int w, int h,
                           const std::wstring &path)
    : Element(ELEMENT_IMAGE, id, x, y, w, h)
{
    m_GeneralImage.SetPath(path);
}

ImageElement::~ImageElement()
{
}

void ImageElement::SetScaleMargins(float left, float top, float right, float bottom)
{
    left = (std::max)(0.0f, left);
    top = (std::max)(0.0f, top);
    right = (std::max)(0.0f, right);
    bottom = (std::max)(0.0f, bottom);

    if (left <= 0.0f && top <= 0.0f && right <= 0.0f && bottom <= 0.0f)
    {
        m_HasScaleMargins = false;
        return;
    }

    m_HasScaleMargins = true;
    m_ScaleMarginLeft = left;
    m_ScaleMarginTop = top;
    m_ScaleMarginRight = right;
    m_ScaleMarginBottom = bottom;
}

bool ImageElement::ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const
{
    return m_GeneralImage.ResolveImageCropRect(imageWidth, imageHeight, rect);
}

bool ImageElement::ComputeImageLayout(float imageWidth, float imageHeight, ImageLayout &layout)
{
    const int w = GetWidth();
    const int h = GetHeight();
    layout.contentX = m_X + m_PaddingLeft;
    layout.contentY = m_Y + m_PaddingTop;
    layout.contentW = w - m_PaddingLeft - m_PaddingRight;
    layout.contentH = h - m_PaddingTop - m_PaddingBottom;

    if (layout.contentW <= 0 || layout.contentH <= 0 || imageWidth <= 0.0f || imageHeight <= 0.0f)
    {
        return false;
    }

    layout.finalRect = D2D1::RectF(
        (float)layout.contentX,
        (float)layout.contentY,
        (float)(layout.contentX + layout.contentW),
        (float)(layout.contentY + layout.contentH));
    layout.srcRect = D2D1::RectF(0.0f, 0.0f, imageWidth, imageHeight);
    D2D1_RECT_F cropRect;
    if (ResolveImageCropRect(imageWidth, imageHeight, cropRect))
    {
        layout.srcRect = cropRect;
    }

    const float srcWidth = layout.srcRect.right - layout.srcRect.left;
    const float srcHeight = layout.srcRect.bottom - layout.srcRect.top;
    if (srcWidth <= 0.0f || srcHeight <= 0.0f)
    {
        return false;
    }

    if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE)
    {
        const float scaleX = (float)layout.contentW / srcWidth;
        const float scaleY = (float)layout.contentH / srcHeight;
        const float scale = (std::min)(scaleX, scaleY);
        const float finalW = srcWidth * scale;
        const float finalH = srcHeight * scale;

        layout.finalRect.left = layout.contentX + (layout.contentW - finalW) / 2.0f;
        layout.finalRect.top = layout.contentY + (layout.contentH - finalH) / 2.0f;
        layout.finalRect.right = layout.finalRect.left + finalW;
        layout.finalRect.bottom = layout.finalRect.top + finalH;
    }
    else if (m_PreserveAspectRatio == IMAGE_ASPECT_CROP)
    {
        const float scaleX = (float)layout.contentW / srcWidth;
        const float scaleY = (float)layout.contentH / srcHeight;
        const float scale = (std::max)(scaleX, scaleY);
        const float visibleW = layout.contentW / scale;
        const float visibleH = layout.contentH / scale;

        layout.srcRect.left += (srcWidth - visibleW) / 2.0f;
        layout.srcRect.top += (srcHeight - visibleH) / 2.0f;
        layout.srcRect.right = layout.srcRect.left + visibleW;
        layout.srcRect.bottom = layout.srcRect.top + visibleH;
    }

    return true;
}

bool ImageElement::MapPointToImagePixel(float targetX, float targetY, UINT imageWidth, UINT imageHeight, float &pixelX, float &pixelY)
{
    ImageLayout layout;
    if (!ComputeImageLayout((float)imageWidth, (float)imageHeight, layout))
    {
        return false;
    }

    if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE)
    {
        if (targetX < layout.finalRect.left || targetX >= layout.finalRect.right ||
            targetY < layout.finalRect.top || targetY >= layout.finalRect.bottom)
        {
            return false;
        }
    }
    else
    {
        if (targetX < layout.contentX || targetX >= layout.contentX + layout.contentW ||
            targetY < layout.contentY || targetY >= layout.contentY + layout.contentH)
        {
            return false;
        }
    }

    const float srcW = layout.srcRect.right - layout.srcRect.left;
    const float srcH = layout.srcRect.bottom - layout.srcRect.top;
    const float finalW = layout.finalRect.right - layout.finalRect.left;
    const float finalH = layout.finalRect.bottom - layout.finalRect.top;
    if (srcW <= 0.0f || srcH <= 0.0f || finalW <= 0.0f || finalH <= 0.0f)
    {
        return false;
    }

    pixelX = layout.srcRect.left + ((targetX - layout.finalRect.left) / finalW) * srcW;
    pixelY = layout.srcRect.top + ((targetY - layout.finalRect.top) / finalH) * srcH;

    m_GeneralImage.ApplyFlipToPixel(pixelX, pixelY, layout.srcRect);

    return pixelX >= 0.0f && pixelX < imageWidth && pixelY >= 0.0f && pixelY < imageHeight;
}

void ImageElement::UpdateImage(const std::wstring &path)
{
    m_GeneralImage.SetPath(path);
}

void ImageElement::Render(ID2D1DeviceContext *context)
{
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    // Draw background and bevel inside the transform
    RenderBackground(context);
    RenderBevel(context);

    m_GeneralImage.EnsureBitmap(context);
    ID2D1Bitmap *bitmap = m_GeneralImage.GetBitmap();
    if (!bitmap)
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    D2D1_SIZE_F imgSize = bitmap->GetSize();
    ImageLayout layout;
    if (!ComputeImageLayout(imgSize.width, imgSize.height, layout))
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    D2D1_MATRIX_3X2_F imageBaseTransform;
    bool hasImageFlipTransform = false;
    D2D1_MATRIX_3X2_F flipTransform;
    if (m_GeneralImage.BuildFlipTransform(layout.finalRect, flipTransform))
    {
        context->GetTransform(&imageBaseTransform);
        context->SetTransform(flipTransform * imageBaseTransform);
        hasImageFlipTransform = true;
    }

    float opacity = m_GeneralImage.GetImageAlpha() / 255.0f;
    D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

    Microsoft::WRL::ComPtr<ID2D1Image> pFinalImage;
    if (!m_GeneralImage.BuildProcessedImage(context, pFinalImage))
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    if (m_Tile)
    {
        Microsoft::WRL::ComPtr<ID2D1ImageBrush> pBrush;
        D2D1_IMAGE_BRUSH_PROPERTIES brushProps = D2D1::ImageBrushProperties(
            layout.srcRect,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_INTERPOLATION_MODE_LINEAR);
        context->CreateImageBrush(pFinalImage.Get(), brushProps, pBrush.GetAddressOf());
        if (pBrush)
        {
            pBrush->SetTransform(D2D1::Matrix3x2F::Translation(layout.finalRect.left - layout.srcRect.left, layout.finalRect.top - layout.srcRect.top));
            pBrush->SetOpacity(opacity);
            context->FillRectangle(layout.finalRect, pBrush.Get());
        }
    }
    else
    {
        const bool useScaleMargins =
            m_HasScaleMargins &&
            !m_Tile &&
            m_PreserveAspectRatio == IMAGE_ASPECT_STRETCH &&
            pFinalImage.Get() == (ID2D1Image *)bitmap;

        if (useScaleMargins)
        {
            const float srcW = layout.srcRect.right - layout.srcRect.left;
            const float srcH = layout.srcRect.bottom - layout.srcRect.top;
            const float dstW = layout.finalRect.right - layout.finalRect.left;
            const float dstH = layout.finalRect.bottom - layout.finalRect.top;

            float sl = (std::min)(m_ScaleMarginLeft, srcW);
            float sr = (std::min)(m_ScaleMarginRight, srcW - sl);
            float st = (std::min)(m_ScaleMarginTop, srcH);
            float sb = (std::min)(m_ScaleMarginBottom, srcH - st);

            if (sl + sr > srcW)
            {
                const float ratio = (srcW <= 0.0f) ? 0.0f : (srcW / (sl + sr));
                sl *= ratio;
                sr *= ratio;
            }
            if (st + sb > srcH)
            {
                const float ratio = (srcH <= 0.0f) ? 0.0f : (srcH / (st + sb));
                st *= ratio;
                sb *= ratio;
            }

            float dl = sl;
            float dr = sr;
            float dt = st;
            float db = sb;
            if (dl + dr > dstW)
            {
                const float ratio = (dstW <= 0.0f) ? 0.0f : (dstW / (dl + dr));
                dl *= ratio;
                dr *= ratio;
            }
            if (dt + db > dstH)
            {
                const float ratio = (dstH <= 0.0f) ? 0.0f : (dstH / (dt + db));
                dt *= ratio;
                db *= ratio;
            }

            const float sx0 = layout.srcRect.left;
            const float sx1 = sx0 + sl;
            const float sx2 = layout.srcRect.right - sr;
            const float sx3 = layout.srcRect.right;
            const float sy0 = layout.srcRect.top;
            const float sy1 = sy0 + st;
            const float sy2 = layout.srcRect.bottom - sb;
            const float sy3 = layout.srcRect.bottom;

            const float dx0 = layout.finalRect.left;
            const float dx1 = dx0 + dl;
            const float dx2 = layout.finalRect.right - dr;
            const float dx3 = layout.finalRect.right;
            const float dy0 = layout.finalRect.top;
            const float dy1 = dy0 + dt;
            const float dy2 = layout.finalRect.bottom - db;
            const float dy3 = layout.finalRect.bottom;

            auto drawPatch = [&](float dL, float dT, float dR, float dB, float sL, float sT, float sR, float sB)
            {
                if (dR <= dL || dB <= dT || sR <= sL || sB <= sT)
                    return;
                const D2D1_RECT_F dst = D2D1::RectF(dL, dT, dR, dB);
                const D2D1_RECT_F src = D2D1::RectF(sL, sT, sR, sB);
                context->DrawBitmap(bitmap, &dst, opacity, interp, &src);
            };

            // Top row
            drawPatch(dx0, dy0, dx1, dy1, sx0, sy0, sx1, sy1);
            drawPatch(dx1, dy0, dx2, dy1, sx1, sy0, sx2, sy1);
            drawPatch(dx2, dy0, dx3, dy1, sx2, sy0, sx3, sy1);
            // Middle row
            drawPatch(dx0, dy1, dx1, dy2, sx0, sy1, sx1, sy2);
            drawPatch(dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2);
            drawPatch(dx2, dy1, dx3, dy2, sx2, sy1, sx3, sy2);
            // Bottom row
            drawPatch(dx0, dy2, dx1, dy3, sx0, sy2, sx1, sy3);
            drawPatch(dx1, dy2, dx2, dy3, sx1, sy2, sx2, sy3);
            drawPatch(dx2, dy2, dx3, dy3, sx2, sy2, sx3, sy3);
        }
        else if (pFinalImage.Get() == (ID2D1Image *)bitmap)
        {
            context->DrawBitmap(bitmap, &layout.finalRect, opacity, interp, &layout.srcRect);
        }
        else
        {
            // For effects, we use DrawImage. Offset by finalRect top-left and scale.
            // DrawImage doesn't take a destination rect directly, it draws at offset.
            // We need to apply scaling to the transform if finalRect size != srcRect size.
            float scaleX = (layout.finalRect.right - layout.finalRect.left) / (layout.srcRect.right - layout.srcRect.left);
            float scaleY = (layout.finalRect.bottom - layout.finalRect.top) / (layout.srcRect.bottom - layout.srcRect.top);

            D2D1_MATRIX_3X2_F effectTransform;
            context->GetTransform(&effectTransform);

            // Adjust transform for scale and source rect offset
            context->SetTransform(
                D2D1::Matrix3x2F::Translation(-layout.srcRect.left, -layout.srcRect.top) *
                D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
                D2D1::Matrix3x2F::Translation(layout.finalRect.left, layout.finalRect.top) *
                effectTransform);

            context->DrawImage(pFinalImage.Get(), nullptr, nullptr, m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            context->SetTransform(effectTransform);
        }
    }

    if (hasImageFlipTransform)
    {
        context->SetTransform(imageBaseTransform);
    }

    RestoreRenderTransform(context, originalTransform);
}

int ImageElement::GetAutoWidth()
{
    return m_GeneralImage.GetAutoWidth() + m_PaddingLeft + m_PaddingRight;
}

int ImageElement::GetAutoHeight()
{
    return m_GeneralImage.GetAutoHeight() + m_PaddingTop + m_PaddingBottom;
}

bool ImageElement::HitTest(int x, int y)
{
    // Bounding box and transform check
    if (!Element::HitTest(x, y))
        return false;

    // Map the point to image local space
    float targetX = (float)x;
    float targetY = (float)y;

    if (m_HasTransformMatrix || m_Rotate != 0.0f)
    {
        GfxRect bounds = GetBounds();
        float centerX = bounds.X + bounds.Width / 2.0f;
        float centerY = bounds.Y + bounds.Height / 2.0f;

        D2D1::Matrix3x2F matrix;
        if (m_HasTransformMatrix)
        {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]);
        }
        else
        {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (matrix.Invert())
        {
            D2D1_POINT_2F transformed = matrix.TransformPoint(D2D1::Point2F((float)x, (float)y));
            targetX = transformed.x;
            targetY = transformed.y;
        }
    }

    if (m_HasSolidColor && m_SolidAlpha > 0)
        return true;
    IWICBitmap *wic = m_GeneralImage.GetWICBitmap();
    if (!wic)
        return false;

    UINT imgW, imgH;
    wic->GetSize(&imgW, &imgH);
    if (imgW == 0 || imgH == 0)
        return false;

    float targetPixelX = 0.0f;
    float targetPixelY = 0.0f;
    if (!MapPointToImagePixel(targetX, targetY, imgW, imgH, targetPixelX, targetPixelY))
        return false;

    // Get pixel alpha
    WICRect rect = {(INT)targetPixelX, (INT)targetPixelY, 1, 1};
    BYTE pixel[4]; // BGRA
    HRESULT hr = wic->CopyPixels(&rect, 4, 4, pixel);
    if (FAILED(hr))
        return false;

    return pixel[3] > 0; // Alpha channel
}
