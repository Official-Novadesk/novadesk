/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Tooltip.h"
#include "Element.h"
#include "../shared/Logging.h"
#include <algorithm>
#include <windowsx.h>

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

    m_ToolTipHWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    m_ToolTipBalloonHWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    InitializeToolTip(m_ToolTipHWnd);
    InitializeToolTip(m_ToolTipBalloonHWnd);

    if (m_ToolTipHWnd || m_ToolTipBalloonHWnd)
    {
        return true;
    }

    Logging::Log(LogLevel::Error, L"Tooltip::Initialize failed to create tooltip controls");
    return false;
}

void Tooltip::InitializeToolTip(HWND hwnd)
{
    if (!hwnd) return;

    TOOLINFOW ti = { 0 };
    ti.uFlags = TTF_TRACK | TTF_ABSOLUTE | TTF_TRANSPARENT;
    ti.hwnd = m_ParentHWnd;
    ti.uId = 0;
    ti.lpszText = (LPWSTR)L" ";
    ti.rect = { 0, 0, 0, 0 };

    UINT sizes[] = { sizeof(TOOLINFOW), 44 , 48  };
    BOOL added = FALSE;
    UINT appliedSize = 0;

    for (UINT size : sizes)
    {
        ti.cbSize = size;
        if (SendMessageW(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&ti))
        {
            added = TRUE;
            appliedSize = size;
            m_ToolInfoSize = size;
            break;
        }
    }

    DWORD lastError = 0;
    if (!added) lastError = GetLastError();

    SendMessageW(hwnd, TTM_SETDELAYTIME, TTDT_INITIAL, 0);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void Tooltip::Update(Element* element)
{
    if (element && element->HasToolTip())
    {

        HWND targetTT = element->GetToolTipBalloon() ? m_ToolTipBalloonHWnd : m_ToolTipHWnd;
        if (!targetTT) return;

        m_ActiveToolTipHWnd = targetTT;

        TOOLINFOW ti = { 0 };
        ti.cbSize = m_ToolInfoSize;
        ti.hwnd = m_ParentHWnd;
        ti.uId = 0;

        ti.lpszText = (LPWSTR)element->GetToolTipText().c_str();
        SendMessageW(m_ActiveToolTipHWnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);

        HICON hIcon = nullptr;
        std::wstring tipIcon = element->GetToolTipIcon();

        if (tipIcon == L"info") hIcon = (HICON)TTI_INFO;
        else if (tipIcon == L"error") hIcon = (HICON)TTI_ERROR;
        else if (tipIcon == L"warning") hIcon = (HICON)TTI_WARNING;

        SendMessageW(m_ActiveToolTipHWnd, TTM_SETTITLEW, (WPARAM)hIcon, (LPARAM)element->GetToolTipTitle().c_str());

        int maxWidth = element->GetToolTipMaxWidth() > 0 ? element->GetToolTipMaxWidth() : 1000;
        SendMessageW(m_ActiveToolTipHWnd, TTM_SETMAXTIPWIDTH, 0, maxWidth);

        POINT pt;
        if (GetCursorPos(&pt))
        {
            SendMessageW(targetTT, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x + 20, pt.y + 20));
            m_LastPos = pt;
        }

        m_IsMovePending = false; 

        BOOL visible = IsWindowVisible(targetTT);

        LRESULT activated = 0;

        if (m_ActiveToolTipHWnd != targetTT || !visible)
        {
            activated = SendMessageW(targetTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
            ShowWindow(targetTT, SW_SHOWNA);
        }

        m_ActiveToolTipHWnd = targetTT;
    }
    else
    {
        if (m_ToolTipHWnd) {
            TOOLINFOW ti = { 0 };
            ti.cbSize = m_ToolInfoSize;
            ti.hwnd = m_ParentHWnd;
            ti.uId = 0;
            SendMessageW(m_ToolTipHWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
        }
        if (m_ToolTipBalloonHWnd) {
            TOOLINFOW ti = { 0 };
            ti.cbSize = m_ToolInfoSize;
            ti.hwnd = m_ParentHWnd;
            ti.uId = 0;
            SendMessageW(m_ToolTipBalloonHWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
        }
        m_ActiveToolTipHWnd = nullptr;
    }
}

void Tooltip::Move()
{
    if (m_ActiveToolTipHWnd)
    {
        POINT pt;
        GetCursorPos(&pt);

        int dx = pt.x - m_LastPos.x;
        int dy = pt.y - m_LastPos.y;
        DWORD now = GetTickCount();

        if (dx * dx + dy * dy >= 10)
        {
            if (!m_IsMovePending)
            {
                m_IsMovePending = true;
                m_PendingPos = pt;
                m_PendingMoveTime = now;
            }
            else
            {

                if (now - m_PendingMoveTime > 50)
                {
                    SendMessageW(m_ActiveToolTipHWnd, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x + 20, pt.y + 20));
                    m_LastPos = pt;
                    m_IsMovePending = false;
                }
            }
        }
        else
        {
        }
    }
}

void Tooltip::Destroy()
{
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

