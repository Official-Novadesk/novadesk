/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "MouseMonitor.h"

MouseMonitor::MouseMonitor()
{
}

MouseMonitor::~MouseMonitor()
{
}

MouseMonitor::Position MouseMonitor::GetPosition()
{
    Position pos = { 0, 0 };
    
    POINT cursorPos;
    if (GetCursorPos(&cursorPos))
    {
        pos.x = cursorPos.x;
        pos.y = cursorPos.y;
    }
    
    return pos;
}
