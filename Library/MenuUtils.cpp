/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "MenuUtils.h"

namespace MenuUtils {
    void BuildMenu(HMENU hMenu, const std::vector<MenuItem>& items)
    {
        for (const auto& item : items)
        {
            if (item.isSeparator)
            {
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            }
            else if (!item.children.empty())
            {
                HMENU hSubMenu = CreatePopupMenu();
                BuildMenu(hSubMenu, item.children);

                UINT flags = MF_POPUP | MF_STRING;
                if (item.checked) flags |= MF_CHECKED;

                AppendMenu(hMenu, flags, (UINT_PTR)hSubMenu, item.text.c_str());
            }
            else
            {
                UINT flags = MF_STRING;
                if (item.checked) flags |= MF_CHECKED;
                AppendMenu(hMenu, flags, item.id, item.text.c_str());
            }
        }
    }
}
