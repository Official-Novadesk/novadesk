/* Copyright (C) 2026 OfficialNovadesk 
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
#include "MenuUtils.h"
#include "JSApi/JSContextMenu.h"
#include "Utils.h"
#include <vector>
#include <windowsx.h>
#include <algorithm>
#include "Direct2DHelper.h"
#include "ImageElement.h"
#include "TextElement.h"
#include "BarElement.h"
#include "RoundLineElement.h"
#include "JSApi/JSApi.h"
#include "JSApi/JSCommon.h"
#include "JSApi/JSEvents.h"
#include "RectangleShape.h"
#include "EllipseShape.h"
#include "LineShape.h"
#include "ArcShape.h"
#include "PathShape.h"
#include "CurveShape.h"
#include "ShapeElement.h"
#include "ColorUtil.h"
#include "PathUtils.h"

#define WIDGET_CLASS_NAME L"NovadeskWidget"
#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)
#define SNAP_DISTANCE 10

extern std::vector<Widget*> widgets; // Defined in Novadesk.cpp

/*
** Check if a widget pointer is valid (exists in the global widgets list).
*/
bool Widget::IsValid(Widget* pWidget)
{
    if (!pWidget) return false;
    for (auto* w : widgets)
    {
        if (w == pWidget) return true;
    }
    return false;
}


/*
** Construct a new Widget with the specified options.
** Options include size, position, colors, z-order, and behavior flags.
*/
Widget::Widget(const WidgetOptions& options) 
    : m_hWnd(nullptr), m_Options(options), m_WindowZPosition(options.zPos), m_IsBatchUpdating(false)
{
}

/*
** Destructor. Cleans up window resources and removes from system tracking.
*/
Widget::~Widget()
{
    JSApi::TriggerWidgetEvent(this, "close");
    JSApi::CleanupWidget(m_Options.id);

    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
    }
    JSApi::TriggerWidgetEvent(this, "closed");
    
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
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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

    // Initialize tooltip
    m_Tooltip.Initialize(m_hWnd, hInstance);

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
        JSApi::TriggerWidgetEvent(this, "show");
    }
}

void Widget::Hide()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_HIDE);
        JSApi::TriggerWidgetEvent(this, "hide");
    }
}

void Widget::Refresh()
{
    JSApi::TriggerWidgetEvent(this, "refresh");

    BeginUpdate();
    RemoveElements(L""); // Clear all elements
    if (!m_Options.scriptPath.empty())
    {
        JSApi::ExecuteWidgetScript(this);
    }
    EndUpdate();
}

void Widget::SetFocus()
{
    if (m_hWnd) {
        ::SetFocus(m_hWnd);
    }
}

void Widget::UnFocus()
{
    if (m_hWnd && ::GetFocus() == m_hWnd) {
        ::SetFocus(NULL);
    }
}

