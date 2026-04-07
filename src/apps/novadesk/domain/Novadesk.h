/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "../Resource.h"
#include "../../shared/MenuItem.h"

int TrayCreate(const std::wstring &path);
void TrayDestroy(int trayId);
void TraySetImage(int trayId, const std::wstring &path);
void TraySetToolTip(int trayId, const std::wstring &toolTip);
void TraySetContextMenu(int trayId, const std::vector<MenuItem> &menu);
bool RequestSingleInstanceLock();
void ReleaseSingleInstanceLock();
