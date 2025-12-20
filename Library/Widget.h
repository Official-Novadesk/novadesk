/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGET_H__
#define __NOVADESK_WIDGET_H__

#include <windows.h>
#include <string>
#include <vector>
#include "System.h"
#include "Element.h"
#include "Text.h"
#include "Image.h"

struct WidgetOptions
{
    int width;
    int height;
    std::wstring backgroundColor;
    ZPOSITION zPos;
    BYTE alpha;
    COLORREF color;
    bool draggable;
    bool clickThrough;
    bool keepOnScreen;
    bool snapEdges;
};

class Widget
{
public:
    /*
    ** Construct a new Widget with the specified options.
    ** Options include size, position, colors, z-order, and behavior flags.
    */
    Widget(const WidgetOptions& options);

    /*
    ** Destructor. Cleans up window resources and removes from system tracking.
    */
    ~Widget();

    /*
    ** Create the widget window.
    ** Registers the window class if needed and creates the actual window.
    ** Returns true on success, false on failure.
    */
    bool Create();

    /*
    ** Show the widget window.
    ** Makes the window visible and applies the configured z-order position.
    */
    void Show();

    /*
    ** Change the z-order position of this widget.
    ** If all is true, affects all widgets in the same z-order group.
    */
    void ChangeZPos(ZPOSITION zPos, bool all = false);

    /*
    ** Change the z-order position of a single widget.
    ** Similar to ChangeZPos but only affects this specific widget.
    */
    void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
    
    /*
    ** Get the window handle for this widget.
    */
    HWND GetWindow() const { return m_hWnd; }

    /*
    ** Get the current z-order position of this widget.
    */
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }

    /*
    ** Add an image content item to the widget.
    ** The image will be loaded and cached for rendering.
    */
    void AddImage(const std::wstring& id, int x, int y, int w, int h, 
                  const std::wstring& path, ScaleMode mode = SCALE_FILL);

    /*
    ** Add a text content item to the widget.
    ** Text will be rendered with the specified font and styling.
    */
    void AddText(const std::wstring& id, int x, int y, int w, int h,
                 const std::wstring& text, const std::wstring& fontFamily,
                 int fontSize, COLORREF color, BYTE alpha, bool bold = false,
                 bool italic = false, TextAlign align = ALIGN_LEFT,
                 VerticalAlign vAlign = VALIGN_TOP, float lineHeight = 1.0f);

    /*
    ** Update an existing image content item with a new image path.
    ** Returns true if the item was found and updated.
    */
    bool UpdateImage(const std::wstring& id, const std::wstring& newPath);

    /*
    ** Update an existing text content item with new text.
    ** Returns true if the item was found and updated.
    */
    bool UpdateText(const std::wstring& id, const std::wstring& newText);

    /*
    ** Remove a content item by its ID.
    ** Returns true if the item was found and removed.
    */
    bool RemoveContent(const std::wstring& id);

    /*
    ** Clear all content items from the widget.
    */
    void ClearContent();

    /*
    ** Redraw the widget window to reflect content changes.
    */
    void Redraw();

private:
    /*
    ** Window procedure for handling widget window messages.
    ** Handles painting, mouse input, dragging, and z-order management.
    */
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /*
    ** Register the widget window class.
    ** Only needs to be called once per application instance.
    */
    static bool Register();

    /*
    ** Retrieve the Widget instance associated with a window handle.
    */
    static Widget* GetWidgetFromHWND(HWND hWnd);

    /*
    ** Render all content items (images and text) to the device context.
    */
    void RenderContent(HDC hdc);

    /*
    ** Find a content element by its ID.
    ** Returns pointer to the element or nullptr if not found.
    */
    Element* FindElementById(const std::wstring& id);
    
    HWND m_hWnd;
    WidgetOptions m_Options;
    ZPOSITION m_WindowZPosition;
    HBRUSH m_hBackBrush;
    std::vector<Element*> m_Elements;

    static const UINT_PTR TIMER_TOPMOST = 2;
};

#endif