std::wstring Widget::GetTitle() const
{
    if (!m_hWnd) return L"";
    int len = GetWindowTextLength(m_hWnd);
    if (len == 0) return L"";
    std::vector<wchar_t> buf(len + 1);
    GetWindowText(m_hWnd, buf.data(), len + 1);
    return std::wstring(buf.data());
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
        if (all) break;
        // Fallthrough

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
    if (zPos == ZPOSITION_NORMAL && (!all || System::GetShowDesktop()))
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
    GradientInfo grad;
    
    bool isGradient = Utils::ParseGradientString(colorStr, grad);
    bool isColor = ColorUtil::ParseRGBA(colorStr, color, alpha);

    if (isGradient || isColor)
    {
        bool changed = false;
        if (m_Options.backgroundColor != colorStr) {
            m_Options.backgroundColor = colorStr;
            changed = true;
        }

        if (isGradient) {
            if (m_Options.bgGradient.type != grad.type || m_Options.bgGradient.angle != grad.angle || m_Options.bgGradient.stops.size() != grad.stops.size()) {
                m_Options.bgGradient = grad;
                changed = true;
            }
        } else {
            if (m_Options.bgGradient.type != GRADIENT_NONE) {
                m_Options.bgGradient.type = GRADIENT_NONE;
                changed = true;
            }
            if (m_Options.color != color || m_Options.bgAlpha != alpha) {
                m_Options.color = color;
                m_Options.bgAlpha = alpha;
                changed = true;
            }
        }

        if (changed) {
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
    case WM_SETFOCUS:
        if (widget) JSApi::TriggerWidgetEvent(widget, "focus");
        return 0;

    case WM_KILLFOCUS:
        if (widget) JSApi::TriggerWidgetEvent(widget, "unFocus");
        return 0;

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
            
            // Bring widget to front when clicked (for normal)
            if (widget->m_WindowZPosition != ZPOSITION_ONDESKTOP && widget->m_WindowZPosition != ZPOSITION_ONBOTTOM)
            {
                widget->ChangeSingleZPos(widget->m_WindowZPosition);
            }
            
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

    case WM_MOUSELEAVE:
        if (widget)
        {
            widget->HandleMouseMessage(message, wParam, lParam);
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

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && widget)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            Element* hitElement = nullptr;
            Element* mouseActionElement = nullptr;
            for (auto it = widget->m_Elements.rbegin(); it != widget->m_Elements.rend(); ++it)
            {
                Element* candidate = *it;
                if (!candidate || !candidate->IsVisible()) continue;
                if (candidate->IsContained()) continue;

                if (candidate->IsContainer()) {
                    Element* childHit = nullptr;
                    Element* childAction = nullptr;
                    Element* childMouseAction = nullptr;
                    Element* childToolTip = nullptr;
                    if (widget->HitTestContainerChildrenDetailed(
                        candidate, pt.x, pt.y, WM_MOUSEMOVE, 0, childHit, childAction, childMouseAction, childToolTip)) {
                        if (!hitElement) hitElement = childHit;
                        if (!mouseActionElement) mouseActionElement = childMouseAction;
                    }
                }

                if (candidate->HitTest(pt.x, pt.y)) {
                    if (!hitElement) hitElement = candidate;
                    if (!mouseActionElement && candidate->HasMouseAction()) mouseActionElement = candidate;
                }

                if (hitElement && mouseActionElement) break;
            }

            Element* cursorElement = mouseActionElement ? mouseActionElement : hitElement;
            if (cursorElement && cursorElement->HasMouseAction())
            {
                HCURSOR cursor = widget->m_CursorManager.GetCursorForElement(cursorElement);
                if (cursor) {
                    SetCursor(cursor);
                    return TRUE;
                }
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

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

    case WM_MOUSEACTIVATE:
        if (widget)
        {
            if (widget->m_WindowZPosition == ZPOSITION_ONDESKTOP || widget->m_WindowZPosition == ZPOSITION_ONBOTTOM)
            {
                return MA_NOACTIVATE;
            }
        }
        return MA_ACTIVATE;

    case WM_NCHITTEST:
        return HTCLIENT; // We handle everything in client area to get mouse messages
        
    case WM_TIMER:
        if (widget)
        {
            if (wParam == TIMER_TOPMOST)
            {
                if (widget->m_WindowZPosition == ZPOSITION_ONTOPMOST)
                {
                    widget->ChangeZPos(ZPOSITION_ONTOPMOST);
                }
            }
            else if (wParam == TIMER_TOOLTIP)
            {
                widget->m_Tooltip.CheckVisibility();
                // If tooltip is no longer active after check, kill the timer
                if (!widget->m_Tooltip.IsActive())
                {
                    KillTimer(hWnd, TIMER_TOOLTIP);
                    
                    // If we hid the tooltip, we likely lost mouse focus or are covered.
                    // Reset mouse over element state so that when we return, we trigger a fresh enter.
                    if (widget->m_MouseOverElement)
                    {

                        widget->m_MouseOverElement->m_IsMouseOver = false;
                        
                        int leaveId = widget->m_MouseOverElement->m_OnMouseLeaveCallbackId;
                        if (leaveId != -1)
                             JSApi::CallEventCallback(leaveId);
                             
                        widget->m_MouseOverElement = nullptr;
                    }
                    widget->m_TooltipElement = nullptr;
                }
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
            else if (widget->m_WindowZPosition == ZPOSITION_ONBOTTOM || widget->m_WindowZPosition == ZPOSITION_ONDESKTOP)
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
                        if (wp->y < otherRect.bottom + SNAP_DISTANCE && wp->y + widget->m_Options.height > otherRect.top - SNAP_DISTANCE)
                        {
                            if (abs(wp->x - otherRect.left) < SNAP_DISTANCE) wp->x = otherRect.left;
                            if (abs(wp->x - otherRect.right) < SNAP_DISTANCE) wp->x = otherRect.right;
                            if (abs(wp->x + widget->m_Options.width - otherRect.left) < SNAP_DISTANCE) wp->x = otherRect.left - widget->m_Options.width;
                            if (abs(wp->x + widget->m_Options.width - otherRect.right) < SNAP_DISTANCE) wp->x = otherRect.right - widget->m_Options.width;
                        }

                        // Horizontal overlap -> Snap vertically
                        if (wp->x < otherRect.right + SNAP_DISTANCE && wp->x + widget->m_Options.width > otherRect.left - SNAP_DISTANCE)
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
                JSApi::TriggerWidgetEvent(widget, "move");
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
        RemoveElements(options.id);
    }

    ImageElement* element = new ImageElement(options.id, options.x, options.y, options.width, options.height, options.path);
    
    PropertyParser::ApplyImageOptions(element, options); // Changed from ApplyElementOptions and moved

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
    
    m_Elements.push_back(element);
    UpdateContainerForElement(element, options.containerId);
    
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
        RemoveElements(options.id);
    }

    TextElement* element = new TextElement(options.id, options.x, options.y, options.width, options.height, 
                             options.text, options.fontFace, options.fontSize, options.fontColor, options.alpha,
                             options.fontWeight, options.italic, options.textAlign, options.clip, options.fontPath);
                             
    // Logging::Log(LogLevel::Debug, L"Widget::AddText: Created TextElement id='%s', text='%s', x=%d, y=%d", element->GetId().c_str(), element->GetText().c_str(), element->GetX(), element->GetY());

    PropertyParser::ApplyTextOptions(element, options); // Changed from ApplyElementOptions

    m_Elements.push_back(element);
    UpdateContainerForElement(element, options.containerId);
    
    Redraw();
}

/*
** Add a bar content item to the widget.
*/
void Widget::AddBar(const PropertyParser::BarOptions& options)
{
    if (options.id.empty()) {
        Logging::Log(LogLevel::Error, L"AddBar failed: Element ID cannot be empty.");
        return;
    }

    if (FindElementById(options.id)) {
        RemoveElements(options.id);
    }

    BarElement* element = new BarElement(options.id, options.x, options.y, options.width, options.height, options.value, options.orientation);
    
    PropertyParser::ApplyBarOptions(element, options); // Changed from ApplyElementOptions

    m_Elements.push_back(element);
    UpdateContainerForElement(element, options.containerId);
    
    Redraw();
}

/*
** Add a round line content item to the widget.
*/
void Widget::AddRoundLine(const PropertyParser::RoundLineOptions& options)
{
    if (options.id.empty()) {
        Logging::Log(LogLevel::Error, L"AddRoundLine failed: Element ID cannot be empty.");
        return;
    }

    if (FindElementById(options.id)) {
        RemoveElements(options.id);
    }

    RoundLineElement* element = new RoundLineElement(options.id, options.x, options.y, options.width, options.height, options.value);
    
    PropertyParser::ApplyRoundLineOptions(element, options);

    m_Elements.push_back(element);
    UpdateContainerForElement(element, options.containerId);
    
    Redraw();
}

/*
** Add shapes item to the widget.
*/
void Widget::AddShape(const PropertyParser::ShapeOptions& options)
{
    if (options.id.empty()) {
        Logging::Log(LogLevel::Error, L"AddShape failed: Element ID cannot be empty.");
        return;
    }

    if (FindElementById(options.id)) {
        RemoveElements(options.id);
    }

    ShapeElement* element = nullptr;

    if (options.isCombine) {
        element = new PathShape(options.id, options.x, options.y, options.width, options.height);
    }
    else if (options.shapeType == L"ellipse") {
        element = new EllipseShape(options.id, options.x, options.y, options.width, options.height);
    }
    else if (options.shapeType == L"line") {
        element = new LineShape(options.id, options.x, options.y, options.width, options.height);
    }
    else if (options.shapeType == L"arc") {
        element = new ArcShape(options.id, options.x, options.y, options.width, options.height);
    }
    else if (options.shapeType == L"path") {
        element = new PathShape(options.id, options.x, options.y, options.width, options.height);
    }
    else if (options.shapeType == L"curve") {
        element = new CurveShape(options.id, options.x, options.y, options.width, options.height);
    }
    else {
        element = new RectangleShape(options.id, options.x, options.y, options.width, options.height);
    }

    PropertyParser::ApplyShapeOptions(element, options);

    if (options.isCombine) {
        PathShape* path = static_cast<PathShape*>(element);
        BuildCombinedShapeGeometry(path, options);
    }

    m_Elements.push_back(element);
    UpdateContainerForElement(element, options.containerId);

    Redraw();
}

bool Widget::BuildCombinedShapeGeometry(PathShape* target, const PropertyParser::ShapeOptions& options)
{
    if (!target) return false;

    target->ClearCombinedGeometry();

    if (options.combineBaseId.empty()) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' missing base id.", options.id.c_str());
        return false;
    }

    Element* baseElement = FindElementById(options.combineBaseId);
    ShapeElement* baseShape = dynamic_cast<ShapeElement*>(baseElement);
    if (!baseShape) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' base '%s' is not a shape.", options.id.c_str(), options.combineBaseId.c_str());
        return false;
    }
    if (baseShape == target) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' cannot use itself as base.", options.id.c_str());
        return false;
    }

    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: Direct2D factory unavailable.", options.id.c_str());
        return false;
    }

    Microsoft::WRL::ComPtr<ID2D1Geometry> baseGeometry;
    if (!baseShape->CreateGeometry(factory, baseGeometry) || !baseGeometry) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: base geometry could not be created.", options.id.c_str());
        return false;
    }

    D2D1_MATRIX_3X2_F baseTransform = baseShape->GetRenderTransformMatrix();
    Microsoft::WRL::ComPtr<ID2D1Geometry> combinedGeometry;

    bool baseTransformIsIdentity =
        baseTransform._11 == 1.0f && baseTransform._12 == 0.0f &&
        baseTransform._21 == 0.0f && baseTransform._22 == 1.0f &&
        baseTransform._31 == 0.0f && baseTransform._32 == 0.0f;

    if (baseTransformIsIdentity) {
        combinedGeometry = baseGeometry;
    }
    else {
        Microsoft::WRL::ComPtr<ID2D1TransformedGeometry> transformed;
        if (FAILED(factory->CreateTransformedGeometry(baseGeometry.Get(), &baseTransform, transformed.GetAddressOf()))) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: base transform could not be applied.", options.id.c_str());
            return false;
        }
        combinedGeometry = transformed;
    }

    std::vector<PathShape::CombineOp> resolvedOps;
    resolvedOps.reserve(options.combineOps.size());

    for (const auto& op : options.combineOps) {
        PathShape::CombineOp resolved;
        resolved.id = op.id;
        resolved.mode = op.mode;
        resolved.consume = op.hasConsume ? op.consume : (options.hasCombineConsumeAll ? options.combineConsumeAll : false);
        resolvedOps.push_back(resolved);

        Element* opElement = FindElementById(op.id);
        ShapeElement* opShape = dynamic_cast<ShapeElement*>(opElement);
        if (!opShape) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' cannot combine with '%s' (not a shape).", options.id.c_str(), op.id.c_str());
            return false;
        }
        if (opShape == target) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' cannot combine with itself.", options.id.c_str());
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1Geometry> opGeometry;
        if (!opShape->CreateGeometry(factory, opGeometry) || !opGeometry) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: geometry for '%s' could not be created.", options.id.c_str(), op.id.c_str());
            return false;
        }

        D2D1_MATRIX_3X2_F opTransform = opShape->GetRenderTransformMatrix();

        Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
        if (FAILED(factory->CreatePathGeometry(path.GetAddressOf()))) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: could not create path geometry.", options.id.c_str());
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(path->Open(sink.GetAddressOf()))) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: could not open geometry sink.", options.id.c_str());
            return false;
        }

        HRESULT hr = combinedGeometry->CombineWithGeometry(opGeometry.Get(), op.mode, opTransform, sink.Get());
        sink->Close();
        if (FAILED(hr)) {
            Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: combine operation with '%s' failed.", options.id.c_str(), op.id.c_str());
            return false;
        }

        combinedGeometry = path;
    }

    D2D1_RECT_F bounds = D2D1::RectF();
    HRESULT hr = combinedGeometry->GetBounds(nullptr, &bounds);
    if (FAILED(hr)) {
        Logging::Log(LogLevel::Error, L"Combine shape '%s' failed: could not compute bounds.", options.id.c_str());
        return false;
    }

    bool consumeBase = options.hasCombineConsumeAll ? options.combineConsumeAll : false;
    target->SetCombineData(options.combineBaseId, resolvedOps, consumeBase);
    target->SetCombinedGeometry(combinedGeometry, bounds);

    if (consumeBase) {
        baseShape->AddCombineConsumer();
    }
    for (const auto& resolved : resolvedOps) {
        if (!resolved.consume) continue;
        Element* opElement = FindElementById(resolved.id);
        if (ShapeElement* opShape = dynamic_cast<ShapeElement*>(opElement)) {
            opShape->AddCombineConsumer();
        }
    }

    return true;
}

