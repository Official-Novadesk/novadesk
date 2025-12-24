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
    std::wstring id;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::wstring backgroundColor = L"rgba(255,255,255,255)";
    ZPOSITION zPos = ZPOSITION_NORMAL;
    BYTE bgAlpha = 255;        // Alpha component of background color (0-255)
    BYTE windowOpacity = 255;  // Overall window opacity (0-255)
    COLORREF color = RGB(255, 255, 255);
    bool draggable = true;
    bool clickThrough = false;
    bool keepOnScreen = false;
    bool snapEdges = true;
    bool m_WDefined = false;
    bool m_HDefined = false;
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
    ** Set window position and size.
    */
    void SetWindowPosition(int x, int y, int w, int h);

    /*
    ** Set overall window opacity (0-255).
    */
    void SetWindowOpacity(BYTE opacity);

    /*
    ** Set background color and alpha.
    */
    void SetBackgroundColor(const std::wstring& colorStr);

    /*
    ** Enable/disable window dragging.
    */
    void SetDraggable(bool enable) { m_Options.draggable = enable; }

    /*
    ** Enable/disable click-through.
    */
    void SetClickThrough(bool enable);

    /*
    ** Enable/disable keep on screen.
    */
    void SetKeepOnScreen(bool enable) { m_Options.keepOnScreen = enable; }

    /*
    ** Enable/disable snap edges.
    */
    void SetSnapEdges(bool enable) { m_Options.snapEdges = enable; }

    /*
    ** Get current widget options.
    */
    const WidgetOptions& GetOptions() const { return m_Options; }
    
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
                  const std::wstring& path, const std::wstring& solidColor = L"",
                  int solidColorRadius = 0,
                  int preserveAspectRatio = 0,
                  const std::wstring& imageTint = L"",
                  int imageAlpha = 255,
                  bool grayscale = false,
                  const std::vector<float>& colorMatrix = {},
                  bool tile = false,
                  float rotate = 0.0f,
                  const std::vector<float>& transformMatrix = {});

    /*
    ** Add a text content item to the widget.
    ** Text will be rendered with the specified font and styling.
    */
    void AddText(const std::wstring& id, int x, int y, int w, int h,
                 const std::wstring& text, const std::wstring& fontFamily,
                 int fontSize, COLORREF color, BYTE alpha, bool bold = false,
                 bool italic = false, Alignment align = ALIGN_LEFT_TOP,
                 ClipString clip = CLIP_NONE, int clipW = -1, int clipH = -1,
                 const std::wstring& solidColor = L"", int solidColorRadius = 0);

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

    /*
    ** Find a content element by its ID.
    ** Returns pointer to the element or nullptr if not found.
    */
    Element* FindElementById(const std::wstring& id);

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
    ** Update the layered window content using UpdateLayeredWindow.
    ** Draws all content to a memory DC and updates the window.
    */
    void UpdateLayeredWindowContent();

    /*
    ** Handle mouse messages and dispatch to elements.
    */
    void HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_hWnd;
    WidgetOptions m_Options;
    ZPOSITION m_WindowZPosition;
    std::vector<Element*> m_Elements;
    Element* m_MouseOverElement = nullptr;

    // Dragging State
    bool m_IsDragging = false;
    bool m_DragThresholdMet = false;
    POINT m_DragStartCursor = { 0, 0 };
    POINT m_DragStartWindow = { 0, 0 };

    static const UINT_PTR TIMER_TOPMOST = 2;
};

#endif

