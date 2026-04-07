/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <NovadeskAPI/novadesk_addon.h>

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

const NovadeskHostAPI* g_Host = nullptr;

namespace
{
    struct BrightnessInfo
    {
        uint32_t min = 0;
        uint32_t max = 100;
        uint32_t current = 0;
        int percent = 0;
        bool supported = false;
    };

    class BrightnessControl
    {
    public:
        explicit BrightnessControl(int displayIndex = 0) : m_displayIndex(displayIndex) {}
        ~BrightnessControl() { Cleanup(); }

        bool GetBrightness(BrightnessInfo &outInfo)
        {
            if (!Initialize())
            {
                outInfo.supported = false;
                return false;
            }

            const bool ok = (m_method == Method::LaptopIoctl) ? GetLaptopBrightness(outInfo) : false;
            if (!ok)
            {
                outInfo.supported = false;
            }
            return ok;
        }

        bool SetBrightnessPercent(int percent)
        {
            if (!Initialize())
            {
                return false;
            }
            if (m_method == Method::LaptopIoctl)
            {
                return SetLaptopBrightnessPercent(percent);
            }
            return false;
        }

    private:
        enum class Method
        {
            Unknown,
            LaptopIoctl,
            None
        };

        int m_displayIndex = 0;
        Method m_method = Method::Unknown;
        HANDLE m_lcdDevice = INVALID_HANDLE_VALUE;
        std::vector<BYTE> m_laptopLevels;
        bool m_initialized = false;

        bool Initialize()
        {
            (void)m_displayIndex;
            if (m_initialized)
                return m_method != Method::None;

            m_initialized = true;
            m_method = Method::None;

            if (InitializeLaptopIoctl())
            {
                m_method = Method::LaptopIoctl;
                return true;
            }
            return false;
        }

        bool InitializeLaptopIoctl()
        {
#ifndef IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            m_lcdDevice = CreateFileW(L"\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (m_lcdDevice == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            BYTE levels[256] = {};
            DWORD bytesReturned = 0;
            if (!DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS, nullptr, 0, levels, sizeof(levels), &bytesReturned, nullptr) || bytesReturned < 2)
            {
                CloseHandle(m_lcdDevice);
                m_lcdDevice = INVALID_HANDLE_VALUE;
                return false;
            }

            m_laptopLevels.assign(levels, levels + bytesReturned);
            std::sort(m_laptopLevels.begin(), m_laptopLevels.end());
            m_laptopLevels.erase(std::unique(m_laptopLevels.begin(), m_laptopLevels.end()), m_laptopLevels.end());
            return m_laptopLevels.size() >= 2;
        }

