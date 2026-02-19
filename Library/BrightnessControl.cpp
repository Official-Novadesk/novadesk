/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BrightnessControl.h"
#include <winioctl.h>
#include <vector>
#include <algorithm>
#include <cstdlib>

// Laptop panel brightness IOCTLs.
#ifndef IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#ifndef IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x126, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#ifndef IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

BrightnessControl::BrightnessControl(int displayIndex)
    : m_DisplayIndex(displayIndex)
{
}

BrightnessControl::~BrightnessControl()
{
    Cleanup();
}

bool BrightnessControl::Initialize()
{
    if (m_Initialized) return m_Method != Method::None;

    m_Initialized = true;
    m_Method = Method::None;

    if (InitializeLaptopIoctl()) {
        m_Method = Method::LaptopIoctl;
        return true;
    }

    return false;
}

bool BrightnessControl::InitializeLaptopIoctl()
{
    m_hLcdDevice = CreateFileW(L"\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (m_hLcdDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    BYTE levels[256] = {};
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(
        m_hLcdDevice,
        IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS,
        nullptr, 0,
        levels, sizeof(levels),
        &bytesReturned,
        nullptr) || bytesReturned < 2) {
        CloseHandle(m_hLcdDevice);
        m_hLcdDevice = INVALID_HANDLE_VALUE;
        return false;
    }

    m_LaptopLevels.assign(levels, levels + bytesReturned);
    std::sort(m_LaptopLevels.begin(), m_LaptopLevels.end());
    m_LaptopLevels.erase(std::unique(m_LaptopLevels.begin(), m_LaptopLevels.end()), m_LaptopLevels.end());
    return m_LaptopLevels.size() >= 2;
}

void BrightnessControl::Cleanup()
{
    if (m_hLcdDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hLcdDevice);
        m_hLcdDevice = INVALID_HANDLE_VALUE;
    }
    m_LaptopLevels.clear();
}

BYTE BrightnessControl::FindClosestLaptopLevel(BYTE raw) const
{
    if (m_LaptopLevels.empty()) return raw;

    BYTE best = m_LaptopLevels.front();
    int bestDiff = std::abs((int)best - (int)raw);
    for (BYTE level : m_LaptopLevels) {
        int diff = std::abs((int)level - (int)raw);
        if (diff < bestDiff) {
            best = level;
            bestDiff = diff;
        }
    }
    return best;
}

bool BrightnessControl::GetLaptopBrightness(BrightnessInfo& outInfo)
{
    if (m_hLcdDevice == INVALID_HANDLE_VALUE || m_LaptopLevels.size() < 2) return false;

    BYTE values[3] = {};
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(
        m_hLcdDevice,
        IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS,
        nullptr, 0,
        values, sizeof(values),
        &bytesReturned,
        nullptr) || bytesReturned < 3) {
        return false;
    }

    // values[0] is policy: 1=AC, 2=DC.
    BYTE rawCurrent = (values[0] == 1) ? values[1] : values[2];
    BYTE minRaw = m_LaptopLevels.front();
    BYTE maxRaw = m_LaptopLevels.back();
    BYTE normalizedRaw = FindClosestLaptopLevel(rawCurrent);

    outInfo.min = minRaw;
    outInfo.max = maxRaw;
    outInfo.current = normalizedRaw;
    outInfo.supported = true;

    if (maxRaw > minRaw) {
        outInfo.percent = (int)(((double)(normalizedRaw - minRaw) * 100.0) / (double)(maxRaw - minRaw));
    } else {
        outInfo.percent = 0;
    }

    if (outInfo.percent < 0) outInfo.percent = 0;
    if (outInfo.percent > 100) outInfo.percent = 100;
    return true;
}

bool BrightnessControl::SetLaptopBrightnessPercent(int percent)
{
    if (m_hLcdDevice == INVALID_HANDLE_VALUE || m_LaptopLevels.size() < 2) return false;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    BYTE minRaw = m_LaptopLevels.front();
    BYTE maxRaw = m_LaptopLevels.back();
    BYTE targetRaw = minRaw;
    if (maxRaw > minRaw) {
        targetRaw = (BYTE)(minRaw + (BYTE)(((double)(maxRaw - minRaw) * (double)percent) / 100.0));
    }
    targetRaw = FindClosestLaptopLevel(targetRaw);

    BYTE setValues[3] = { 3, targetRaw, targetRaw };  // Set AC and DC.
    DWORD bytesReturned = 0;
    return DeviceIoControl(
        m_hLcdDevice,
        IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS,
        setValues, sizeof(setValues),
        nullptr, 0,
        &bytesReturned,
        nullptr) == TRUE;
}

bool BrightnessControl::GetBrightness(BrightnessInfo& outInfo)
{
    if (!Initialize()) {
        outInfo.supported = false;
        return false;
    }

    bool ok = (m_Method == Method::LaptopIoctl) ? GetLaptopBrightness(outInfo) : false;
    if (!ok) {
        outInfo.supported = false;
    }
    return ok;
}

bool BrightnessControl::SetBrightnessPercent(int percent)
{
    if (!Initialize()) {
        return false;
    }

    if (m_Method == Method::LaptopIoctl) {
        return SetLaptopBrightnessPercent(percent);
    }
    return false;
}
