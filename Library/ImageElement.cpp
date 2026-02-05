/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ImageElement.h"
#include "Direct2DHelper.h"
#include <d2d1effects.h>
#include "Logging.h"

ImageElement::ImageElement(const std::wstring& id, int x, int y, int w, int h,
             const std::wstring& path)
    : Element(ELEMENT_IMAGE, id, x, y, w, h),
      m_ImagePath(path)
{
}

ImageElement::~ImageElement()
{
}

void ImageElement::EnsureBitmap(ID2D1DeviceContext* context)
{
    if (m_pLastTarget != (ID2D1RenderTarget*)context)
    {
        m_D2DBitmap.Reset();
        // WIC bitmap is CPU side, doesn't strictly need reset on target change, 
        // but we'll re-verify it if we are reloading anyway.
        m_pLastTarget = (ID2D1RenderTarget*)context;
    }

    if (!m_D2DBitmap)
    {
        Direct2D::LoadBitmapFromFile(context, m_ImagePath, m_D2DBitmap.ReleaseAndGetAddressOf(), m_pWICBitmap.ReleaseAndGetAddressOf());
    }
}

void ImageElement::UpdateImage(const std::wstring& path)
{
    m_ImagePath = path;
    m_D2DBitmap.Reset();
    m_pWICBitmap.Reset();
    Direct2D::LoadWICBitmapFromFile(m_ImagePath, m_pWICBitmap.ReleaseAndGetAddressOf());
}

