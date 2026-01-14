/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __COSMOS_IMAGE_ELEMENT_H__
#define __COSMOS_IMAGE_ELEMENT_H__

#include "Element.h"

enum ImageAspectRatio
{
    IMAGE_ASPECT_STRETCH,   // 0
    IMAGE_ASPECT_PRESERVE,  // 1
    IMAGE_ASPECT_CROP       // 2
};

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

    void SetPreserveAspectRatio(ImageAspectRatio mode) { m_PreserveAspectRatio = mode; }
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
    void SetTransformMatrix(const float* matrix) {
        if (matrix) {
            memcpy(m_TransformMatrix, matrix, sizeof(float) * 6);
            m_HasTransformMatrix = true;
        } else {
            m_HasTransformMatrix = false;
        }
    }

    const std::wstring& GetImagePath() const { return m_ImagePath; }
    ImageAspectRatio GetPreserveAspectRatio() const { return m_PreserveAspectRatio; }
    bool HasImageTint() const { return m_HasImageTint; }
    COLORREF GetImageTint() const { return m_ImageTint; }
    BYTE GetImageTintAlpha() const { return m_ImageTintAlpha; }
    BYTE GetImageAlpha() const { return m_ImageAlpha; }
    bool IsGrayscale() const { return m_Grayscale; }
    bool IsTile() const { return m_Tile; }
    bool HasTransformMatrix() const { return m_HasTransformMatrix; }
    const float* GetTransformMatrix() const { return m_TransformMatrix; }
    bool HasColorMatrix() const { return m_HasColorMatrix; }
    const float* GetColorMatrix() const { return (const float*)m_ColorMatrix; }

private:
    std::wstring m_ImagePath;
    Gdiplus::Bitmap* m_Image;
    
    // 0 = Stretch, 1 = Preserve, 2 = Crop
    ImageAspectRatio m_PreserveAspectRatio = IMAGE_ASPECT_STRETCH;
    
    bool m_HasImageTint = false;
    COLORREF m_ImageTint = 0;
    BYTE m_ImageTintAlpha = 255;
    
    // New properties
    BYTE m_ImageAlpha = 255;
    bool m_Grayscale = false;
    bool m_HasColorMatrix = false;
    float m_ColorMatrix[5][5];

    // Newest properties
    // Newest properties
    bool m_Tile = false;
    
    // Transformation matrix (6 elements: m11, m12, m21, m22, dx, dy)
    bool m_HasTransformMatrix = false;
    float m_TransformMatrix[6];

    void LoadImage();
};

#endif
