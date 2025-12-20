/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>

/*
** Content type enumeration.
*/
enum ElementType
{
    ELEMENT_IMAGE,
    ELEMENT_TEXT
};

/*
** Base class for all widget elements (Text, Image, etc.)
*/
class Element
{
public:
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height)
        : m_Type(type), m_Id(id), m_X(x), m_Y(y), m_Width(width), m_Height(height)
    {
    }

    virtual ~Element() {}

    /*
    ** Render the element to the GDI+ Graphics context.
    */
    virtual void Render(Gdiplus::Graphics& graphics) = 0;

    /*
    ** Getters
    */
    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    /*
    ** Setters
    */
    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { m_Width = w; m_Height = h; }

protected:
    ElementType m_Type;
    std::wstring m_Id;
    int m_X, m_Y;
    int m_Width, m_Height;
};

#endif
