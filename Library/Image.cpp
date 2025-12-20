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
             const std::wstring& path, ScaleMode mode)
    : Element(ELEMENT_IMAGE, id, x, y, w, h),
      m_ImagePath(path), m_ScaleMode(mode), m_Image(nullptr)
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
    
    m_Image = Image::FromFile(m_ImagePath.c_str());
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
    if (!m_Image) return;
    
    int imgWidth = m_Image->GetWidth();
    int imgHeight = m_Image->GetHeight();
    
    RectF destRect((REAL)m_X, (REAL)m_Y, (REAL)m_Width, (REAL)m_Height);
    
    switch (m_ScaleMode)
    {
    case SCALE_FILL:
    case SCALE_STRETCH:
        // Stretch to fill the entire bounds
        graphics.DrawImage(m_Image, destRect);
        break;
        
    case SCALE_CONTAIN:
        {
            // Fit inside bounds, maintain aspect ratio
            float scaleX = (float)m_Width / imgWidth;
            float scaleY = (float)m_Height / imgHeight;
            float scale = (std::min)(scaleX, scaleY);
            
            int scaledW = (int)(imgWidth * scale);
            int scaledH = (int)(imgHeight * scale);
            int offsetX = (m_Width - scaledW) / 2;
            int offsetY = (m_Height - scaledH) / 2;
            
            RectF fitRect((REAL)(m_X + offsetX), (REAL)(m_Y + offsetY),
                         (REAL)scaledW, (REAL)scaledH);
            graphics.DrawImage(m_Image, fitRect);
        }
        break;
        
    case SCALE_COVER:
        {
            // Cover bounds, maintain aspect ratio (may crop)
            float scaleX = (float)m_Width / imgWidth;
            float scaleY = (float)m_Height / imgHeight;
            float scale = (std::max)(scaleX, scaleY);
            
            int scaledW = (int)(imgWidth * scale);
            int scaledH = (int)(imgHeight * scale);
            int offsetX = (m_Width - scaledW) / 2;
            int offsetY = (m_Height - scaledH) / 2;
            
            RectF coverRect((REAL)(m_X + offsetX), (REAL)(m_Y + offsetY),
                           (REAL)scaledW, (REAL)scaledH);
            graphics.DrawImage(m_Image, coverRect);
        }
        break;
    }
}
