/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "DiskMonitor.h"
#include <vector>

DiskMonitor::DiskMonitor(const std::wstring& driveLetter)
    : m_DriveLetter(driveLetter)
{
    // Ensure drive letter ends with backslash
    if (!m_DriveLetter.empty() && m_DriveLetter.back() != L'\\')
    {
        m_DriveLetter += L'\\';
    }
}

DiskMonitor::~DiskMonitor()
{
}

static bool FillStatsForDrive(const std::wstring& driveLetter, DiskMonitor::Stats& stats)
{
    stats = { 0, 0, 0, 0, driveLetter };

    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (!GetDiskFreeSpaceEx(driveLetter.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
    {
        return false;
    }

    stats.totalSpace = totalBytes.QuadPart;
    stats.freeSpace = totalFreeBytes.QuadPart;
    stats.usedSpace = totalBytes.QuadPart - totalFreeBytes.QuadPart;

    if (totalBytes.QuadPart > 0)
    {
        stats.percentUsed = (int)((stats.usedSpace * 100) / totalBytes.QuadPart);
    }

    return true;
}

DiskMonitor::Stats DiskMonitor::GetStats()
{
    Stats stats;
    FillStatsForDrive(m_DriveLetter, stats);
    return stats;
}

std::vector<DiskMonitor::Stats> DiskMonitor::GetAllDrives()
{
    std::vector<Stats> allDrives;

    // If a specific drive was set, return empty (single drive mode)
    if (!m_DriveLetter.empty())
    {
        return allDrives;
    }

    // Get logical drive strings (e.g., "C:\", "D:\", etc.)
    DWORD drivesMask = GetLogicalDrives();
    
    for (int i = 0; i < 26; i++)
    {
        if (drivesMask & (1 << i))
        {
            wchar_t driveLetter[4];
            swprintf_s(driveLetter, L"%c:\\", L'A' + i);
            
            // Check if it's a fixed drive (not removable, CD-ROM, etc.)
            UINT driveType = GetDriveType(driveLetter);
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE)
            {
                Stats stats;
                if (FillStatsForDrive(driveLetter, stats))
                {
                    allDrives.push_back(stats);
                }
            }
        }
    }

    return allDrives;
}
