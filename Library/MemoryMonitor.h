/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

class MemoryMonitor {
public:
    MemoryMonitor();
    ~MemoryMonitor();

    struct Stats {
        unsigned __int64 totalPhys;
        unsigned __int64 availPhys;
        unsigned __int64 usedPhys;
        int memoryLoad;
    };

    Stats GetStats();
};
