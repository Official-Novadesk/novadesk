/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Resource.h"

// Tray icon control functions
void ShowTrayIconDynamic();
void HideTrayIconDynamic();

#include "MenuItem.h"

void SetTrayMenu(const std::vector<MenuItem>& menu);
void ClearTrayMenu();
void SetShowDefaultTrayItems(bool show);

