/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "CursorManager.h"
#include "Element.h"
#include "PathUtils.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>

#ifndef IDC_PEN
#define IDC_PEN IDC_CROSS
#endif

CursorManager::~CursorManager()
{
    for (auto& kv : m_CustomCursorCache)
    {
        if (kv.second) {
            DestroyCursor(kv.second);
        }
    }
    m_CustomCursorCache.clear();
}

HCURSOR CursorManager::LoadCustomCursorFile(const std::wstring& fullPath)
{
    auto it = m_CustomCursorCache.find(fullPath);
    if (it != m_CustomCursorCache.end()) {
        return it->second;
    }

    HCURSOR cursor = reinterpret_cast<HCURSOR>(
        LoadImageW(nullptr, fullPath.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE)
    );
    if (cursor) {
        m_CustomCursorCache[fullPath] = cursor;
    }
    return cursor;
}

HCURSOR CursorManager::GetCursorForElement(Element* element)
{
    if (!element || !element->GetMouseEventCursor()) {
        return nullptr;
    }

    std::wstring name = element->GetMouseEventCursorName();
    std::transform(name.begin(), name.end(), name.begin(), ::towlower);

    const struct CursorMapItem { const wchar_t* name; LPCWSTR id; } cursorMap[] = {
        { L"hand", IDC_HAND },
        { L"text", IDC_IBEAM },
        { L"help", IDC_HELP },
        { L"busy", IDC_APPSTARTING },
        { L"cross", IDC_CROSS },
        { L"pen", IDC_PEN },
        { L"no", IDC_NO },
        { L"size_all", IDC_SIZEALL },
        { L"size_nesw", IDC_SIZENESW },
        { L"size_ns", IDC_SIZENS },
        { L"size_nwse", IDC_SIZENWSE },
        { L"size_we", IDC_SIZEWE },
        { L"uparrow", IDC_UPARROW },
        { L"wait", IDC_WAIT }
    };

    for (const auto& item : cursorMap) {
        if (name == item.name) {
            return LoadCursor(nullptr, item.id);
        }
    }

    std::wstring dir = element->GetCursorsDir();
    if (!dir.empty() && !name.empty()) {
        std::wstring candidate = PathUtils::NormalizePath(dir + L"\\" + name);
        if (std::filesystem::exists(candidate)) {
            if (HCURSOR c = LoadCustomCursorFile(candidate)) return c;
        }

        const std::wstring exts[] = { L".cur", L".ani" };
        for (const auto& ext : exts) {
            std::wstring withExt = candidate + ext;
            if (std::filesystem::exists(withExt)) {
                if (HCURSOR c = LoadCustomCursorFile(withExt)) return c;
            }
        }
    }

    return LoadCursor(nullptr, IDC_HAND);
}

