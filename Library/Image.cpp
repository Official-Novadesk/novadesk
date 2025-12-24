/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Image.h"
#include "Logging.h"
#include <algorithm>

using namespace Gdiplus;

ImageElement::ImageElement(const std::wstring& id, int x, int y, int w, int h,
             const std::wstring& path)
    : Element(ELEMENT_IMAGE, id, x, y, w, h),
      m_ImagePath(path), m_Image(nullptr)
{
    LoadImage();
}

ImageElement::~ImageElement()
{
    if (m_Image)
    {
        delete m_Image;
        m_Image = nullptr;
    }
}

void ImageElement::LoadImage()
{
    if (m_Image)
    {
        delete m_Image;
        m_Image = nullptr;
    }
    
    m_Image = Bitmap::FromFile(m_ImagePath.c_str());
    if (m_Image && m_Image->GetLastStatus() != Ok)
    {
        delete m_Image;
        m_Image = nullptr;
        Logging::Log(LogLevel::Error, L"Failed to load image: %s", m_ImagePath.c_str());
    }
}

void ImageElement::UpdateImage(const std::wstring& path)
{
    m_ImagePath = path;
    LoadImage();
}

void ImageElement::Render(Graphics& graphics)
{
    // Draw bevel first (if enabled)
    RenderBevel(graphics);
    
    // Draw background second
    RenderBackground(graphics);

    if (!m_Image) return;
    
    // Set antialiasing (interpolation mode for images)
    if (m_AntiAlias) {
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    } else {
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    }
    
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;
    
    // Apply padding
    int contentX = m_X + m_PaddingLeft;
    int contentY = m_Y + m_PaddingTop;
    int contentW = w - m_PaddingLeft - m_PaddingRight;
    int contentH = h - m_PaddingTop - m_PaddingBottom;
    
    Logging::Log(LogLevel::Debug, L"Image Render: W=%d H=%d Pad=[%d,%d,%d,%d] Content=[%d,%d,%d,%d] Preserve=%d", 
        w, h, m_PaddingLeft, m_PaddingTop, m_PaddingRight, m_PaddingBottom,
        contentX, contentY, contentW, contentH, m_PreserveAspectRatio);
    
    // Ensure positive dimensions
    if (contentW <= 0 || contentH <= 0) {
        Logging::Log(LogLevel::Debug, L"Image Render skipped due to zero/negative content dimensions");
        return;
    }
    
    REAL finalX = (REAL)contentX;
    REAL finalY = (REAL)contentY;
    REAL finalW = (REAL)contentW;
    REAL finalH = (REAL)contentH;
    
    if (m_PreserveAspectRatio == 1) // Preserve (Fit)
    {
        REAL imgW = (REAL)m_Image->GetWidth();
        REAL imgH = (REAL)m_Image->GetHeight();
        REAL scaleX = (REAL)contentW / imgW;
        REAL scaleY = (REAL)contentH / imgH;
        REAL scale = min(scaleX, scaleY); // Fit inside
        
        finalW = imgW * scale;
        finalH = imgH * scale;
        finalX = contentX + (contentW - finalW) / 2.0f;
        finalY = contentY + (contentH - finalH) / 2.0f;
    }
    else if (m_PreserveAspectRatio == 2) // Crop (Fill)
    {
        // For crop, we clip content outside the box
        graphics.SetClip(RectF((REAL)contentX, (REAL)contentY, (REAL)contentW, (REAL)contentH));
        
        REAL imgW = (REAL)m_Image->GetWidth();
        REAL imgH = (REAL)m_Image->GetHeight();
        REAL scaleX = (REAL)contentW / imgW;
        REAL scaleY = (REAL)contentH / imgH;
        REAL scale = max(scaleX, scaleY); // Fill the box
        
        finalW = imgW * scale;
        finalH = imgH * scale;
        finalX = contentX + (contentW - finalW) / 2.0f;
        finalY = contentY + (contentH - finalH) / 2.0f;
    }
    
    ImageAttributes* attr = nullptr;
    ImageAttributes imageAttr;
    
    // Check if we need ColorMatrix
    if (m_HasColorMatrix || m_Grayscale || m_HasImageTint || m_ImageAlpha < 255)
    {
        ColorMatrix colorMatrix;
        bool matrixSet = false;

        if (m_HasColorMatrix)
        {
            // Use custom matrix
            memcpy(&colorMatrix, m_ColorMatrix, sizeof(ColorMatrix));
            matrixSet = true;
        }
        else if (m_Grayscale)
        {
            // Grayscale matrix
            ColorMatrix grayMatrix = {
                0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
                0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
                0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
                0.0f,   0.0f,   0.0f,   1.0f, 0.0f,
                0.0f,   0.0f,   0.0f,   0.0f, 1.0f
            };
            colorMatrix = grayMatrix;
            matrixSet = true;
        }
        else if (m_HasImageTint)
        {
            // Tint matrix
            ColorMatrix tintMatrix = {
                (REAL)GetRValue(m_ImageTint)/255.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, (REAL)GetGValue(m_ImageTint)/255.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, (REAL)GetBValue(m_ImageTint)/255.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, (REAL)m_ImageTintAlpha/255.0f, 0.0f, 
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f
            };
            colorMatrix = tintMatrix;
            matrixSet = true;
        }
        
        // If we haven't set a matrix but need alpha, start with identity
        if (!matrixSet)
        {
             ColorMatrix identityMatrix = {
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f
            };
            colorMatrix = identityMatrix;
        }

        // Apply Image Alpha
        if (m_ImageAlpha < 255)
        {
            colorMatrix.m[3][3] *= (m_ImageAlpha / 255.0f);
        }

        imageAttr.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        attr = &imageAttr;
    }
    
    // Apply transformation (matrix takes priority over simple rotation)
    if (m_HasTransformMatrix)
    {
        // Apply transformation matrix
        Gdiplus::Matrix matrix(
            m_TransformMatrix[0], m_TransformMatrix[1],
            m_TransformMatrix[2], m_TransformMatrix[3],
            m_TransformMatrix[4], m_TransformMatrix[5]
        );
        
        // Translate to center, apply matrix, translate back
        REAL centerX = finalX + finalW / 2.0f;
        REAL centerY = finalY + finalH / 2.0f;
        graphics.TranslateTransform(centerX, centerY);
        graphics.MultiplyTransform(&matrix);
        graphics.TranslateTransform(-centerX, -centerY);
    }
    else if (m_Rotate != 0.0f)
    {
        // Simple rotation
        graphics.TranslateTransform(finalX + finalW / 2.0f, finalY + finalH / 2.0f);
        graphics.RotateTransform(m_Rotate);
        graphics.TranslateTransform(-(finalX + finalW / 2.0f), -(finalY + finalH / 2.0f));
    }

    if (m_Tile)
    {
        RectF srcRect(0, 0, (REAL)m_Image->GetWidth(), (REAL)m_Image->GetHeight());
        TextureBrush tiledBrush(m_Image, srcRect, attr);
        tiledBrush.SetWrapMode(WrapModeTile);
        tiledBrush.TranslateTransform(finalX, finalY);
        graphics.FillRectangle(&tiledBrush, finalX, finalY, finalW, finalH);
    }
    else
    {
        graphics.DrawImage(m_Image, RectF(finalX, finalY, finalW, finalH), 
                           0, 0, (REAL)m_Image->GetWidth(), (REAL)m_Image->GetHeight(), 
                           UnitPixel, attr);
    }
    
    // Restore transform if it was applied
    if (m_HasTransformMatrix || m_Rotate != 0.0f)
    {
        graphics.ResetTransform();
    }
    
    // Restore clip if it was set for Crop mode
    if (m_PreserveAspectRatio == 2)
    {
        graphics.ResetClip();
    }
}