void Widget::ReleaseCombinedConsumes(PathShape* target)
{
    if (!target || !target->IsCombineShape()) return;

    std::wstring baseId;
    std::vector<PathShape::CombineOp> ops;
    bool consumeBase = false;
    target->GetCombineData(baseId, ops, consumeBase);

    if (consumeBase && !baseId.empty()) {
        if (ShapeElement* baseShape = dynamic_cast<ShapeElement*>(FindElementById(baseId))) {
            baseShape->RemoveCombineConsumer();
        }
    }

    for (const auto& op : ops) {
        if (!op.consume) continue;
        if (ShapeElement* opShape = dynamic_cast<ShapeElement*>(FindElementById(op.id))) {
            opShape->RemoveCombineConsumer();
        }
    }
}

void Widget::UpdateContainerForElement(Element* element, const std::wstring& newContainerId)
{
    if (!element) return;

    Element* currentContainer = element->GetContainer();
    if (newContainerId.empty()) {
        if (currentContainer) {
            currentContainer->RemoveContainerItem(element);
            element->SetContainer(nullptr);
        }
        element->SetContainerId(L"");
        return;
    }

    if (element->GetId() == newContainerId) {
        Logging::Log(LogLevel::Error, L"Container cannot self-reference: %s", newContainerId.c_str());
        return;
    }

    Element* newContainer = FindElementById(newContainerId);
    if (!newContainer) {
        Logging::Log(LogLevel::Error, L"Invalid container: %s", newContainerId.c_str());
        return;
    }

    if (element->IsContainer()) {
        Logging::Log(LogLevel::Error, L"Container cannot be contained: %s", element->GetId().c_str());
        return;
    }

    if (newContainer->IsContained()) {
        Logging::Log(LogLevel::Error, L"Nested containers are not allowed: %s", newContainerId.c_str());
        return;
    }

    if (WouldCreateContainerCycle(element, newContainer)) {
        Logging::Log(LogLevel::Error, L"Container cycle detected for: %s", newContainerId.c_str());
        return;
    }

    if (currentContainer != newContainer) {
        if (currentContainer) currentContainer->RemoveContainerItem(element);
        newContainer->AddContainerItem(element);
        element->SetContainer(newContainer);
    }

    element->SetContainerId(newContainerId);
}

