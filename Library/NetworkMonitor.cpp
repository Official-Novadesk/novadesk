/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "NetworkMonitor.h"
#include <cstdlib>

NetworkMonitor::NetworkMonitor()
    : m_LastInBytes(0)
    , m_LastOutBytes(0)
    , m_StartInBytes(0)
    , m_StartOutBytes(0)
    , m_LastUpdateTime(0)
    , m_FirstUpdate(true)
{
}

NetworkMonitor::~NetworkMonitor()
{
}

NetworkMonitor::Stats NetworkMonitor::GetStats()
{
    Stats stats = { 0, 0, 0, 0 };

    // Get network interface statistics using older API for compatibility
    PMIB_IFTABLE pIfTable = NULL;
    ULONG dwSize = 0;
    
    // Get required buffer size
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        pIfTable = (PMIB_IFTABLE)malloc(dwSize);
        if (pIfTable != NULL)
        {
            if (GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR)
            {
                unsigned __int64 totalIn = 0;
                unsigned __int64 totalOut = 0;

                // Sum up all active interfaces
                for (DWORD i = 0; i < pIfTable->dwNumEntries; i++)
                {
                    MIB_IFROW* pIfRow = &pIfTable->table[i];
                    
                    // Skip loopback interfaces
                    if (pIfRow->dwType == MIB_IF_TYPE_LOOPBACK)
                    {
                        continue;
                    }

                    totalIn += pIfRow->dwInOctets;
                    totalOut += pIfRow->dwOutOctets;
                }

                if (m_FirstUpdate)
                {
                    m_StartInBytes = totalIn;
                    m_StartOutBytes = totalOut;
                }

                stats.totalIn = totalIn - m_StartInBytes;
                stats.totalOut = totalOut - m_StartOutBytes;

                // Calculate speed (bytes per second)
                ULONGLONG currentTime = GetTickCount64();
                
                if (!m_FirstUpdate && m_LastUpdateTime > 0)
                {
                    ULONGLONG timeDiff = currentTime - m_LastUpdateTime;
                    if (timeDiff > 0)
                    {
                        double seconds = timeDiff / 1000.0;
                        stats.inBytesPerSec = (totalIn - m_LastInBytes) / seconds;
                        stats.outBytesPerSec = (totalOut - m_LastOutBytes) / seconds;
                    }
                }

                m_LastInBytes = totalIn;
                m_LastOutBytes = totalOut;
                m_LastUpdateTime = currentTime;
                m_FirstUpdate = false;
            }

            free(pIfTable);
        }
    }

    return stats;
}
