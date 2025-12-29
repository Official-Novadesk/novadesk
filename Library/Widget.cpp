/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Widget.h"
#include "PropertyParser.h"
#include "Logging.h"
#include "Settings.h"
#include "Resource.h"
#include <vector>
#include <windowsx.h>
#include <algorithm>
#include <gdiplus.h>

#include "JSApi.h"
#include "ColorUtil.h"
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define WIDGET_CLASS_NAME L"NovadeskWidget"
#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)
#define SNAP_DISTANCE 10

extern std::vector<Widget*> widgets; // Defined in Novadesk.cpp

/*
** Construct a new Widget with the specified options.
** Options include size, position, colors, z-order, and behavior flags.
*/
Widget::Widget(const WidgetOptions& options) 
    : m_hWnd(nullptr), m_Options(options), m_WindowZPosition(options.zPos)
{
}

/*
** Destructor. Cleans up window resources and removes from system tracking.
*/
Widget::~Widget()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
    }
    
    // Clean up elements
    for (auto* element : m_Elements)
    {
        delete element;
    }
    m_Elements.clear();
}

/*
** Register the widget window class.
** Only needs to be called once per application instance.
*/
bool Widget::Register()
{
    static bool registered = false;
    if (registered) return true;

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr; // We'll paint it ourselves
    wcex.lpszClassName = WIDGET_CLASS_NAME;

    if (RegisterClassExW(&wcex))
    {
        registered = true;
        return true;
    }
    return false;
}

/*
** Create the widget window.
** Registers the window class if needed and creates the actual window.
** Returns true on success, false on failure.
*/
bool Widget::Create()
{
    if (!Register()) return false;

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    if (m_Options.clickThrough)
    {
        dwExStyle |= WS_EX_TRANSPARENT;
    }

    m_hWnd = CreateWindowExW(
        dwExStyle,
        WIDGET_CLASS_NAME,
        m_Options.id.c_str(),
        WS_POPUP,
        m_Options.x, m_Options.y, m_Options.width, m_Options.height,
        nullptr, nullptr, hInstance, this);

    if (!m_hWnd) return false;

    // Sync actual position if CW_USEDEFAULT was used or OS repositioned it
    RECT rc;
    if (GetWindowRect(m_hWnd, &rc))
    {
        m_Options.x = rc.left;
        m_Options.y = rc.top;
        if (!m_Options.m_WDefined) m_Options.width = rc.right - rc.left;
        if (!m_Options.m_HDefined) m_Options.height = rc.bottom - rc.top;
    }

    // Initial Z-position - use ChangeSingleZPos to bring new widgets to front
    ChangeSingleZPos(m_WindowZPosition);

    // Initial draw to set content
    UpdateLayeredWindowContent();

    if (m_WindowZPosition == ZPOSITION_ONTOPMOST)
    {
        SetTimer(m_hWnd, TIMER_TOPMOST, 500, nullptr);
    }

    return true;
}

/*
** Show the widget window.
** Makes the window visible and applies the configured z-order position.
*/
void Widget::Show()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hWnd);
    }
}

/*
** Change the z-order position of this widget.
** If all is true, affects all widgets in the same z-order group.
*/
void Widget::ChangeZPos(ZPOSITION zPos, bool all)
{
    ZPOSITION oldZPos = m_WindowZPosition;
    HWND winPos = HWND_NOTOPMOST;
    
    bool changed = (m_Options.zPos != zPos);
    // Logging::Log(LogLevel::Debug, L"ChangeZPos: id=%s, current=%d, new=%d, changed=%d", m_Options.id.c_str(), m_Options.zPos, zPos, changed);
    m_Options.zPos = zPos;
    m_WindowZPosition = zPos;

    switch (zPos)
    {
    case ZPOSITION_ONTOPMOST:
    case ZPOSITION_ONTOP:
        winPos = HWND_TOPMOST;
        break;

    case ZPOSITION_ONBOTTOM:
        if (all)
        {
            if (System::GetShowDesktop())
            {
                winPos = System::GetWindow();
            }
            else
            {
                winPos = System::GetHelperWindow();
            }
        }
        else
        {
            winPos = HWND_BOTTOM;
        }
        break;

    case ZPOSITION_NORMAL:
        break;

    case ZPOSITION_ONDESKTOP:
        if (System::GetShowDesktop())
        {
            winPos = System::GetHelperWindow();
            if (!all)
            {
                while (HWND prev = ::GetNextWindow(winPos, GW_HWNDPREV))
                {
                    if (GetWindowLongPtr(prev, GWL_EXSTYLE) & WS_EX_TOPMOST)
                    {
                        if (SetWindowPos(m_hWnd, prev, 0, 0, 0, 0, ZPOS_FLAGS))
                        {
                            goto timer_check;
                        }
                    }
                    winPos = prev;
                }
            }
        }
        else
        {
            if (all)
            {
                winPos = System::GetHelperWindow();
            }
            else
            {
                winPos = HWND_BOTTOM;
            }
        }
        break;
    }

    SetWindowPos(m_hWnd, winPos, 0, 0, 0, 0, ZPOS_FLAGS);

    // Save Z-Pos state only if it actually changed
    if (changed)
    {
        Settings::SaveWidget(m_Options.id, m_Options);
    }

timer_check:
    if (oldZPos == ZPOSITION_ONTOPMOST && m_WindowZPosition != ZPOSITION_ONTOPMOST)
    {
        KillTimer(m_hWnd, TIMER_TOPMOST);
    }
    else if (oldZPos != ZPOSITION_ONTOPMOST && m_WindowZPosition == ZPOSITION_ONTOPMOST)
    {
        SetTimer(m_hWnd, TIMER_TOPMOST, 500, nullptr);
    }
}

