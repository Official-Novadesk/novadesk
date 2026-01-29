/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <Windows.h>
#include <CommCtrl.h>
#include <string>

class Element;

class Tooltip
{
public:
    Tooltip();
    ~Tooltip();

    bool Initialize(HWND parentHWnd, HINSTANCE hInstance);
    void Update(Element* element);
    void Move();
    void CheckVisibility();  // Called by timer to check if tooltip should be hidden
    void Destroy();
    
    bool IsActive() const { return m_ActiveToolTipHWnd != nullptr; }

private:
    HWND m_ParentHWnd = nullptr;
    HWND m_ToolTipHWnd = nullptr;
    HWND m_ToolTipBalloonHWnd = nullptr;
    HWND m_ActiveToolTipHWnd = nullptr;
    DWORD m_LastMoveTime = 0;

    void InitializeToolTip(HWND hwnd);
    void DeactivateCurrent();
};

#endif