        void Cleanup()
        {
            if (m_lcdDevice != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_lcdDevice);
                m_lcdDevice = INVALID_HANDLE_VALUE;
            }
            m_laptopLevels.clear();
        }

        BYTE FindClosestLaptopLevel(BYTE raw) const
        {
            if (m_laptopLevels.empty())
                return raw;

            BYTE best = m_laptopLevels.front();
            int bestDiff = std::abs(static_cast<int>(best) - static_cast<int>(raw));
            for (BYTE level : m_laptopLevels)
            {
                const int diff = std::abs(static_cast<int>(level) - static_cast<int>(raw));
                if (diff < bestDiff)
                {
                    best = level;
                    bestDiff = diff;
                }
            }
            return best;
        }

        bool GetLaptopBrightness(BrightnessInfo &outInfo)
        {
#ifndef IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x126, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            if (m_lcdDevice == INVALID_HANDLE_VALUE || m_laptopLevels.size() < 2)
                return false;

            BYTE values[3] = {};
            DWORD bytesReturned = 0;
            if (!DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS, nullptr, 0, values, sizeof(values), &bytesReturned, nullptr) || bytesReturned < 3)
            {
                return false;
            }

            const BYTE rawCurrent = (values[0] == 1) ? values[1] : values[2];
            const BYTE minRaw = m_laptopLevels.front();
            const BYTE maxRaw = m_laptopLevels.back();
            const BYTE normalizedRaw = FindClosestLaptopLevel(rawCurrent);

            outInfo.min = minRaw;
            outInfo.max = maxRaw;
            outInfo.current = normalizedRaw;
            outInfo.supported = true;

            if (maxRaw > minRaw)
            {
                outInfo.percent = static_cast<int>((static_cast<double>(normalizedRaw - minRaw) * 100.0) / static_cast<double>(maxRaw - minRaw));
            }
            else
            {
                outInfo.percent = 0;
            }

            if (outInfo.percent < 0)
                outInfo.percent = 0;
            if (outInfo.percent > 100)
                outInfo.percent = 100;
            return true;
        }

        bool SetLaptopBrightnessPercent(int percent)
        {
#ifndef IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            if (m_lcdDevice == INVALID_HANDLE_VALUE || m_laptopLevels.size() < 2)
                return false;

            if (percent < 0)
                percent = 0;
            if (percent > 100)
                percent = 100;

            const BYTE minRaw = m_laptopLevels.front();
            const BYTE maxRaw = m_laptopLevels.back();
            BYTE targetRaw = minRaw;
            if (maxRaw > minRaw)
            {
                targetRaw = static_cast<BYTE>(minRaw + static_cast<BYTE>((static_cast<double>(maxRaw - minRaw) * static_cast<double>(percent)) / 100.0));
            }
            targetRaw = FindClosestLaptopLevel(targetRaw);

            BYTE setValues[3] = {3, targetRaw, targetRaw};
            DWORD bytesReturned = 0;
            return DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, setValues, sizeof(setValues), nullptr, 0, &bytesReturned, nullptr) == TRUE;
        }
    };

    bool GetBrightness(BrightnessInfo &outInfo, int displayIndex)
    {
        BrightnessControl control(displayIndex);
        return control.GetBrightness(outInfo);
    }

    bool SetBrightnessPercent(int percent, int displayIndex)
    {
        BrightnessControl control(displayIndex);
        return control.SetBrightnessPercent(percent);
    }

    bool TryReadPropInt(novadesk_context ctx, const char *name, int &outValue, bool &outFound)
    {
        outFound = false;
        const int before = g_Host->GetTop(ctx);
        g_Host->GetProperty(ctx, 0, name);
        const bool pushed = g_Host->GetTop(ctx) > before;
        if (!pushed)
            return true;

        if (g_Host->IsNull(ctx, -1))
        {
            g_Host->Pop(ctx);
            return true;
        }

        if (!g_Host->IsNumber(ctx, -1))
        {
            g_Host->Pop(ctx);
            return false;
        }

        outValue = static_cast<int>(g_Host->GetNumber(ctx, -1));
        outFound = true;
        g_Host->Pop(ctx);
        return true;
    }

    int JsBrightnessGetValue(novadesk_context ctx)
    {
        int display = 0;
        const int top = g_Host->GetTop(ctx);
        if (top > 0 && g_Host->IsObject(ctx, 0))
        {
            bool found = false;
            if (!TryReadPropInt(ctx, "display", display, found))
            {
                g_Host->ThrowError(ctx, "getValue({ display }) requires numeric display");
                return 0;
            }
        }

        BrightnessInfo info;
        GetBrightness(info, display);

        g_Host->PushObject(ctx);
        g_Host->RegisterBool(ctx, "supported", info.supported ? 1 : 0);
        g_Host->RegisterNumber(ctx, "current", static_cast<double>(info.current));
        g_Host->RegisterNumber(ctx, "min", static_cast<double>(info.min));
        g_Host->RegisterNumber(ctx, "max", static_cast<double>(info.max));
        g_Host->RegisterNumber(ctx, "percent", static_cast<double>(info.percent));
        return 1;
    }

    int JsBrightnessSetValue(novadesk_context ctx)
    {
        int percent = 0;
        int display = 0;
        const int top = g_Host->GetTop(ctx);

        if (top > 0 && !g_Host->IsNull(ctx, 0))
        {
            if (g_Host->IsObject(ctx, 0))
            {
                bool foundPercent = false;
                if (!TryReadPropInt(ctx, "percent", percent, foundPercent))
                {
                    g_Host->ThrowError(ctx, "setValue({ percent }) requires numeric percent");
                    return 0;
                }
                if (!foundPercent)
                {
                    g_Host->ThrowError(ctx, "setValue({ percent[, display] }) requires percent");
                    return 0;
                }

                bool foundDisplay = false;
                if (!TryReadPropInt(ctx, "display", display, foundDisplay))
                {
                    g_Host->ThrowError(ctx, "setValue({ percent }) requires numeric percent");
                    return 0;
                }
            }
            else
            {
                if (!g_Host->IsNumber(ctx, 0))
                {
                    g_Host->ThrowError(ctx, "setValue(value) requires numeric value");
                    return 0;
                }
                percent = static_cast<int>(g_Host->GetNumber(ctx, 0));
            }
        }
        else
        {
            g_Host->ThrowError(ctx, "setValue({ percent[, display] }) requires percent");
            return 0;
        }

        const bool ok = SetBrightnessPercent(percent, display);
        g_Host->PushBool(ctx, ok ? 1 : 0);
        return 1;
    }
} // namespace

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host)
{
    (void)hMsgWnd;
    g_Host = host;

    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "Brightness");
    addon.RegisterString("version", "1.0.0");

    addon.RegisterFunction("getValue", JsBrightnessGetValue, 1);
    addon.RegisterFunction("setValue", JsBrightnessSetValue, 1);
}

NOVADESK_ADDON_UNLOAD()
{
}