bool Widget::WouldCreateContainerCycle(Element* element, Element* container) const
{
    if (!element || !container) return false;
    if (element == container) return true;

    Element* cursor = container;
    while (cursor) {
        if (cursor == element) return true;
        cursor = cursor->GetContainer();
    }
    return false;
}

void Widget::RenderContainerChildren(Element* container)
{
    if (!container || !container->IsContainer() || !m_pContext) return;

    GfxRect bounds = container->GetBounds();
    D2D1_RECT_F clipRect = D2D1::RectF(
        (float)bounds.X, (float)bounds.Y,
        (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height)
    );

    Microsoft::WRL::ComPtr<ID2D1Layer> layer;
    m_pContext->CreateLayer(layer.GetAddressOf());
    if (!layer) return;

    Microsoft::WRL::ComPtr<ID2D1BitmapBrush> opacityBrush;
    bool hasOpacityMask = false;

    if (bounds.Width > 0 && bounds.Height > 0) {
        Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> maskTarget;
        HRESULT hr = m_pContext->CreateCompatibleRenderTarget(
            D2D1::SizeF((FLOAT)bounds.Width, (FLOAT)bounds.Height),
            maskTarget.GetAddressOf());
        if (SUCCEEDED(hr) && maskTarget) {
            Microsoft::WRL::ComPtr<ID2D1DeviceContext> maskContext;
            maskTarget.As(&maskContext);
            if (maskContext) {
                maskContext->BeginDraw();
                maskContext->Clear(D2D1::ColorF(0, 0, 0, 0));

                D2D1_MATRIX_3X2_F originalTransform;
                maskContext->GetTransform(&originalTransform);
                D2D1_MATRIX_3X2_F translate = D2D1::Matrix3x2F::Translation((FLOAT)-bounds.X, (FLOAT)-bounds.Y);
                maskContext->SetTransform(translate * originalTransform);

                // Render the container itself to build a pixel-alpha mask.
                container->Render(maskContext.Get());

                maskContext->SetTransform(originalTransform);
                if (SUCCEEDED(maskContext->EndDraw())) {
                    Microsoft::WRL::ComPtr<ID2D1Bitmap> maskBitmap;
                    if (SUCCEEDED(maskTarget->GetBitmap(maskBitmap.GetAddressOf())) && maskBitmap) {
                        D2D1_BITMAP_BRUSH_PROPERTIES1 props = D2D1::BitmapBrushProperties1(
                            D2D1_EXTEND_MODE_CLAMP,
                            D2D1_EXTEND_MODE_CLAMP,
                            D2D1_INTERPOLATION_MODE_LINEAR
                        );
                        D2D1_BRUSH_PROPERTIES brushProps = D2D1::BrushProperties(1.0f);
                        Microsoft::WRL::ComPtr<ID2D1BitmapBrush1> brush1;
                        if (SUCCEEDED(m_pContext->CreateBitmapBrush(maskBitmap.Get(), &props, &brushProps, brush1.GetAddressOf()))) {
                            brush1->SetTransform(D2D1::Matrix3x2F::Translation((FLOAT)bounds.X, (FLOAT)bounds.Y));
                            opacityBrush = brush1;
                            hasOpacityMask = true;
                        }
                    }
                }
            }
        }
    }

    if (hasOpacityMask) {
        m_pContext->PushLayer(D2D1::LayerParameters(clipRect, nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(), 1.0f, opacityBrush.Get()), layer.Get());
    }
    else {
        m_pContext->PushLayer(D2D1::LayerParameters(clipRect), layer.Get());
    }

    D2D1_MATRIX_3X2_F originalTransform;
    m_pContext->GetTransform(&originalTransform);
    D2D1_MATRIX_3X2_F translate = D2D1::Matrix3x2F::Translation((float)bounds.X, (float)bounds.Y);
    m_pContext->SetTransform(translate * originalTransform);

    for (Element* child : container->GetContainerItems()) {
        if (!child || !child->IsVisible()) continue;
        child->Render(m_pContext.Get());
        if (child->IsContainer()) {
            RenderContainerChildren(child);
        }
    }

    m_pContext->SetTransform(originalTransform);
    m_pContext->PopLayer();
}

bool Widget::HitTestContainerChildren(Element* container, int x, int y, Element*& outElement)
{
    Element* outActionElement = nullptr;
    Element* outMouseActionElement = nullptr;
    Element* outToolTipElement = nullptr;
    return HitTestContainerChildrenDetailed(
        container,
        x,
        y,
        WM_MOUSEMOVE,
        0,
        outElement,
        outActionElement,
        outMouseActionElement,
        outToolTipElement);
}