/*
** Change the z-order position of a single widget.
** Similar to ChangeZPos but only affects this specific widget.
*/
void Widget::ChangeSingleZPos(ZPOSITION zPos, bool all)
{
    if (zPos == ZPOSITION_NORMAL && System::GetShowDesktop())
    {
        m_WindowZPosition = zPos;
        SetWindowPos(m_hWnd, System::GetBackmostTopWindow(), 0, 0, 0, 0, ZPOS_FLAGS);
        BringWindowToTop(m_hWnd);
    }
    else if (zPos == ZPOSITION_ONDESKTOP)
    {
        m_WindowZPosition = zPos;
        SetWindowPos(m_hWnd, System::GetBackmostTopWindow(), 0, 0, 0, 0, ZPOS_FLAGS);
        BringWindowToTop(m_hWnd);
    }
    else
    {
        // For clicked widgets, we just want to re-assert z-order without necessarily saving
        // unless it's a new Z-position.
        ChangeZPos(zPos, all);
    }
}

/*
** Set window position and size.
*/
void Widget::SetWindowPosition(int x, int y, int w, int h)
{
    if (x == CW_USEDEFAULT) x = m_Options.x;
    if (y == CW_USEDEFAULT) y = m_Options.y;

    // Use -1 to indicate "keep current" for size
    bool sizeProvided = false;
    if (w != -1)
    {
        m_Options.width = w;
        m_Options.m_WDefined = (w > 0);
        sizeProvided = true;
    }

    if (h != -1)
    {
        m_Options.height = h;
        m_Options.m_HDefined = (h > 0);
        sizeProvided = true;
    }

    bool moved = (x != m_Options.x || y != m_Options.y);

    if (moved || sizeProvided)
    {
        m_Options.x = x;
        m_Options.y = y;

        SetWindowPos(m_hWnd, NULL, x, y, m_Options.width, m_Options.height, SWP_NOZORDER | SWP_NOACTIVATE);
        
        if (sizeProvided)
        {
            Redraw();
        }

        Settings::SaveWidget(m_Options.id, m_Options);
    }
}

/*
** Set overall window opacity (0-255).
*/
void Widget::SetWindowOpacity(BYTE opacity)
{
    if (m_Options.windowOpacity != opacity)
    {
        m_Options.windowOpacity = opacity;
        Redraw();
        Settings::SaveWidget(m_Options.id, m_Options);
    }
}

/*
** Set background color and alpha.
*/
void Widget::SetBackgroundColor(const std::wstring& colorStr)
{
    COLORREF color = m_Options.color;
    BYTE alpha = m_Options.bgAlpha;
    
    if (ColorUtil::ParseRGBA(colorStr, color, alpha))
    {
        if (m_Options.backgroundColor != colorStr || m_Options.color != color || m_Options.bgAlpha != alpha)
        {
            m_Options.backgroundColor = colorStr;
            m_Options.color = color;
            m_Options.bgAlpha = alpha;
            Redraw();
            Settings::SaveWidget(m_Options.id, m_Options);
        }
    }
}

