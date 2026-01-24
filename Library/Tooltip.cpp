/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Tooltip.h"
#include "Element.h"
#include "Logging.h"
#include <algorithm>

Tooltip::Tooltip()
{
}

Tooltip::~Tooltip()
{
    Destroy();
}

bool Tooltip::Initialize(HWND parentHWnd, HINSTANCE hInstance)
{
    m_ParentHWnd = parentHWnd;

    // Create standard tooltip control
    m_ToolTipHWnd = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        parentHWnd, nullptr, hInstance, nullptr);

    // Create balloon tooltip control
    m_ToolTipBalloonHWnd = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        parentHWnd, nullptr, hInstance, nullptr);

    InitializeToolTip(m_ToolTipHWnd);
    InitializeToolTip(m_ToolTipBalloonHWnd);

    if (m_ToolTipHWnd || m_ToolTipBalloonHWnd)
    {
        // Logging::Log(LogLevel::Debug, L"Tooltip controls created successfully");
        return true;
    }

    return false;
}

void Tooltip::InitializeToolTip(HWND hwnd)
{
    if (!hwnd) return;

    TOOLINFO ti = { sizeof(TOOLINFO) };
    ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd = m_ParentHWnd;
    ti.uId = 0;
    ti.lpszText = nullptr;

    SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
    SendMessage(hwnd, TTM_SETDELAYTIME, TTDT_INITIAL, 0);

    // Ensure it's topmost without affecting owner Z-order
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void Tooltip::Update(Element* element)
{
    // Deactivate current tooltip if any
    DeactivateCurrent();

    if (element && element->HasToolTip())
    {
        Logging::Log(LogLevel::Debug, L"Tooltip::Update - Showing tooltip for element: %s", element->GetId().c_str());

        // Choose which tooltip control to use
        HWND targetTT = element->GetToolTipBalloon() ? m_ToolTipBalloonHWnd : m_ToolTipHWnd;
        if (!targetTT) return;

        m_ActiveToolTipHWnd = targetTT;

        TOOLINFO ti = { sizeof(TOOLINFO) };
        ti.hwnd = m_ParentHWnd;
        ti.uId = 0;

        // Logging::Log(LogLevel::Debug, L"UpdateToolTip: id=%s, text=%s, balloon=%d",
        //     element->GetId().c_str(), element->GetToolTipText().c_str(), element->GetToolTipBalloon());

        // Update Text
        ti.lpszText = (LPWSTR)element->GetToolTipText().c_str();
        SendMessage(m_ActiveToolTipHWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

        // Update Title and Icon
        HICON hIcon = nullptr;
        std::wstring tipIcon = element->GetToolTipIcon();

        if (tipIcon == L"info") hIcon = (HICON)TTI_INFO;
        else if (tipIcon == L"error") hIcon = (HICON)TTI_ERROR;
        else if (tipIcon == L"warning") hIcon = (HICON)TTI_WARNING;

        SendMessage(m_ActiveToolTipHWnd, TTM_SETTITLE, (WPARAM)hIcon, (LPARAM)element->GetToolTipTitle().c_str());
        
        // Set max width and height
        int maxWidth = element->GetToolTipMaxWidth() > 0 ? element->GetToolTipMaxWidth() : 1000;
        SendMessage(m_ActiveToolTipHWnd, TTM_SETMAXTIPWIDTH, 0, maxWidth);
        
        // Note: TTM_SETMAXHEIGHT doesn't exist in standard tooltip API
        // Height is automatically calculated based on content and width

        // Position and Show
        Move();

        SendMessage(m_ActiveToolTipHWnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
    }
}

void Tooltip::Move()
{
    if (m_ActiveToolTipHWnd)
    {
        POINT pt;
        GetCursorPos(&pt);
        
        // Check if another window is covering our widget at the cursor position
        HWND hwndUnderCursor = WindowFromPoint(pt);
        
        if (hwndUnderCursor != m_ParentHWnd && hwndUnderCursor != m_ActiveToolTipHWnd)
        {
            // Another window is on top at this position, hide tooltip

            DeactivateCurrent();
            return;
        }
        
        DWORD currentTime = GetTickCount();
        if (currentTime - m_LastMoveTime < 200)
        {
            return;
        }
        m_LastMoveTime = currentTime;
        
        SendMessage(m_ActiveToolTipHWnd, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x + 15, pt.y + 15));
    }
}

void Tooltip::DeactivateCurrent()
{
    if (m_ActiveToolTipHWnd)
    {

        TOOLINFO ti = { sizeof(TOOLINFO) };
        ti.hwnd = m_ParentHWnd;
        ti.uId = 0;
        SendMessage(m_ActiveToolTipHWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
        m_ActiveToolTipHWnd = nullptr;
    }
}

void Tooltip::CheckVisibility()
{
    if (!m_ActiveToolTipHWnd) return;
    
    POINT pt;
    GetCursorPos(&pt);
    
    // Check if another window is covering our widget at the cursor position
    HWND hwndUnderCursor = WindowFromPoint(pt);
    
    if (hwndUnderCursor != m_ParentHWnd && hwndUnderCursor != m_ActiveToolTipHWnd && GetParent(hwndUnderCursor) != m_ParentHWnd)
    {

        DeactivateCurrent();
    }
}

void Tooltip::Destroy()
{
    DeactivateCurrent();

    if (m_ToolTipHWnd)
    {
        DestroyWindow(m_ToolTipHWnd);
        m_ToolTipHWnd = nullptr;
    }
    if (m_ToolTipBalloonHWnd)
    {
        DestroyWindow(m_ToolTipBalloonHWnd);
        m_ToolTipBalloonHWnd = nullptr;
    }
}