bool Widget::HitTestContainerChildrenDetailed(
    Element* container,
    int x,
    int y,
    UINT message,
    WPARAM wParam,
    Element*& outHitElement,
    Element*& outActionElement,
    Element*& outMouseActionElement,
    Element*& outToolTipElement)
{
    if (!container || !container->IsContainer() || !container->IsVisible()) return false;

    GfxRect bounds = container->GetBounds();
    if (x < bounds.X || x >= bounds.X + bounds.Width ||
        y < bounds.Y || y >= bounds.Y + bounds.Height) {
        return false;
    }

    if (ShapeElement* shapeContainer = dynamic_cast<ShapeElement*>(container)) {
        if (!shapeContainer->HitTest(x, y)) {
            return false;
        }
    }

    int localX = x - bounds.X;
    int localY = y - bounds.Y;
    bool foundAny = false;

    const auto& items = container->GetContainerItems();
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        Element* child = *it;
        if (!child || !child->IsVisible()) continue;

        if (child->IsContainer()) {
            if (HitTestContainerChildrenDetailed(
                child,
                localX,
                localY,
                message,
                wParam,
                outHitElement,
                outActionElement,
                outMouseActionElement,
                outToolTipElement)) {
                foundAny = true;
            }
        }

        if (!child->HitTest(localX, localY)) {
            continue;
        }

        foundAny = true;
        if (!outHitElement) outHitElement = child;
        if (!outActionElement && child->HasAction(message, wParam)) outActionElement = child;
        if (!outMouseActionElement && child->HasMouseAction()) outMouseActionElement = child;
        if (!outToolTipElement && child->HasToolTip()) outToolTipElement = child;
    }

    return foundAny;
}

/*
** Update properties of an existing element.
*/
void Widget::ApplyParsedPropertiesToElement(Element* element, duk_context* ctx)
{
    if (!element) return;

    if (element->GetType() == ELEMENT_TEXT) {
        PropertyParser::TextOptions options;
        PropertyParser::PreFillTextOptions(options, static_cast<TextElement*>(element));
        PropertyParser::ParseTextOptions(ctx, options);
        PropertyParser::ApplyTextOptions(static_cast<TextElement*>(element), options);
        UpdateContainerForElement(element, options.containerId);
    } else if (element->GetType() == ELEMENT_IMAGE) {
        PropertyParser::ImageOptions options;
        PropertyParser::PreFillImageOptions(options, static_cast<ImageElement*>(element));
        PropertyParser::ParseImageOptions(ctx, options);
        PropertyParser::ApplyImageOptions(static_cast<ImageElement*>(element), options);
        UpdateContainerForElement(element, options.containerId);
    } else if (element->GetType() == ELEMENT_BAR) {
        PropertyParser::BarOptions options;
        PropertyParser::PreFillBarOptions(options, static_cast<BarElement*>(element));
        PropertyParser::ParseBarOptions(ctx, options);
        PropertyParser::ApplyBarOptions(static_cast<BarElement*>(element), options);
        UpdateContainerForElement(element, options.containerId);
    } else if (element->GetType() == ELEMENT_ROUNDLINE) {
        PropertyParser::RoundLineOptions options;
        PropertyParser::PreFillRoundLineOptions(options, static_cast<RoundLineElement*>(element));
        PropertyParser::ParseRoundLineOptions(ctx, options);
        PropertyParser::ApplyRoundLineOptions(static_cast<RoundLineElement*>(element), options);
        UpdateContainerForElement(element, options.containerId);
    } else if (element->GetType() == ELEMENT_SHAPE) {
        PropertyParser::ShapeOptions options;
        PropertyParser::PreFillShapeOptions(options, static_cast<ShapeElement*>(element));
        PropertyParser::ParseShapeOptions(ctx, options);
        PropertyParser::ApplyShapeOptions(static_cast<ShapeElement*>(element), options);
        UpdateContainerForElement(element, options.containerId);

        PathShape* path = dynamic_cast<PathShape*>(element);
        if (path && (options.isCombine || path->IsCombineShape())) {
            if (options.isCombine) {
                ReleaseCombinedConsumes(path);
                BuildCombinedShapeGeometry(path, options);
            }
        }
    }
}

void Widget::SetElementProperties(const std::wstring& id, duk_context* ctx)
{
    Element* element = FindElementById(id);
    if (!element) return;

    ApplyParsedPropertiesToElement(element, ctx);
    
    if (!m_IsBatchUpdating) {
        Redraw();
    }
}

void Widget::SetGroupProperties(const std::wstring& group, duk_context* ctx)
{
    if (group.empty() || !ctx) return;

    bool changed = false;
    for (Element* element : m_Elements) {
        if (!element) continue;
        if (element->GetGroupId() != group) continue;
        ApplyParsedPropertiesToElement(element, ctx);
        changed = true;
    }

    if (changed && !m_IsBatchUpdating) {
        Redraw();
    }
}

void Widget::RemoveElementsByGroup(const std::wstring& group)
{
    if (group.empty()) return;

    std::vector<std::wstring> ids;
    ids.reserve(m_Elements.size());
    for (Element* element : m_Elements) {
        if (!element) continue;
        if (element->GetGroupId() == group) {
            ids.push_back(element->GetId());
        }
    }

    if (!ids.empty()) {
        RemoveElements(ids);
    }
}

