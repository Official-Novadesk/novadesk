/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_IMAGE_ELEMENT_H__
#define __NOVADESK_IMAGE_ELEMENT_H__

#include "Element.h"

enum ScaleMode
{
    SCALE_FILL,      // Stretch to fill bounds (default)
    SCALE_CONTAIN,   // Fit inside bounds, maintain aspect ratio
    SCALE_COVER,     // Cover bounds, maintain aspect ratio (may crop)
    SCALE_STRETCH    // Alias for SCALE_FILL
};

class ImageElement : public Element
{
public:
    ImageElement(const std::wstring& id, int x, int y, int w, int h,
                 const std::wstring& path, ScaleMode mode);
                 
    virtual ~ImageElement();

    virtual void Render(Gdiplus::Graphics& graphics) override;
    
    // Returns true if image loaded successfully
    bool IsLoaded() const { return m_Image != nullptr && m_Image->GetLastStatus() == Gdiplus::Ok; }
    
    void UpdateImage(const std::wstring& path);

private:
    std::wstring m_ImagePath;
    ScaleMode m_ScaleMode;
    Gdiplus::Image* m_Image;
    
    void LoadImage();
};

#endif