/*
** Enable/disable click-through.
*/
void Widget::SetClickThrough(bool enable)
{
    if (m_Options.clickThrough != enable)
    {
        m_Options.clickThrough = enable;
        
        LONG exStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
        if (enable)
            exStyle |= WS_EX_TRANSPARENT;
        else
            exStyle &= ~WS_EX_TRANSPARENT;
            
        SetWindowLong(m_hWnd, GWL_EXSTYLE, exStyle);
        Settings::SaveWidget(m_Options.id, m_Options);
    }
}

/*
** Retrieve the Widget instance associated with a window handle.
*/
Widget* Widget::GetWidgetFromHWND(HWND hWnd)
{
    for (auto w : widgets)
    {
        if (w->m_hWnd == hWnd) return w;
    }
    return nullptr;
}

/*
** Window procedure for handling widget window messages.
** Handles painting, mouse input, dragging, and z-order management.
*/
LRESULT CALLBACK Widget::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
    {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
        return 0;
    }

    Widget* widget = (Widget*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message)
    {
    case WM_ERASEBKGND:
        return 1; // Handled, we don't need to erase background for layered window
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            return 0;
        }
    case WM_LBUTTONDOWN:
        if (widget)
        {
            widget->HandleMouseMessage(message, wParam, lParam);
            
            // Bring widget to front when clicked (for ondesktop/normal)
            widget->ChangeSingleZPos(widget->m_WindowZPosition);
            
            if (widget->m_Options.draggable)
            {
                SetCapture(hWnd);
                widget->m_IsDragging = true;
                widget->m_DragThresholdMet = false;
                GetCursorPos(&widget->m_DragStartCursor);
                RECT rc;
                GetWindowRect(hWnd, &rc);
                widget->m_DragStartWindow = { rc.left, rc.top };
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (widget)
        {
            if (widget->m_IsDragging)
            {
                widget->m_IsDragging = false;
                ReleaseCapture();
                
                if (widget->m_DragThresholdMet)
                {
                    // Save final position
                    RECT rc;
                    GetWindowRect(hWnd, &rc);
                    widget->m_Options.x = rc.left;
                    widget->m_Options.y = rc.top;
                    Settings::SaveWidget(widget->m_Options.id, widget->m_Options);
                }
                else
                {
                    // If we didn't drag enough, it's a click
                    widget->HandleMouseMessage(message, wParam, lParam);
                }
                widget->m_DragThresholdMet = false;
            }
            else
            {
                widget->HandleMouseMessage(message, wParam, lParam);
            }
        }
        return 0;

    case WM_RBUTTONUP:
        if (widget)
        {
             // If not handled by an element, let DefWindowProc handle it (generates WM_CONTEXTMENU)
             if (!widget->HandleMouseMessage(message, wParam, lParam))
             {
                 return DefWindowProc(hWnd, message, wParam, lParam);
             }
        }
        return 0;

    case WM_CONTEXTMENU:
        if (widget)
        {
            widget->OnContextMenu();
        }
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    // case WM_RBUTTONUP: // Handled above
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (widget)
        {
            widget->HandleMouseMessage(message, wParam, lParam);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (widget) 
        {
            if (widget->m_IsDragging)
            {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - widget->m_DragStartCursor.x;
                int dy = pt.y - widget->m_DragStartCursor.y;

                if (!widget->m_DragThresholdMet)
                {
                    if (abs(dx) > GetSystemMetrics(SM_CXDRAG) || abs(dy) > GetSystemMetrics(SM_CYDRAG))
                    {
                        widget->m_DragThresholdMet = true;
                    }
                }

                if (widget->m_DragThresholdMet)
                {
                    int newX = widget->m_DragStartWindow.x + dx;
                    int newY = widget->m_DragStartWindow.y + dy;

                    SetWindowPos(hWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    
                    // Update local options for hit testing
                    widget->m_Options.x = newX;
                    widget->m_Options.y = newY;
                }
            }
            else
            {
                widget->HandleMouseMessage(message, wParam, lParam);
            }
        }
        return 0;

    case WM_NCHITTEST:
        return HTCLIENT; // We handle everything in client area to get mouse messages
        
    case WM_TIMER:
        if (wParam == TIMER_TOPMOST)
        {
            if (widget && widget->m_WindowZPosition == ZPOSITION_ONTOPMOST)
            {
                widget->ChangeZPos(ZPOSITION_ONTOPMOST);
            }
        }
        return 0;

    case WM_WINDOWPOSCHANGING:
        if (widget)
        {
            LPWINDOWPOS wp = (LPWINDOWPOS)lParam;

            if (widget->m_WindowZPosition == ZPOSITION_NORMAL && System::GetShowDesktop())
            {
                if (!(wp->flags & (SWP_NOOWNERZORDER | SWP_NOACTIVATE)))
                {
                    wp->hwndInsertAfter = System::GetBackmostTopWindow();
                }
            }
            else if (widget->m_WindowZPosition == ZPOSITION_ONBOTTOM)
            {
                wp->flags |= SWP_NOZORDER;
            }

            if (!(wp->flags & SWP_NOMOVE))
            {
                // Snapping
                if (widget->m_Options.snapEdges)
                {
                    const auto& monitors = System::GetMultiMonitorInfo().monitors;
                    RECT windowRect = { wp->x, wp->y, wp->x + widget->m_Options.width, wp->y + widget->m_Options.height };
                    const RECT* workArea = nullptr;

                    // Find monitor with largest intersection
                    LONG maxArea = 0;
                    for (const auto& mon : monitors)
                    {
                        if (!mon.active) continue;
                        RECT intersect;
                        if (::IntersectRect(&intersect, &windowRect, &mon.screen))
                        {
                            LONG area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
                            if (area > maxArea)
                            {
                                maxArea = area;
                                workArea = &mon.work;
                            }
                        }
                    }

                    // Snap to other widgets
                    for (Widget* w : widgets)
                    {
                        if (w == widget) continue;
                        RECT otherRect;
                        GetWindowRect(w->GetWindow(), &otherRect);
                        
                        // Vertical overlap -> Snap horizontally
                        if (wp->y < otherRect.bottom && wp->y + widget->m_Options.height > otherRect.top)
                        {
                            if (abs(wp->x - otherRect.left) < SNAP_DISTANCE) wp->x = otherRect.left;
                            if (abs(wp->x - otherRect.right) < SNAP_DISTANCE) wp->x = otherRect.right;
                            if (abs(wp->x + widget->m_Options.width - otherRect.left) < SNAP_DISTANCE) wp->x = otherRect.left - widget->m_Options.width;
                            if (abs(wp->x + widget->m_Options.width - otherRect.right) < SNAP_DISTANCE) wp->x = otherRect.right - widget->m_Options.width;
                        }

                        // Horizontal overlap -> Snap vertically
                        if (wp->x < otherRect.right && wp->x + widget->m_Options.width > otherRect.left)
                        {
                            if (abs(wp->y - otherRect.top) < SNAP_DISTANCE) wp->y = otherRect.top;
                            if (abs(wp->y - otherRect.bottom) < SNAP_DISTANCE) wp->y = otherRect.bottom;
                            if (abs(wp->y + widget->m_Options.height - otherRect.top) < SNAP_DISTANCE) wp->y = otherRect.top - widget->m_Options.height;
                            if (abs(wp->y + widget->m_Options.height - otherRect.bottom) < SNAP_DISTANCE) wp->y = otherRect.bottom - widget->m_Options.height;
                        }
                    }

                    // Snap to screen edges
                    if (workArea)
                    {
                        if (abs(wp->x - workArea->left) < SNAP_DISTANCE) wp->x = workArea->left;
                        if (abs(wp->y - workArea->top) < SNAP_DISTANCE) wp->y = workArea->top;
                        if (abs(wp->x + widget->m_Options.width - workArea->right) < SNAP_DISTANCE) wp->x = workArea->right - widget->m_Options.width;
                        if (abs(wp->y + widget->m_Options.height - workArea->bottom) < SNAP_DISTANCE) wp->y = workArea->bottom - widget->m_Options.height;
                    }
                }

                // Keep on screen (using Rainmeter's robust multi-point algorithm)
                if (widget->m_Options.keepOnScreen)
                {
                    const auto& monitors = System::GetMultiMonitorInfo().monitors;
                    const RECT* targetMonitor = nullptr;
                    
                    // Try 5 different points to find which monitor contains the window
                    // This is more robust than just checking the center point
                    POINT testPoints[5] = {
                        { wp->x + widget->m_Options.width / 2, wp->y + widget->m_Options.height / 2 },  // Center
                        { wp->x, wp->y },                                                                 // Top-left
                        { wp->x + widget->m_Options.width, wp->y + widget->m_Options.height },           // Bottom-right
                        { wp->x, wp->y + widget->m_Options.height },                                     // Bottom-left
                        { wp->x + widget->m_Options.width, wp->y }                                       // Top-right
                    };
                    
                    for (int i = 0; i < 5 && !targetMonitor; ++i)
                    {
                        for (const auto& mon : monitors)
                        {
                            if (!mon.active) continue;
                            
                            // Check if point is within monitor's screen area
                            if (testPoints[i].x >= mon.screen.left && testPoints[i].x < mon.screen.right &&
                                testPoints[i].y >= mon.screen.top && testPoints[i].y < mon.screen.bottom)
                            {
                                targetMonitor = &mon.screen;
                                break;
                            }
                        }
                    }
                    
                    // If no monitor found, use primary monitor
                    if (!targetMonitor)
                    {
                        const int primaryIndex = System::GetMultiMonitorInfo().primary - 1;
                        if (primaryIndex >= 0 && primaryIndex < (int)monitors.size())
                        {
                            targetMonitor = &monitors[primaryIndex].screen;
                        }
                    }
                    
                    // Constrain window position to monitor bounds
                    if (targetMonitor)
                    {
                        wp->x = (std::min)(wp->x, (int)targetMonitor->right - widget->m_Options.width);
                        wp->x = (std::max)(wp->x, (int)targetMonitor->left);
                        wp->y = (std::min)(wp->y, (int)targetMonitor->bottom - widget->m_Options.height);
                        wp->y = (std::max)(wp->y, (int)targetMonitor->top);
                    }
                }
            }
        }
        return 0;

    case WM_WINDOWPOSCHANGED:
        if (widget)
        {
            LPWINDOWPOS wp = (LPWINDOWPOS)lParam;
            if (!(wp->flags & SWP_NOMOVE))
            {
                widget->m_Options.x = wp->x;
                widget->m_Options.y = wp->y;
            }
            if (!(wp->flags & SWP_NOSIZE))
            {
                widget->m_Options.width = wp->cx;
                widget->m_Options.height = wp->cy;
            }
        }
        return 0;
    case WM_EXITSIZEMOVE:
        if (widget)
        {
            RECT rc;
            GetWindowRect(hWnd, &rc);
            widget->m_Options.x = rc.left;
            widget->m_Options.y = rc.top;
            // Width/Height might change if we allow resizing (not implemented yet for borderless but good to have)
            // widget->m_Options.width = rc.right - rc.left; 
            // widget->m_Options.height = rc.bottom - rc.top;
            
            Settings::SaveWidget(widget->m_Options.id, widget->m_Options);
        }
        return 0;
    case WM_DESTROY:
        if (widget)
        {
            auto it = std::find(widgets.begin(), widgets.end(), widget);
            if (it != widgets.end()) widgets.erase(it);
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// Content Management Methods
// ============================================================================

// Helper to apply common element options
/*
** Add an image content item to the widget.
** The image will be loaded and cached for rendering.
*/
void Widget::AddImage(const PropertyParser::ImageOptions& options)
{
    if (options.id.empty()) {
        Logging::Log(LogLevel::Error, L"AddImage failed: Element ID cannot be empty.");
        return;
    }

    if (FindElementById(options.id)) {
        Logging::Log(LogLevel::Error, L"AddImage failed: Element with ID '%s' already exists.", options.id.c_str());
        return;
    }

    ImageElement* element = new ImageElement(options.id, options.x, options.y, options.width, options.height, options.path);
    element->SetPreserveAspectRatio(options.preserveAspectRatio);
    element->SetImageAlpha(options.imageAlpha);
    element->SetGrayscale(options.grayscale);
    element->SetTile(options.tile);
        
    if (options.hasTransformMatrix)
    {
        element->SetTransformMatrix(options.transformMatrix.data());
    }
    
    if (options.hasColorMatrix)
    {
        element->SetColorMatrix(options.colorMatrix.data());
    }

    if (options.hasImageTint)
    {
        element->SetImageTint(options.imageTint, options.imageTintAlpha);
    }
    
    PropertyParser::ApplyElementOptions(element, options);
    
    m_Elements.push_back(element);
    
    Redraw();
}

/*
** Add a text content item to the widget.
** Text will be rendered with the specified font and styling.
*/
void Widget::AddText(const PropertyParser::TextOptions& options)
{
    if (options.id.empty()) {
        Logging::Log(LogLevel::Error, L"AddText failed: Element ID cannot be empty.");
        return;
    }

    if (FindElementById(options.id)) {
        Logging::Log(LogLevel::Error, L"AddText failed: Element with ID '%s' already exists.", options.id.c_str());
        return;
    }

    TextElement* element = new TextElement(options.id, options.x, options.y, options.width, options.height, 
                             options.text, options.fontFace, options.fontSize, options.fontColor, options.alpha,
                             options.bold, options.italic, options.textAlign, options.clip, options.clipW, options.clipH);
                             
    PropertyParser::ApplyElementOptions(element, options);

    m_Elements.push_back(element);
    
    Redraw();
}

/*
** Update properties of an existing element.
*/
void Widget::SetElementProperties(const std::wstring& id, duk_context* ctx)
{
    Element* element = FindElementById(id);
    if (!element) return;

    if (element->GetType() == ELEMENT_TEXT) {
        PropertyParser::TextOptions options;
        
        // Fill struct with current state before parsing for partial updates
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();
        
        TextElement* t = static_cast<TextElement*>(element);
        options.text = t->GetText();
        options.fontFace = t->GetFontFace();
        options.fontSize = t->GetFontSize();
        options.fontColor = t->GetFontColor();
        options.alpha = t->GetFontAlpha();
        options.bold = t->IsBold();
        options.italic = t->IsItalic();
        options.textAlign = t->GetTextAlign();
        options.clip = t->GetClipString();
        options.clipW = t->GetClipW();
        options.clipH = t->GetClipH();

        PropertyParser::ParseTextOptions(ctx, options);
        PropertyParser::ApplyTextOptions(t, options);
    } else if (element->GetType() == ELEMENT_IMAGE) {
        PropertyParser::ImageOptions options;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.rotate = element->GetRotate();
        options.antialias = element->GetAntiAlias();

        ImageElement* img = static_cast<ImageElement*>(element);
        options.path = img->GetImagePath();
        options.preserveAspectRatio = img->GetPreserveAspectRatio();
        options.imageAlpha = img->GetImageAlpha();
        options.grayscale = img->IsGrayscale();
        options.tile = img->IsTile();

        PropertyParser::ParseImageOptions(ctx, options);
        PropertyParser::ApplyImageOptions(img, options);
    }

    Redraw();
}

/*
** Remove one or more content items by ID.
** If id is empty, clears all content.
*/
bool Widget::RemoveElements(const std::wstring& id)
{
    if (id.empty())
    {
        for (auto* el : m_Elements) delete el;
        m_Elements.clear();
        m_MouseOverElement = nullptr;
        Redraw();
        return true;
    }

    for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
    {
        if ((*it)->GetId() == id)
        {
            if (*it == m_MouseOverElement) m_MouseOverElement = nullptr;
            delete *it;
            m_Elements.erase(it);
            Redraw();
            return true;
        }
    }
    return false;
}

/*
** Remove multiple elements by their IDs.
*/
void Widget::RemoveElements(const std::vector<std::wstring>& ids)
{
    bool changed = false;
    for (const auto& id : ids)
    {
        for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
        {
            if ((*it)->GetId() == id)
            {
                if (*it == m_MouseOverElement) m_MouseOverElement = nullptr;
                delete *it;
                m_Elements.erase(it);
                changed = true;
                break;
            }
        }
    }
    if (changed) Redraw();
}

/*
** Add a custom item to the context menu.
*/
void Widget::AddContextMenuItem(const std::wstring& label, const std::wstring& action)
{
    m_ContextMenuItems.push_back({ label, action });
}

/*
** Clear all custom context menu items.
*/
void Widget::ClearContextMenuItems()
{
    m_ContextMenuItems.clear();
}

/*
** Remove a specific context menu item by label.
*/
void Widget::RemoveContextMenuItem(const std::wstring& label)
{
    auto it = std::remove_if(m_ContextMenuItems.begin(), m_ContextMenuItems.end(),
        [&](const ContextMenuItem& item) { return item.label == label; });

    if (it != m_ContextMenuItems.end())
    {
        m_ContextMenuItems.erase(it, m_ContextMenuItems.end());
    }
}

/*
** Redraw the widget window to reflect content changes.
*/
void Widget::Redraw()
{
    UpdateLayeredWindowContent();
}



/*
** Update the layered window content using UpdateLayeredWindow.
** Draws all content to a memory DC and updates the window.
*/
void Widget::UpdateLayeredWindowContent()
{
    if (!m_hWnd) return;

    // 1. Calculate bounding box of all elements if widget size is not defined
    int calcW = m_Options.width;
    int calcH = m_Options.height;

    if (!m_Options.m_WDefined || !m_Options.m_HDefined)
    {
        int maxX = 0;
        int maxY = 0;
        for (Element* element : m_Elements)
        {
            maxX = (std::max)(maxX, element->GetX() + element->GetWidth());
            maxY = (std::max)(maxY, element->GetY() + element->GetHeight());
        }
        
        if (!m_Options.m_WDefined) calcW = maxX;
        if (!m_Options.m_HDefined) calcH = maxY;
        
        // Ensure at least 1x1
        if (calcW <= 0) calcW = 1;
        if (calcH <= 0) calcH = 1;
        
        // If size changed, update window and options
        if (calcW != m_Options.width || calcH != m_Options.height)
        {
            m_Options.width = calcW;
            m_Options.height = calcH;
            SetWindowPos(m_hWnd, NULL, 0, 0, calcW, calcH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    // Use current dimensions
    int w = m_Options.width;
    int h = m_Options.height;
    if (w <= 0 || h <= 0) return;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    // Create 32-bit bitmap for alpha channel
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pvBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Draw GDI+
    {
        Graphics graphics(hdcMem);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
        graphics.SetCompositingMode(CompositingModeSourceOver);

        // Clear with 0 alpha (fully transparent)
        graphics.Clear(Color(0, 0, 0, 0));

        // Draw Background
        Color backColor(m_Options.bgAlpha, GetRValue(m_Options.color), GetGValue(m_Options.color), GetBValue(m_Options.color));
        SolidBrush backBrush(backColor);
        graphics.FillRectangle(&backBrush, 0, 0, w, h);

        // Draw Elements
        for (Element* element : m_Elements)
        {
            element->Render(graphics);
        }
    }

    POINT pptDst = { 0, 0 };
    // We need current window position
    RECT rc; 
    GetWindowRect(m_hWnd, &rc);
    pptDst.x = rc.left;
    pptDst.y = rc.top;
    
    POINT pptSrc = { 0, 0 };
    SIZE size = { w, h };
    
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = m_Options.windowOpacity; // Master opacity
    bf.AlphaFormat = AC_SRC_ALPHA; // Pre-multiplied alpha

    UpdateLayeredWindow(m_hWnd, hdcScreen, &pptDst, &size, hdcMem, &pptSrc, 0, &bf, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

/*
** Find a content element by its ID.
** Returns pointer to the element or nullptr if not found.
*/
Element* Widget::FindElementById(const std::wstring& id)
{
    for (auto* element : m_Elements)
    {
        if (element->GetId() == id) return element;
    }
    return nullptr;
}

/*
** Handle mouse messages and dispatch to elements.
** Returns true if the message was handled by an element, false otherwise.
*/
bool Widget::HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool handled = false;
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    // For mouse wheel, coordinates are screen relative
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        POINT pt = { x, y };
        ScreenToClient(m_hWnd, &pt);
        x = pt.x;
        y = pt.y;
    }

    // Find element at cursor (Front to Back)
    Element* hitElement = nullptr;
    Element* actionElement = nullptr;

    for (auto it = m_Elements.rbegin(); it != m_Elements.rend(); ++it)
    {
        Element* el = *it;
        if (!el) continue;
        if (el->HitTest(x, y))
        {
            if (!hitElement) hitElement = el;

            // Check if this element HANDLES the action
            if (el->HasAction(message, wParam))
            {
                actionElement = el;
                break;
            }
            
            // If it hits but has no action, it FALLS THROUGH to elements below (Rainmeter behavior)
        }
    }

    // Handle Hover/Leave logic (this always uses the top-most hit element, regardless of actions)
    if (message == WM_MOUSEMOVE)
    {
        if (hitElement != m_MouseOverElement)
        {
            if (m_MouseOverElement)
            {
                m_MouseOverElement->m_IsMouseOver = false;
                std::wstring leaveAction = m_MouseOverElement->m_OnMouseLeave;
                if (!leaveAction.empty())
                    JSApi::ExecuteScript(leaveAction);
                
                // If ExecuteScript cleared the elements, m_MouseOverElement is now invalid
                if (m_Elements.empty()) {
                    m_MouseOverElement = nullptr;
                    return true; // Technically handled or compromised
                }
            }

            if (hitElement)
            {
                hitElement->m_IsMouseOver = true;
                std::wstring overAction = hitElement->m_OnMouseOver;
                if (!overAction.empty())
                    JSApi::ExecuteScript(overAction);

                // If ExecuteScript cleared the elements, hitElement is now invalid
                if (m_Elements.empty()) {
                    m_MouseOverElement = nullptr;
                    return true;
                }
            }
            m_MouseOverElement = hitElement;
        }
        
        // Ensure we track mouse leave window events
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        TrackMouseEvent(&tme);
        
        handled = true; // Mouse move is generally considered handled if we process hovers? 
                        // Actually for context menu, only RBUTTONUP handling implies consumption.
                        // But for function signature satisfaction, we return true if we did logic.
    }
    else if (message == WM_MOUSELEAVE)
    {
        if (m_MouseOverElement)
        {
             m_MouseOverElement->m_IsMouseOver = false;
             std::wstring leaveAction = m_MouseOverElement->m_OnMouseLeave;
             if (!leaveAction.empty())
                 JSApi::ExecuteScript(leaveAction);
             m_MouseOverElement = nullptr;
        }
        handled = true;
    }

    // Dispatch Actions
    if (actionElement)
    {
        std::wstring action;
        switch (message)
        {
        case WM_LBUTTONUP:     action = actionElement->m_OnLeftMouseUp; break;
        case WM_LBUTTONDOWN:   action = actionElement->m_OnLeftMouseDown; break;
        case WM_LBUTTONDBLCLK: action = actionElement->m_OnLeftDoubleClick; break;
        case WM_RBUTTONUP:     action = actionElement->m_OnRightMouseUp; break;
        case WM_RBUTTONDOWN:   action = actionElement->m_OnRightMouseDown; break;
        case WM_RBUTTONDBLCLK: action = actionElement->m_OnRightDoubleClick; break;
        case WM_MBUTTONUP:     action = actionElement->m_OnMiddleMouseUp; break;
        case WM_MBUTTONDOWN:   action = actionElement->m_OnMiddleMouseDown; break;
        case WM_MBUTTONDBLCLK: action = actionElement->m_OnMiddleDoubleClick; break;
        case WM_XBUTTONUP:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) action = actionElement->m_OnX1MouseUp;
            else action = actionElement->m_OnX2MouseUp;
            break;
        case WM_XBUTTONDOWN:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) action = actionElement->m_OnX1MouseDown;
            else action = actionElement->m_OnX2MouseDown;
            break;
        case WM_XBUTTONDBLCLK:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) action = actionElement->m_OnX1DoubleClick;
            else action = actionElement->m_OnX2DoubleClick;
            break;
        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) action = actionElement->m_OnScrollUp;
            else action = actionElement->m_OnScrollDown;
            break;
        case WM_MOUSEHWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) action = actionElement->m_OnScrollRight;
            else action = actionElement->m_OnScrollLeft;
            break;
        }

        if (!action.empty())
        {
            // ExecuteScript might clear elements, so we must use our local copy of the action string
            JSApi::ExecuteScript(action);
            handled = true;
        }
    }
    
    return handled;
}

/*
** Show the context menu for the widget.
*/
void Widget::OnContextMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    
    // Custom Items (IDs 2000+)
    int customIdStart = 2000;
    for (size_t i = 0; i < m_ContextMenuItems.size(); ++i)
    {
        AppendMenu(hMenu, MF_STRING, customIdStart + i, m_ContextMenuItems[i].label.c_str());
    }

    if (m_ShowDefaultContextMenuItems)
    {
        if (!m_ContextMenuItems.empty())
        {
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        }
        // 1001: Refresh, 1002: Exit Widget, 1003: Exit App
        AppendMenu(hMenu, MF_STRING, 1001, L"Refresh Widget");
        AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(hMenu, MF_STRING, 1002, L"Exit Widget");
        AppendMenu(hMenu, MF_STRING, 1003, L"Exit App");
    }

    SetForegroundWindow(m_hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd >= 2000)
    {
        size_t index = cmd - 2000;
        if (index < m_ContextMenuItems.size())
        {
            JSApi::ExecuteScript(m_ContextMenuItems[index].action);
        }
    }
    else if (cmd == 1001)
    {
        JSApi::Reload();
    }
    else if (cmd == 1002)
    {
        // Safe deletion from system
        auto it = std::find(widgets.begin(), widgets.end(), this);
        if (it != widgets.end())
        {
            widgets.erase(it);
        }
        delete this; // Calls DestroyWindow
    }
    else if (cmd == 1003)
    {
        PostQuitMessage(0);
    }
}

