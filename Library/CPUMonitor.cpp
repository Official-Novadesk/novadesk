/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "CPUMonitor.h"
#include <algorithm>
#include <vector>

int CPUMonitor::s_NumProcessors = 0;
CPUMonitor::FPNTQSI CPUMonitor::s_NtQuerySystemInformation = nullptr;

#define SystemProcessorPerformanceInformation 8

CPUMonitor::CPUMonitor(int processor) : m_Processor(processor), m_LastIdleTime(0.0), m_LastSystemTime(0.0) {
    if (s_NumProcessors == 0) InitializeStatic();

    if (m_Processor < 0 || m_Processor > s_NumProcessors) m_Processor = 0;

    double idle, system;
    UpdateTimes(idle, system);
    m_LastIdleTime = idle;
    m_LastSystemTime = system;
}

CPUMonitor::~CPUMonitor() {}

void CPUMonitor::InitializeStatic() {
    HMODULE hmod = GetModuleHandle(L"ntdll");
    if (hmod) {
        s_NtQuerySystemInformation = (FPNTQSI)GetProcAddress(hmod, "NtQuerySystemInformation");
    }

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    s_NumProcessors = (int)systemInfo.dwNumberOfProcessors;
}

void CPUMonitor::UpdateTimes(double& idle, double& system) {
    if (m_Processor == 0) {
        FILETIME ftIdle, ftKernel, ftUser;
        if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser)) {
            idle = (double)ftIdle.dwLowDateTime + ((double)ftIdle.dwHighDateTime * 4294967296.0);
            double kernel = (double)ftKernel.dwLowDateTime + ((double)ftKernel.dwHighDateTime * 4294967296.0);
            double user = (double)ftUser.dwLowDateTime + ((double)ftUser.dwHighDateTime * 4294967296.0);
            system = kernel + user;
        }
    } else if (s_NtQuerySystemInformation) {
        ULONG size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * s_NumProcessors;
        std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> info;
        info.resize(s_NumProcessors);
        if (s_NtQuerySystemInformation(SystemProcessorPerformanceInformation, info.data(), size, &size) == 0) {
            idle = (double)info[m_Processor - 1].IdleTime.QuadPart;
            system = (double)info[m_Processor - 1].KernelTime.QuadPart + (double)info[m_Processor - 1].UserTime.QuadPart;
        }
    }
}

double CPUMonitor::GetUsage() {
    double idle, system;
    UpdateTimes(idle, system);

    double deltaIdle = idle - m_LastIdleTime;
    double deltaSystem = system - m_LastSystemTime;

    double usage = 0.0;
    if (deltaSystem > 0) {
        usage = (1.0 - (deltaIdle / deltaSystem)) * 100.0;
    }

    m_LastIdleTime = idle;
    m_LastSystemTime = system;

    return (std::max)(0.0, (std::min)(100.0, usage));
}
