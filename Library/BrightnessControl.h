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
#include <vector>

class BrightnessControl {
public:
    struct BrightnessInfo {
        DWORD min = 0;
        DWORD max = 100;
        DWORD current = 0;
        int percent = 0;
        bool supported = false;
    };

    BrightnessControl(int displayIndex = 0);
    ~BrightnessControl();

    bool GetBrightness(BrightnessInfo& outInfo);
    bool SetBrightnessPercent(int percent);

private:
    enum class Method {
        Unknown,
        LaptopIoctl,
        None
    };

    int m_DisplayIndex = 0;
    Method m_Method = Method::Unknown;
    HANDLE m_hLcdDevice = INVALID_HANDLE_VALUE;
    std::vector<BYTE> m_LaptopLevels;
    bool m_Initialized = false;

    bool InitializeLaptopIoctl();
    bool GetLaptopBrightness(BrightnessInfo& outInfo);
    bool SetLaptopBrightnessPercent(int percent);
    BYTE FindClosestLaptopLevel(BYTE raw) const;

    bool Initialize();
    void Cleanup();
};
