/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __COSMOS_IMAGE_ELEMENT_H__
#define __COSMOS_IMAGE_ELEMENT_H__

#include "Element.h"

class ImageElement : public Element
{
public:
    ImageElement(const std::wstring& id, int x, int y, int w, int h,
                 const std::wstring& path);
                 
    virtual ~ImageElement();

    virtual void Render(Gdiplus::Graphics& graphics) override;
    
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    
    // Returns true if image loaded successfully
    bool IsLoaded() const { return m_Image != nullptr && m_Image->GetLastStatus() == Gdiplus::Ok; }
    
    void UpdateImage(const std::wstring& path);

    virtual bool HitTest(int x, int y) override;

    void SetPreserveAspectRatio(int mode) { m_PreserveAspectRatio = mode; }
    void SetImageTint(COLORREF color, BYTE alpha) { 
        m_ImageTint = color; 
        m_ImageTintAlpha = alpha; 
        m_HasImageTint = (alpha > 0); 
    }
    
    void SetImageAlpha(BYTE alpha) { m_ImageAlpha = alpha; }
    void SetGrayscale(bool enable) { m_Grayscale = enable; }
    void SetColorMatrix(const float* matrix) {
        if (matrix) {
            memcpy(m_ColorMatrix, matrix, sizeof(float) * 25);
            m_HasColorMatrix = true;
        } else {
            m_HasColorMatrix = false;
        }
    }
    
    void SetTile(bool tile) { m_Tile = tile; }
    void SetImageRotate(float degrees) { m_ImageRotate = degrees; }

private:
    std::wstring m_ImagePath;
    Gdiplus::Bitmap* m_Image;
    
    // 0 = Stretch, 1 = Preserve, 2 = Crop
    int m_PreserveAspectRatio = 0;
    
    bool m_HasImageTint = false;
    COLORREF m_ImageTint = 0;
    BYTE m_ImageTintAlpha = 255;
    
    // New properties
    BYTE m_ImageAlpha = 255;
    bool m_Grayscale = false;
    bool m_HasColorMatrix = false;
    float m_ColorMatrix[5][5];

    // Newest properties
    bool m_Tile = false;
    float m_ImageRotate = 0.0f;

    void LoadImage();
};

#endif