/*
** Remove one or more content items by ID.
** If id is empty, clears all content.
*/
bool Widget::RemoveElements(const std::wstring& id)
{
    if (id.empty())
    {
        for (auto* el : m_Elements) {
            if (PathShape* path = dynamic_cast<PathShape*>(el)) {
                ReleaseCombinedConsumes(path);
            }
            if (el && el->IsContainer()) {
                for (Element* child : el->GetContainerItems()) {
                    if (!child) continue;
                    child->SetContainer(nullptr);
                    child->SetContainerId(L"");
                }
                el->ClearContainerItems();
            }
            UpdateContainerForElement(el, L"");
            delete el;
        }
        m_Elements.clear();
        m_MouseOverElement = nullptr;
        m_TooltipElement = nullptr;
        Redraw();
        return true;
    }

    for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
    {
        if ((*it)->GetId() == id)
        {
            if (*it == m_MouseOverElement) m_MouseOverElement = nullptr;
            if (*it == m_TooltipElement) m_TooltipElement = nullptr;
            if (PathShape* path = dynamic_cast<PathShape*>(*it)) {
                ReleaseCombinedConsumes(path);
            }
            if (*it && (*it)->IsContainer()) {
                for (Element* child : (*it)->GetContainerItems()) {
                    if (!child) continue;
                    child->SetContainer(nullptr);
                    child->SetContainerId(L"");
                }
                (*it)->ClearContainerItems();
            }
            UpdateContainerForElement(*it, L"");
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
                if (*it == m_TooltipElement) m_TooltipElement = nullptr;
                if (PathShape* path = dynamic_cast<PathShape*>(*it)) {
                    ReleaseCombinedConsumes(path);
                }
                if (*it && (*it)->IsContainer()) {
                    for (Element* child : (*it)->GetContainerItems()) {
                        if (!child) continue;
                        child->SetContainer(nullptr);
                        child->SetContainerId(L"");
                    }
                    (*it)->ClearContainerItems();
                }
                UpdateContainerForElement(*it, L"");
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
** Set the entire custom context menu.
*/
void Widget::SetContextMenu(const std::vector<MenuItem>& menu)
{
    m_ContextMenu = menu;
}

/*
** Clear all custom context menu items.
*/
void Widget::ClearContextMenu()
{
    m_ContextMenu.clear();
}

/*
** Redraw the widget window to reflect content changes.
*/
void Widget::Redraw()
{
    if (!m_IsBatchUpdating) {
        UpdateLayeredWindowContent();
    }
}


/*
** Update the layered window content using UpdateLayeredWindow.
** Draws all content to a memory DC and updates the window.
*/
void Widget::UpdateLayeredWindowContent()
{
    if (!m_hWnd) return;

    int calcW = m_Options.width;
    int calcH = m_Options.height;

    bool shouldCalcW = !m_Options.m_WDefined;
    bool shouldCalcH = !m_Options.m_HDefined;

    if (shouldCalcW || shouldCalcH)
    {
        int maxX = 0;
        int maxY = 0;
        for (Element* element : m_Elements)
        {
            if (!element || element->IsContained()) continue;
            GfxRect bounds = element->GetBounds();
            maxX = (std::max)(maxX, bounds.X + bounds.Width);
            maxY = (std::max)(maxY, bounds.Y + bounds.Height);
            
        //     Logging::Log(LogLevel::Debug, L"Widget::UpdateSize: Element '%s' contributing to bounds: [X:%d, Y:%d, W:%d, H:%d] -> TargetMax: [%d, %d]", 
        //         element->GetId().c_str(), bounds.X, bounds.Y, bounds.Width, bounds.Height, maxX, maxY);
        }
        
        if (shouldCalcW) calcW = maxX;
        if (shouldCalcH) calcH = maxY;
        
        // Ensure at least 1x1
        if (calcW <= 0) calcW = 1;
        if (calcH <= 0) calcH = 1;
        
        // If size changed, update window and options
        if (calcW != m_Options.width || calcH != m_Options.height)
        {
            // Logging::Log(LogLevel::Info, L"Widget::UpdateSize: Resizing window '%s' from %dx%d to %dx%d", 
            //     m_Options.id.c_str(), m_Options.width, m_Options.height, calcW, calcH);
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
    bmi.bmiHeader.biHeight = -h; // Top-down DIB
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

    // Draw Direct2D
    {
        if (!m_pContext)
        {
            bool useHW = Settings::GetGlobalBool("useHardwareAcceleration", false);
            D2D1_RENDER_TARGET_TYPE rtType = useHW ? D2D1_RENDER_TARGET_TYPE_DEFAULT : D2D1_RENDER_TARGET_TYPE_SOFTWARE;
            
            // Logging::Log(LogLevel::Info, L"Creating Direct2D Context: Hardware Acceleration = %s", useHW ? L"ON" : L"OFF");

            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                rtType,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                0, 0,
                D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE
            );

            Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> pDCRT;
            HRESULT hr = Direct2D::GetFactory()->CreateDCRenderTarget(&props, pDCRT.GetAddressOf());
            if (SUCCEEDED(hr)) {
                hr = pDCRT.As<ID2D1DeviceContext>(&m_pContext);
            }
            
            if (FAILED(hr)) {
                Logging::Log(LogLevel::Error, L"Failed to create D2D Context (0x%08X)", hr);
            }
        }

        if (m_pContext)
        {
            Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> pDCRT;
            if (SUCCEEDED(m_pContext.As(&pDCRT)))
            {
                RECT renderRect = { 0, 0, w, h };
                HRESULT hr = pDCRT->BindDC(hdcMem, &renderRect);
                if (FAILED(hr)) {
                    Logging::Log(LogLevel::Error, L"BindDC failed (0x%08X)", hr);
                }
            }

            m_pContext->BeginDraw();
            m_pContext->Clear(D2D1::ColorF(0, 0, 0, 0));

            // Draw Background
            D2D1_RECT_F backRect = D2D1::RectF(0, 0, (float)w, (float)h);
            Microsoft::WRL::ComPtr<ID2D1Brush> pBackBrush;
            Direct2D::CreateBrushFromGradientOrColor(
                m_pContext.Get(),
                backRect,
                &m_Options.bgGradient,
                m_Options.color,
                m_Options.bgAlpha / 255.0f,
                pBackBrush.GetAddressOf()
            );

            if (pBackBrush)
            {
                m_pContext->FillRectangle(backRect, pBackBrush.Get());
            }

            // Draw Elements
            for (Element* element : m_Elements)
            {
                if (!element->IsVisible()) continue;
                if (element->IsContained()) continue;
                if (!element->IsContainer()) {
                    element->Render(m_pContext.Get());
                }
                if (element->IsContainer()) {
                    RenderContainerChildren(element);
                }
            }

            HRESULT hr = m_pContext->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET) {
                m_pContext.Reset();
                Logging::Log(LogLevel::Error, L"D2D Device lost, resetting RenderContext");
            }
            else if (FAILED(hr)) {
                Logging::Log(LogLevel::Error, L"D2D EndDraw failed (0x%08X)", hr);
            }
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

    BOOL success = UpdateLayeredWindow(m_hWnd, hdcScreen, &pptDst, &size, hdcMem, &pptSrc, 0, &bf, ULW_ALPHA);
    if (!success) {
        DWORD err = GetLastError();
        Logging::Log(LogLevel::Error, L"UpdateLayeredWindow failed for widget %s (Error: %d). Size: %dx%d, Pos: %d,%d", 
                      m_Options.id.c_str(), err, w, h, pptDst.x, pptDst.y);
    }

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
    bool justEnteredWidget = false;

    if (message == WM_MOUSEMOVE)
    {
        if (!m_IsMouseOverWidget) {
            m_IsMouseOverWidget = true;
            justEnteredWidget = true;
        }
        m_Tooltip.Move();
    }

    // For mouse wheel, coordinates are screen relative
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        POINT pt = { x, y };
        ScreenToClient(m_hWnd, &pt);
        x = pt.x;
        y = pt.y;
    }

    {
        JSApi::MouseEventData widgetEventData;
        widgetEventData.clientX = x;
        widgetEventData.clientY = y;
        widgetEventData.offsetX = x;
        widgetEventData.offsetY = y;

        POINT screenPt = { x, y };
        ClientToScreen(m_hWnd, &screenPt);
        widgetEventData.screenX = screenPt.x;
        widgetEventData.screenY = screenPt.y;

        RECT clientRect = {};
        if (GetClientRect(m_hWnd, &clientRect)) {
            const int clientW = clientRect.right - clientRect.left;
            const int clientH = clientRect.bottom - clientRect.top;
            if (clientW > 0) {
                widgetEventData.offsetXPercent = (int)(((widgetEventData.offsetX + 1) / (double)clientW) * 100.0);
            }
            if (clientH > 0) {
                widgetEventData.offsetYPercent = (int)(((widgetEventData.offsetY + 1) / (double)clientH) * 100.0);
            }
        }

        if (message == WM_MOUSEMOVE) {
            if (justEnteredWidget) {
                JSApi::TriggerWidgetEvent(this, "mouseOver", &widgetEventData);
            }
            JSApi::TriggerWidgetEvent(this, "mouseMove", &widgetEventData);
        } else if (
            message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN
        ) {
            JSApi::TriggerWidgetEvent(this, "mouseDown", &widgetEventData);
        } else if (
            message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP || message == WM_XBUTTONUP
        ) {
            JSApi::TriggerWidgetEvent(this, "mouseUp", &widgetEventData);
        }
    }

    // Find element at cursor (Front to Back)
    Element* hitElement = nullptr;
    Element* actionElement = nullptr;
    Element* mouseActionElement = nullptr;
    Element* toolTipElement = nullptr;

    for (auto it = m_Elements.rbegin(); it != m_Elements.rend(); ++it)
    {
        Element* el = *it;
        if (!el) continue;
        if (!el->IsVisible()) continue;
        if (el->IsContained()) continue;

        if (el->IsContainer()) {
            Element* childHit = nullptr;
            Element* childAction = nullptr;
            Element* childMouseAction = nullptr;
            Element* childToolTip = nullptr;
            if (HitTestContainerChildrenDetailed(el, x, y, message, wParam, childHit, childAction, childMouseAction, childToolTip)) {
                if (!hitElement) hitElement = childHit;
                if (!actionElement) actionElement = childAction;
                if (!mouseActionElement) mouseActionElement = childMouseAction;
                if (!toolTipElement) toolTipElement = childToolTip;
            }
        }

        if (el->HitTest(x, y)) {
            if (!hitElement) hitElement = el;
            if (!actionElement && el->HasAction(message, wParam)) actionElement = el;
            if (!mouseActionElement && el->HasMouseAction()) mouseActionElement = el;
            if (!toolTipElement && el->HasToolTip()) toolTipElement = el;
        }

        if (hitElement && actionElement && mouseActionElement && toolTipElement) break;
    }

    // Handle Hover/Leave logic.
    // Prefer the element that can handle hover callbacks; this avoids
    // non-interactive overlays (for example text labels) stealing hover
    // from the interactive element underneath.
    if (message == WM_MOUSEMOVE)
    {
        Element* hoverElement = actionElement ? actionElement : (mouseActionElement ? mouseActionElement : hitElement);
        Element* nextToolTipElement = toolTipElement ? toolTipElement : hoverElement;

        if (hoverElement != m_MouseOverElement)
        {

            if (m_MouseOverElement)
            {

                m_MouseOverElement->m_IsMouseOver = false;
                int leaveId = m_MouseOverElement->m_OnMouseLeaveCallbackId;
                if (leaveId != -1)
                    JSApi::CallEventCallback(leaveId);
                
                // If callback cleared the elements, m_MouseOverElement is now invalid
                if (m_Elements.empty()) {
                    m_MouseOverElement = nullptr;
                    m_TooltipElement = nullptr;
                    return true;
                }
            }

            if (hoverElement)
            {

                hoverElement->m_IsMouseOver = true;
                int overId = hoverElement->m_OnMouseOverCallbackId;
                if (overId != -1)
                    JSApi::CallEventCallback(overId);

                // If callback cleared the elements, hitElement is now invalid
                if (m_Elements.empty()) {
                    m_MouseOverElement = nullptr;
                    m_TooltipElement = nullptr;
                    return true;
                }
            }
            m_MouseOverElement = hoverElement;
            
            // Refresh cursor when element under mouse changes as it might have different action state
            PostMessage(m_hWnd, WM_SETCURSOR, (WPARAM)m_hWnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }

        if (nextToolTipElement != m_TooltipElement) {
            m_Tooltip.Update(nextToolTipElement);
            m_TooltipElement = nextToolTipElement;

            // Start timer to periodically check if tooltip should be hidden (e.g., another window covers)
            if (m_Tooltip.IsActive()) {
                SetTimer(m_hWnd, TIMER_TOOLTIP, 100, nullptr);  // Check every 100ms
            } else {
                KillTimer(m_hWnd, TIMER_TOOLTIP);
            }
        }
        
        // Ensure we track mouse leave window events
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        TrackMouseEvent(&tme);
        
        handled = true; 
    }
    else if (message == WM_MOUSELEAVE)
    {
        if (m_IsMouseOverWidget) {
            m_IsMouseOverWidget = false;
            JSApi::MouseEventData leaveEventData;
            POINT screenPt = {};
            if (GetCursorPos(&screenPt)) {
                POINT clientPt = screenPt;
                ScreenToClient(m_hWnd, &clientPt);
                leaveEventData.clientX = clientPt.x;
                leaveEventData.clientY = clientPt.y;
                leaveEventData.offsetX = clientPt.x;
                leaveEventData.offsetY = clientPt.y;
                leaveEventData.screenX = screenPt.x;
                leaveEventData.screenY = screenPt.y;

                RECT clientRect = {};
                if (GetClientRect(m_hWnd, &clientRect)) {
                    const int clientW = clientRect.right - clientRect.left;
                    const int clientH = clientRect.bottom - clientRect.top;
                    if (clientW > 0) {
                        leaveEventData.offsetXPercent = (int)(((leaveEventData.offsetX + 1) / (double)clientW) * 100.0);
                    }
                    if (clientH > 0) {
                        leaveEventData.offsetYPercent = (int)(((leaveEventData.offsetY + 1) / (double)clientH) * 100.0);
                    }
                }
            }
            JSApi::TriggerWidgetEvent(this, "mouseLeave", &leaveEventData);
        }
        if (m_MouseOverElement)
        {

             m_MouseOverElement->m_IsMouseOver = false;
             int leaveId = m_MouseOverElement->m_OnMouseLeaveCallbackId;
             if (leaveId != -1)
                 JSApi::CallEventCallback(leaveId);
             m_MouseOverElement = nullptr;
             m_TooltipElement = nullptr;
        }
        // Tooltip Update and kill timer
        m_Tooltip.Update(nullptr);
        KillTimer(m_hWnd, TIMER_TOOLTIP);
        handled = true;
    }

    // Dispatch Actions
    if (actionElement)
    {
        int actionId = -1;

        switch (message)
        {
        case WM_LBUTTONUP:     actionId = actionElement->m_OnLeftMouseUpCallbackId; break;
        case WM_LBUTTONDOWN:   actionId = actionElement->m_OnLeftMouseDownCallbackId; break;
        case WM_LBUTTONDBLCLK: actionId = actionElement->m_OnLeftDoubleClickCallbackId; break;
        case WM_RBUTTONUP:     actionId = actionElement->m_OnRightMouseUpCallbackId; break;
        case WM_RBUTTONDOWN:   actionId = actionElement->m_OnRightMouseDownCallbackId; break;
        case WM_RBUTTONDBLCLK: actionId = actionElement->m_OnRightDoubleClickCallbackId; break;
        case WM_MBUTTONUP:     actionId = actionElement->m_OnMiddleMouseUpCallbackId; break;
        case WM_MBUTTONDOWN:   actionId = actionElement->m_OnMiddleMouseDownCallbackId; break;
        case WM_MBUTTONDBLCLK: actionId = actionElement->m_OnMiddleDoubleClickCallbackId; break;
        case WM_XBUTTONUP:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) { actionId = actionElement->m_OnX1MouseUpCallbackId; }
            else { actionId = actionElement->m_OnX2MouseUpCallbackId; }
            break;
        case WM_XBUTTONDOWN:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) { actionId = actionElement->m_OnX1MouseDownCallbackId; }
            else { actionId = actionElement->m_OnX2MouseDownCallbackId; }
            break;
        case WM_XBUTTONDBLCLK:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) { actionId = actionElement->m_OnX1DoubleClickCallbackId; }
            else { actionId = actionElement->m_OnX2DoubleClickCallbackId; }
            break;
        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) { actionId = actionElement->m_OnScrollUpCallbackId; }
            else { actionId = actionElement->m_OnScrollDownCallbackId; }
            break;
        case WM_MOUSEHWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) { actionId = actionElement->m_OnScrollRightCallbackId; }
            else { actionId = actionElement->m_OnScrollLeftCallbackId; }
            break;
        }

        if (actionId != -1)
        {
             JSApi::MouseEventData eventData;
             eventData.clientX = x;
             eventData.clientY = y;

             POINT screenPt = { x, y };
             ClientToScreen(m_hWnd, &screenPt);
             eventData.screenX = screenPt.x;
             eventData.screenY = screenPt.y;

             const int elementX = actionElement->GetX();
             const int elementY = actionElement->GetY();
             eventData.offsetX = x - elementX;
             eventData.offsetY = y - elementY;

             const int elementW = actionElement->GetWidth();
             const int elementH = actionElement->GetHeight();
             if (elementW > 0) {
                 eventData.offsetXPercent = (int)(((eventData.offsetX + 1) / (double)elementW) * 100.0);
             }
             if (elementH > 0) {
                 eventData.offsetYPercent = (int)(((eventData.offsetY + 1) / (double)elementH) * 100.0);
             }

             // Execute function callback with mouse position aliases.
             JSApi::CallEventCallback(actionId, this, &eventData);
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
    if (m_ContextMenuDisabled) return;

    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    
    MenuUtils::BuildMenu(hMenu, m_ContextMenu);

    if (m_ShowDefaultContextMenuItems)
    {
        if (!m_ContextMenu.empty())
        {
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        }
        
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenu(hSubMenu, MF_STRING, 1001, L"Refresh");
        AppendMenu(hSubMenu, MF_STRING, 1003, L"Exit");
        
        std::wstring appTitle = PathUtils::GetProductName();
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, appTitle.c_str());
    }

    SetForegroundWindow(m_hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd >= 2000)
    {
        JSApi::OnWidgetContextCommand(m_Options.id, cmd);
    }
    else if (cmd == 1001)
    {
        JSApi::Reload();
    }
    else if (cmd == 1002)
    {
        DestroyWindow(m_hWnd);
    }
    else if (cmd == 1003)
    {
        PostQuitMessage(0);
    }
}

void Widget::BeginUpdate()
{
    m_IsBatchUpdating = true;
}

void Widget::EndUpdate()
{
    m_IsBatchUpdating = false;
    Redraw();
}


