#include "Widget.h"
#include "Logging.h"
#include <vector>
#include <windowsx.h>
#include <algorithm>

#define WIDGET_CLASS_NAME L"NovadeskWidget"
#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)
#define SNAP_DISTANCE 10

extern std::vector<Widget*> widgets; // Defined in Novadesk.cpp

Widget::Widget(const WidgetOptions& options) 
    : m_hWnd(nullptr), m_Options(options), m_WindowZPosition(options.zPos)
{
}

Widget::~Widget()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
    }
}

bool Widget::Create()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(m_Options.color);
    wcex.lpszClassName = WIDGET_CLASS_NAME;

    RegisterClassExW(&wcex);

    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    if (m_Options.clickThrough)
    {
        dwExStyle |= WS_EX_TRANSPARENT;
    }

    m_hWnd = CreateWindowExW(
        dwExStyle,
        WIDGET_CLASS_NAME,
        L"Novadesk Widget",
        WS_POPUP,
        CW_USEDEFAULT, 0, m_Options.width, m_Options.height,
        nullptr, nullptr, hInstance, this);

    if (!m_hWnd) return false;

    // Set transparency
    SetLayeredWindowAttributes(m_hWnd, 0, m_Options.alpha, LWA_ALPHA);

    // Initial Z-position
    ChangeZPos(m_WindowZPosition);

    return true;
}

void Widget::Show()
{
    if (m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hWnd);
    }
}

void Widget::ChangeZPos(ZPOSITION zPos, bool all)
{
    HWND winPos = HWND_NOTOPMOST;
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
        // Rainmeter checks IsNormalStayDesktop here, we default to break (HWND_NOTOPMOST)
        break;

    case ZPOSITION_ONDESKTOP:
        if (System::GetShowDesktop())
        {
            winPos = System::GetHelperWindow();
            if (!all)
            {
                // Find the backmost topmost window above the helper
                while (HWND prev = ::GetNextWindow(winPos, GW_HWNDPREV))
                {
                    if (GetWindowLongPtr(prev, GWL_EXSTYLE) & WS_EX_TOPMOST)
                    {
                        if (SetWindowPos(m_hWnd, prev, 0, 0, 0, 0, ZPOS_FLAGS))
                        {
                            return;
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
}

Widget* Widget::GetWidgetFromHWND(HWND hWnd)
{
    for (auto w : widgets)
    {
        if (w->m_hWnd == hWnd) return w;
    }
    return nullptr;
}

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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // Optionally draw a border or text for debugging
            EndPaint(hWnd, &ps);
            return 0;
        }
    case WM_LBUTTONDOWN:
        if (widget && widget->m_Options.draggable)
        {
            Logging::Log(LogLevel::Debug, L"WM_LBUTTONDOWN: Starting drag");
            ReleaseCapture();
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCHITTEST:
        {
            LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
            if (widget && widget->m_Options.draggable)
            {
                // We return HTCAPTION even if it was HTCLIENT
                if (hit == HTCLIENT) hit = HTCAPTION;
            }
            return hit;
        }
    case WM_WINDOWPOSCHANGING:
        if (widget)
        {
            LPWINDOWPOS wp = (LPWINDOWPOS)lParam;
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

                // Keep on screen
                if (widget->m_Options.keepOnScreen)
                {
                    const auto& monitors = System::GetMultiMonitorInfo().monitors;
                    RECT windowRect = { wp->x, wp->y, wp->x + widget->m_Options.width, wp->y + widget->m_Options.height };
                    const RECT* targetWork = nullptr;

                    // Find containing monitor (or primary)
                    for (const auto& mon : monitors)
                    {
                        if (!mon.active) continue;
                        if (PtInRect(&mon.screen, { wp->x + widget->m_Options.width / 2, wp->y + widget->m_Options.height / 2 }))
                        {
                            targetWork = &mon.work;
                            break;
                        }
                    }

                    if (targetWork)
                    {
                        wp->x = (std::max)((int)targetWork->left, (std::min)(wp->x, (int)targetWork->right - widget->m_Options.width));
                        wp->y = (std::max)((int)targetWork->top, (std::min)(wp->y, (int)targetWork->bottom - widget->m_Options.height));
                    }
                }
            }
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
