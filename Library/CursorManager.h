/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

class Element;

class CursorManager
{
public:
    CursorManager() = default;
    ~CursorManager();

    HCURSOR GetCursorForElement(Element* element);

private:
    HCURSOR LoadCustomCursorFile(const std::wstring& fullPath);
    std::unordered_map<std::wstring, HCURSOR> m_CustomCursorCache;
};