void ImageElement::Render(ID2D1DeviceContext* context)
{
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    // Draw background and bevel inside the transform
    RenderBackground(context);
    RenderBevel(context);

    EnsureBitmap(context);
    if (!m_D2DBitmap)
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }
    
    D2D1_SIZE_F imgSize = m_D2DBitmap->GetSize();
    
    int contentX = m_X + m_PaddingLeft;
    int contentY = m_Y + m_PaddingTop;
    int contentW = w - m_PaddingLeft - m_PaddingRight;
    int contentH = h - m_PaddingTop - m_PaddingBottom;
    
    if (contentW <= 0 || contentH <= 0)
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }
    
    D2D1_RECT_F finalRect = D2D1::RectF((float)contentX, (float)contentY, (float)(contentX + contentW), (float)(contentY + contentH));
    D2D1_RECT_F srcRect = D2D1::RectF(0, 0, imgSize.width, imgSize.height);
    
    if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE) // Fit
    {
        float scaleX = (float)contentW / imgSize.width;
        float scaleY = (float)contentH / imgSize.height;
        float scale = (std::min)(scaleX, scaleY);
        
        float finalW = imgSize.width * scale;
        float finalH = imgSize.height * scale;
        finalRect.left = contentX + (contentW - finalW) / 2.0f;
        finalRect.top = contentY + (contentH - finalH) / 2.0f;
        finalRect.right = finalRect.left + finalW;
        finalRect.bottom = finalRect.top + finalH;
    }
    else if (m_PreserveAspectRatio == IMAGE_ASPECT_CROP) // Fill
    {
        float scaleX = (float)contentW / imgSize.width;
        float scaleY = (float)contentH / imgSize.height;
        float scale = (std::max)(scaleX, scaleY);
        
        float finalW = imgSize.width * scale;
        float finalH = imgSize.height * scale;
        
        float srcW = contentW / scale;
        float srcH = contentH / scale;
        srcRect.left = (imgSize.width - srcW) / 2.0f;
        srcRect.top = (imgSize.height - srcH) / 2.0f;
        srcRect.right = srcRect.left + srcW;
        srcRect.bottom = srcRect.top + srcH;
    }
    
    // Rotation already applied above

    float opacity = m_ImageAlpha / 255.0f;
    D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

    Microsoft::WRL::ComPtr<ID2D1Image> pFinalImage = m_D2DBitmap.Get();

    if (m_Grayscale || m_HasImageTint || m_HasColorMatrix || m_ImageAlpha < 255)
    {
        Microsoft::WRL::ComPtr<ID2D1Image> pCurrentInput = (ID2D1Image*)m_D2DBitmap.Get();
        
        // Stage 1: Greyscale
        if (m_Grayscale)
        {
            Microsoft::WRL::ComPtr<ID2D1Effect> pGreyscaleEffect;
            if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, pGreyscaleEffect.GetAddressOf())))
            {
                D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
                    0.299f, 0.299f, 0.299f, 0.0f,
                    0.587f, 0.587f, 0.587f, 0.0f,
                    0.114f, 0.114f, 0.114f, 0.0f,
                    0.0f,   0.0f,   0.0f,   1.0f,
                    0.0f,   0.0f,   0.0f,   0.0f
                );
                pGreyscaleEffect->SetInput(0, pCurrentInput.Get());
                pGreyscaleEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
                pGreyscaleEffect->GetOutput(&pCurrentInput);
            }
        }

        // Stage 2: ColorMatrix, Tint, or ImageAlpha
        // We always run this stage if any of these are set, or if we need to apply ImageAlpha
        if (m_HasColorMatrix || m_HasImageTint || m_ImageAlpha < 255)
        {
            Microsoft::WRL::ComPtr<ID2D1Effect> pColorMatrixEffect;
            if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, pColorMatrixEffect.GetAddressOf())))
            {
                D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 0.0f, 0.0f
                );

                if (m_HasColorMatrix)
                {
                    memcpy(&matrix, m_ColorMatrix, sizeof(float) * 20);
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
                    
                    // Multiply by ImageAlpha for Tint or Identity
                    matrix.m[3][3] *= (m_ImageAlpha / 255.0f);
                }

                pColorMatrixEffect->SetInput(0, pCurrentInput.Get());
                pColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
                pColorMatrixEffect->GetOutput(&pCurrentInput);
            }
        }

        pFinalImage = pCurrentInput;
    }

    if (m_Tile)
    {
        Microsoft::WRL::ComPtr<ID2D1ImageBrush> pBrush;
        D2D1_IMAGE_BRUSH_PROPERTIES brushProps = D2D1::ImageBrushProperties(
            srcRect,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_INTERPOLATION_MODE_LINEAR
        );
        context->CreateImageBrush(pFinalImage.Get(), brushProps, pBrush.GetAddressOf());
        if (pBrush)
        {
            pBrush->SetTransform(D2D1::Matrix3x2F::Translation(finalRect.left - srcRect.left, finalRect.top - srcRect.top));
            pBrush->SetOpacity(opacity);
            context->FillRectangle(finalRect, pBrush.Get());
        }
    }
    else
    {
        if (pFinalImage.Get() == (ID2D1Image*)m_D2DBitmap.Get())
        {
            context->DrawBitmap(m_D2DBitmap.Get(), &finalRect, opacity, interp, &srcRect);
        }
        else
        {
            // For effects, we use DrawImage. Offset by finalRect top-left and scale.
            D2D1_POINT_2F targetOffset = D2D1::Point2F(finalRect.left, finalRect.top);
            
            // DrawImage doesn't take a destination rect directly, it draws at offset.
            // We need to apply scaling to the transform if finalRect size != srcRect size.
            float scaleX = (finalRect.right - finalRect.left) / (srcRect.right - srcRect.left);
            float scaleY = (finalRect.bottom - finalRect.top) / (srcRect.bottom - srcRect.top);
            
            D2D1_MATRIX_3X2_F effectTransform;
            context->GetTransform(&effectTransform);
            
            // Adjust transform for scale and source rect offset
            context->SetTransform(
                D2D1::Matrix3x2F::Translation(-srcRect.left, -srcRect.top) *
                D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
                D2D1::Matrix3x2F::Translation(finalRect.left, finalRect.top) *
                effectTransform
            );

            context->DrawImage(pFinalImage.Get(), nullptr, nullptr, m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            context->SetTransform(effectTransform);
        }
    }
    
    RestoreRenderTransform(context, originalTransform);
}

