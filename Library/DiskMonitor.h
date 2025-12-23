/* Copyright (C) 2026 Novadesk Project 
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
#include <string>
#include <vector>

class DiskMonitor {
public:
    DiskMonitor(const std::wstring& driveLetter = L"C:");
    ~DiskMonitor();

    struct Stats {
        unsigned __int64 totalSpace;      // Total disk space in bytes
        unsigned __int64 freeSpace;       // Free disk space in bytes
        unsigned __int64 usedSpace;       // Used disk space in bytes
        int percentUsed;                  // Percentage used
        std::wstring driveLetter;         // Drive letter (e.g., "C:")
    };

    Stats GetStats();                     // Get stats for single drive
    std::vector<Stats> GetAllDrives();    // Get stats for all drives

private:
    std::wstring m_DriveLetter;
};
