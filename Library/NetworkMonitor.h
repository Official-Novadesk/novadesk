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
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

class NetworkMonitor {
public:
    NetworkMonitor();
    ~NetworkMonitor();

    struct Stats {
        double inBytesPerSec;   // Download speed in bytes/sec
        double outBytesPerSec;  // Upload speed in bytes/sec
        unsigned __int64 totalIn;   // Total bytes received
        unsigned __int64 totalOut;  // Total bytes sent
    };

    Stats GetStats();

private:
    unsigned __int64 m_StartInBytes;
    unsigned __int64 m_StartOutBytes;
    unsigned __int64 m_LastInBytes;
    unsigned __int64 m_LastOutBytes;
    ULONGLONG m_LastUpdateTime;
    bool m_FirstUpdate;
};
