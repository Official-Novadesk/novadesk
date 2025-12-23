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
    // Draw background first
    RenderBackground(graphics);

    if (!m_Image) return;
    
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0) return;
    
    REAL finalX = (REAL)m_X;
    REAL finalY = (REAL)m_Y;
    REAL finalW = (REAL)w;
    REAL finalH = (REAL)h;
    
    if (m_PreserveAspectRatio == 1) // Preserve (Fit)
    {
        REAL imgW = (REAL)m_Image->GetWidth();
        REAL imgH = (REAL)m_Image->GetHeight();
        REAL scaleX = (REAL)w / imgW;
        REAL scaleY = (REAL)h / imgH;
        REAL scale = min(scaleX, scaleY); // Fit inside
        
        finalW = imgW * scale;
        finalH = imgH * scale;
        finalX = m_X + (w - finalW) / 2.0f;
        finalY = m_Y + (h - finalH) / 2.0f;
    }
    else if (m_PreserveAspectRatio == 2) // Crop (Fill)
    {
        // For crop, we clip content outside the box
        graphics.SetClip(RectF((REAL)m_X, (REAL)m_Y, (REAL)w, (REAL)h));
        
        REAL imgW = (REAL)m_Image->GetWidth();
        REAL imgH = (REAL)m_Image->GetHeight();
        REAL scaleX = (REAL)w / imgW;
        REAL scaleY = (REAL)h / imgH;
        REAL scale = max(scaleX, scaleY); // Fill the box
        
        finalW = imgW * scale;
        finalH = imgH * scale;
        finalX = m_X + (w - finalW) / 2.0f;
        finalY = m_Y + (h - finalH) / 2.0f;
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
    
    // Rotation
    if (m_ImageRotate != 0.0f)
    {
        graphics.TranslateTransform(finalX + finalW / 2.0f, finalY + finalH / 2.0f);
        graphics.RotateTransform(m_ImageRotate);
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
    
    // Restore transform if rotated
    if (m_ImageRotate != 0.0f)
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
    return (int)m_Image->GetWidth();
}

int ImageElement::GetAutoHeight()
{
    if (!m_Image) return 0;
    return (int)m_Image->GetHeight();
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