int ImageElement::GetAutoWidth()
{
    if (m_pWICBitmap) {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        return (int)w + m_PaddingLeft + m_PaddingRight;
    }
    if (!m_D2DBitmap) return 0;
    return (int)m_D2DBitmap->GetSize().width + m_PaddingLeft + m_PaddingRight;
}

int ImageElement::GetAutoHeight()
{
    if (m_pWICBitmap) {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        return (int)h + m_PaddingTop + m_PaddingBottom;
    }
    if (!m_D2DBitmap) return 0;
    return (int)m_D2DBitmap->GetSize().height + m_PaddingTop + m_PaddingBottom;
}

bool ImageElement::HitTest(int x, int y)
{
    // Bounding box and transform check
    if (!Element::HitTest(x, y)) return false;
    
    // Map the point to image local space
    float targetX = (float)x;
    float targetY = (float)y;

    if (m_HasTransformMatrix || m_Rotate != 0.0f) {
        GfxRect bounds = GetBounds();
        float centerX = bounds.X + bounds.Width / 2.0f;
        float centerY = bounds.Y + bounds.Height / 2.0f;
        
        D2D1::Matrix3x2F matrix;
        if (m_HasTransformMatrix) {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]
            );
        } else {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (matrix.Invert()) {
            D2D1_POINT_2F transformed = matrix.TransformPoint(D2D1::Point2F((float)x, (float)y));
            targetX = transformed.x;
            targetY = transformed.y;
        }
    }

    if (m_HasSolidColor && m_SolidAlpha > 0) return true;
    if (!m_pWICBitmap) return false;

    // Map the transformed (local-space) coordinates to image pixels
    int contentX = m_X + m_PaddingLeft;
    int contentY = m_Y + m_PaddingTop;
    int contentW = GetWidth() - m_PaddingLeft - m_PaddingRight;
    int contentH = GetHeight() - m_PaddingTop - m_PaddingBottom;

    if (contentW <= 0 || contentH <= 0) return false;

    float relX = targetX - contentX;
    float relY = targetY - contentY;

    UINT imgW, imgH;
    m_pWICBitmap->GetSize(&imgW, &imgH);
    if (imgW == 0 || imgH == 0) return false;

    float targetPixelX = 0, targetPixelY = 0;

    if (m_PreserveAspectRatio == IMAGE_ASPECT_STRETCH)
    {
        targetPixelX = (relX / (float)contentW) * imgW;
        targetPixelY = (relY / (float)contentH) * imgH;
    }
    else if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE) // Fit
    {
        float scaleX = (float)contentW / imgW;
        float scaleY = (float)contentH / imgH;
        float scale = (std::min)(scaleX, scaleY);

        float finalW = imgW * scale;
        float finalH = imgH * scale;
        float offsetX = (contentW - finalW) / 2.0f;
        float offsetY = (contentH - finalH) / 2.0f;

        targetPixelX = ((relX - offsetX) / finalW) * imgW;
        targetPixelY = ((relY - offsetY) / finalH) * imgH;
    }
    else if (m_PreserveAspectRatio == IMAGE_ASPECT_CROP) // Fill
    {
        float scaleX = (float)contentW / imgW;
        float scaleY = (float)contentH / imgH;
        float scale = (std::max)(scaleX, scaleY);

        float srcW = contentW / scale;
        float srcH = contentH / scale;
        float srcX = (imgW - srcW) / 2.0f;
        float srcY = (imgH - srcH) / 2.0f;

        targetPixelX = srcX + (relX / (float)contentW) * srcW;
        targetPixelY = srcY + (relY / (float)contentH) * srcH;
    }

    if (targetPixelX < 0 || targetPixelX >= imgW || targetPixelY < 0 || targetPixelY >= imgH)
        return false;

    // Get pixel alpha
    WICRect rect = { (INT)targetPixelX, (INT)targetPixelY, 1, 1 };
    BYTE pixel[4]; // BGRA
    HRESULT hr = m_pWICBitmap->CopyPixels(&rect, 4, 4, pixel);
    if (FAILED(hr)) return false;

    return pixel[3] > 0; // Alpha channel
}
