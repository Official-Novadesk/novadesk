/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <windows.h>
#include <winternl.h>

class CPUMonitor {
public:
    CPUMonitor(int processor = 0);
    ~CPUMonitor();

    double GetUsage();

private:
    int m_Processor;
    double m_LastIdleTime;
    double m_LastSystemTime;

    static int s_NumProcessors;
    typedef LONG (NTAPI *FPNTQSI)(UINT, PVOID, ULONG, PULONG);
    static FPNTQSI s_NtQuerySystemInformation;

    struct SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
        LARGE_INTEGER IdleTime;
        LARGE_INTEGER KernelTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER Reserved1[2];
        ULONG Reserved2;
    };

    void UpdateTimes(double& idle, double& system);
    static void InitializeStatic();
};