int ImageElement::GetAutoWidth()
{
    if (!m_Image) return 0;
    int w = (int)m_Image->GetWidth() + m_PaddingLeft + m_PaddingRight;
    Logging::Log(LogLevel::Debug, L"Image GetAutoWidth: ImgW=%d PadL=%d PadR=%d Total=%d", 
        (int)m_Image->GetWidth(), m_PaddingLeft, m_PaddingRight, w);
    return w;
}

int ImageElement::GetAutoHeight()
{
    if (!m_Image) return 0;
    int h = (int)m_Image->GetHeight() + m_PaddingTop + m_PaddingBottom;
    Logging::Log(LogLevel::Debug, L"Image GetAutoHeight: ImgH=%d PadT=%d PadB=%d Total=%d", 
        (int)m_Image->GetHeight(), m_PaddingTop, m_PaddingBottom, h);
    return h;
}

bool ImageElement::HitTest(int x, int y)
{
    // Bounding box check first
    if (!Element::HitTest(x, y)) return false;
    
    // If we have a solid background, the entire box is hit
    if (m_HasSolidColor && m_SolidAlpha > 0) return true;

    if (!m_Image) return false;

    // Map widget coordinates to image coordinates
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return false;

    int imgX = (int)((x - m_X) * ((double)m_Image->GetWidth() / w));
    int imgY = (int)((y - m_Y) * ((double)m_Image->GetHeight() / h));
    
    Color pixelColor;
    if (m_Image->GetPixel(imgX, imgY, &pixelColor) == Ok)
    {
        // Hit if pixel is not fully transparent
        return pixelColor.GetAlpha() > 0;
    }
    
    return false;
}
