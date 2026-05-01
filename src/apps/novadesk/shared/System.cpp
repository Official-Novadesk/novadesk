#include "System.h"

#include <cstring>
#include <cwchar>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>

#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <powrprof.h>
#include <iphlpapi.h>
#include <wininet.h>
#include <shellapi.h>
#include <pdh.h>

#ifndef __IAudioMeterInformation_INTERFACE_DEFINED__
#define __IAudioMeterInformation_INTERFACE_DEFINED__
MIDL_INTERFACE("C02216F6-8C67-4B5B-9D00-D008E73E0064")
IAudioMeterInformation : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float *pfPeak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(UINT * pnChannelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(UINT32 u32ChannelCount, float *afPeakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD * pdwHardwareSupportMask) = 0;
};
#endif
// MinGW may not provide __mingw_uuidof support for this interface.
static const IID IID_IAudioMeterInformation_Local =
    {0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}};

#include "PathUtils.h"
#include "Utils.h"
#include "Logging.h"
#include "../../../third_party/json/json.hpp"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Pdh.lib")

namespace novadesk::shared::system
{
    using json = nlohmann::json;

    // *********************************************
    //  JSON

    struct Span
    {
        size_t start = 0;
        size_t end = 0;
    };

    // *********************************************
    //  CPU Metrics

    ULONGLONG g_lastIdleTime = 0;
    ULONGLONG g_lastKernelTime = 0;
    ULONGLONG g_lastUserTime = 0;
    bool g_cpuInitialized = false;

    // *********************************************
    //  Network Metrics

    ULONGLONG g_lastTotalIn = 0;
    ULONGLONG g_lastTotalOut = 0;
    std::chrono::steady_clock::time_point g_lastNetworkSample = std::chrono::steady_clock::time_point::min();

    // *********************************************
    //  Disk IO Metrics (PDH)

    std::mutex g_diskIoMutex;
    PDH_HQUERY g_diskIoQuery = nullptr;
    PDH_HCOUNTER g_diskReadCounter = nullptr;
    PDH_HCOUNTER g_diskWriteCounter = nullptr;
    bool g_diskIoPrimed = false;

    // *********************************************
    //  Power

    typedef struct _PROCESSOR_POWER_INFORMATION_LOCAL
    {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION_LOCAL;

    // *********************************************
    //  COM Utilities

    struct ComInit
    {
        HRESULT hr = E_FAIL;
        ComInit() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComInit()
        {
            if (SUCCEEDED(hr))
                CoUninitialize();
        }
        bool Ok() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
    };

    // *****************************************************************************
    // Audio
    // *****************************************************************************

    static IAudioEndpointVolume *GetVolumeInterface()
    {
        ComInit com;
        if (!com.Ok())
            return nullptr;

        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *device = nullptr;
        IAudioEndpointVolume *volume = nullptr;

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      __uuidof(IMMDeviceEnumerator), (void **)&deviceEnum);
        if (FAILED(hr) || !deviceEnum)
            return nullptr;

        hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        if (SUCCEEDED(hr) && device)
        {
            hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void **)&volume);
        }

        if (device)
            device->Release();
        if (deviceEnum)
            deviceEnum->Release();

        if (FAILED(hr))
        {
            if (volume)
                volume->Release();
            return nullptr;
        }
        return volume;
    }

    bool AudioSetVolume(int volumePercent)
    {
        if (volumePercent < 0)
            volumePercent = 0;
        if (volumePercent > 100)
            volumePercent = 100;

        IAudioEndpointVolume *volume = GetVolumeInterface();
        if (!volume)
        {
            return false;
        }

        const float scalar = static_cast<float>(volumePercent) / 100.0f;
        const HRESULT hr = volume->SetMasterVolumeLevelScalar(scalar, nullptr);
        volume->Release();
        return SUCCEEDED(hr);
    }

    int AudioGetVolume()
    {
        IAudioEndpointVolume *volume = GetVolumeInterface();
        if (!volume)
        {
            return 0;
        }

        float scalar = 0.0f;
        volume->GetMasterVolumeLevelScalar(&scalar);
        volume->Release();

        const int result = static_cast<int>(scalar * 100.0f + 0.5f);
        return (result < 0) ? 0 : ((result > 100) ? 100 : result);
    }

    bool AudioPlaySound(const std::wstring &path, bool loop)
    {
        DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
        if (loop)
        {
            flags |= SND_LOOP;
        }
        return PlaySoundW(path.c_str(), nullptr, flags) == TRUE;
    }

    void AudioStopSound()
    {
        PlaySoundW(nullptr, nullptr, 0);
    }

    // *****************************************************************************
    // Clipboard
    // *****************************************************************************

    bool ClipboardSetText(const std::wstring &text)
    {
        if (!OpenClipboard(nullptr))
        {
            return false;
        }
        EmptyClipboard();

        const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!hMem)
        {
            CloseClipboard();
            return false;
        }

        void *ptr = GlobalLock(hMem);
        if (!ptr)
        {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }
        memcpy(ptr, text.c_str(), bytes);
        GlobalUnlock(hMem);

        if (!SetClipboardData(CF_UNICODETEXT, hMem))
        {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }

        CloseClipboard();
        return true;
    }

    bool ClipboardGetText(std::wstring &outText)
    {
        outText.clear();
        if (!OpenClipboard(nullptr))
        {
            return false;
        }

        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (!hData)
        {
            CloseClipboard();
            return false;
        }

        const wchar_t *text = static_cast<const wchar_t *>(GlobalLock(hData));
        if (!text)
        {
            CloseClipboard();
            return false;
        }

        outText = text;
        GlobalUnlock(hData);
        CloseClipboard();
        return true;
    }

    // *****************************************************************************
    // CPU Metrics
    // *****************************************************************************

    bool GetCpuStats(CpuStats &outStats)
    {
        FILETIME idleFt{}, kernelFt{}, userFt{};
        if (!GetSystemTimes(&idleFt, &kernelFt, &userFt))
        {
            return false;
        }

        ULARGE_INTEGER idle{}, kernel{}, user{};
        idle.LowPart = idleFt.dwLowDateTime;
        idle.HighPart = idleFt.dwHighDateTime;
        kernel.LowPart = kernelFt.dwLowDateTime;
        kernel.HighPart = kernelFt.dwHighDateTime;
        user.LowPart = userFt.dwLowDateTime;
        user.HighPart = userFt.dwHighDateTime;

        if (!g_cpuInitialized)
        {
            g_lastIdleTime = idle.QuadPart;
            g_lastKernelTime = kernel.QuadPart;
            g_lastUserTime = user.QuadPart;
            g_cpuInitialized = true;
            outStats.usage = 0.0;
            return true;
        }

        const ULONGLONG idleDelta = idle.QuadPart - g_lastIdleTime;
        const ULONGLONG kernelDelta = kernel.QuadPart - g_lastKernelTime;
        const ULONGLONG userDelta = user.QuadPart - g_lastUserTime;
        const ULONGLONG totalDelta = kernelDelta + userDelta;

        g_lastIdleTime = idle.QuadPart;
        g_lastKernelTime = kernel.QuadPart;
        g_lastUserTime = user.QuadPart;

        if (totalDelta == 0)
        {
            outStats.usage = 0.0;
            return true;
        }

        double usage = (static_cast<double>(totalDelta - idleDelta) * 100.0) / static_cast<double>(totalDelta);
        if (usage < 0.0)
            usage = 0.0;
        if (usage > 100.0)
            usage = 100.0;
        outStats.usage = usage;
        return true;
    }

    // *****************************************************************************
    // Disk Metrics
    // *****************************************************************************

    bool GetDiskStats(const std::wstring &path, DiskStats &outStats)
    {
        std::wstring target = path;
        if (target.empty())
        {
            std::error_code ec;
            target = std::filesystem::current_path(ec).root_path().wstring();
            if (target.empty())
            {
                target = L"C:\\";
            }
        }

        ULARGE_INTEGER freeBytesAvailable{};
        ULARGE_INTEGER totalBytes{};
        ULARGE_INTEGER totalFreeBytes{};
        if (!GetDiskFreeSpaceExW(target.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
        {
            return false;
        }

        outStats.total = static_cast<double>(totalBytes.QuadPart);
        outStats.available = static_cast<double>(freeBytesAvailable.QuadPart);
        outStats.used = static_cast<double>(totalBytes.QuadPart - totalFreeBytes.QuadPart);
        outStats.percent = (outStats.total > 0.0)
                               ? static_cast<int>((outStats.used * 100.0) / outStats.total + 0.5)
                               : 0;
        return true;
    }

    bool GetDiskIoStats(DiskIoStats &outStats)
    {
        std::lock_guard<std::mutex> lock(g_diskIoMutex);

        if (!g_diskIoQuery)
        {
            if (PdhOpenQueryW(nullptr, 0, &g_diskIoQuery) != ERROR_SUCCESS)
            {
                return false;
            }

            if (PdhAddEnglishCounterW(g_diskIoQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &g_diskReadCounter) != ERROR_SUCCESS ||
                PdhAddEnglishCounterW(g_diskIoQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &g_diskWriteCounter) != ERROR_SUCCESS)
            {
                if (g_diskIoQuery)
                {
                    PdhCloseQuery(g_diskIoQuery);
                }
                g_diskIoQuery = nullptr;
                g_diskReadCounter = nullptr;
                g_diskWriteCounter = nullptr;
                g_diskIoPrimed = false;
                return false;
            }

            if (PdhCollectQueryData(g_diskIoQuery) != ERROR_SUCCESS)
            {
                PdhCloseQuery(g_diskIoQuery);
                g_diskIoQuery = nullptr;
                g_diskReadCounter = nullptr;
                g_diskWriteCounter = nullptr;
                g_diskIoPrimed = false;
                return false;
            }
            g_diskIoPrimed = true;
        }

        if (!g_diskIoPrimed)
        {
            if (PdhCollectQueryData(g_diskIoQuery) != ERROR_SUCCESS)
            {
                return false;
            }
            g_diskIoPrimed = true;
        }

        if (PdhCollectQueryData(g_diskIoQuery) != ERROR_SUCCESS)
        {
            return false;
        }

        PDH_FMT_COUNTERVALUE readValue{};
        PDH_FMT_COUNTERVALUE writeValue{};
        if (PdhGetFormattedCounterValue(g_diskReadCounter, PDH_FMT_DOUBLE, nullptr, &readValue) != ERROR_SUCCESS ||
            PdhGetFormattedCounterValue(g_diskWriteCounter, PDH_FMT_DOUBLE, nullptr, &writeValue) != ERROR_SUCCESS)
        {
            return false;
        }

        outStats.readSpeed = (readValue.CStatus == ERROR_SUCCESS) ? readValue.doubleValue : 0.0;
        outStats.writeSpeed = (writeValue.CStatus == ERROR_SUCCESS) ? writeValue.doubleValue : 0.0;

        if (outStats.readSpeed < 0.0)
            outStats.readSpeed = 0.0;
        if (outStats.writeSpeed < 0.0)
            outStats.writeSpeed = 0.0;
        return true;
    }

    // *****************************************************************************
    // RecycleBin Object
    // *****************************************************************************

    bool GetRecycleBinStats(RecycleBinStats &outStats)
    {
        SHQUERYRBINFO rbi{};
        rbi.cbSize = sizeof(rbi);

        HRESULT hr = SHQueryRecycleBinW(nullptr, &rbi);
        if (FAILED(hr))
        {
            return false;
        }

        outStats.count = static_cast<double>(rbi.i64NumItems);
        outStats.size = static_cast<double>(rbi.i64Size);
        return true;
    }

    bool OpenRecycleBin()
    {
        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            L"explorer.exe",
            L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}",
            nullptr,
            SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }

    bool EmptyRecycleBin(bool silent)
    {
        DWORD flags = 0;
        if (silent)
        {
            flags = SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND;
        }

        HRESULT hr = SHEmptyRecycleBinW(nullptr, nullptr, flags);
        return SUCCEEDED(hr);
    }

    // *****************************************************************************
    // Display Metrics
    // *****************************************************************************

    namespace
    {
        struct DisplayEnumContext
        {
            DisplayMetrics *out = nullptr;
        };

        BOOL CALLBACK EnumDisplayMonitorsForMetrics(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM dwData)
        {
            auto *ctx = reinterpret_cast<DisplayEnumContext *>(dwData);
            if (!ctx || !ctx->out || !lprcMonitor)
                return TRUE;

            DisplayMonitorInfo dm{};
            dm.id = static_cast<int>(ctx->out->monitors.size()) + 1;
            dm.active = true;
            dm.screen.left = lprcMonitor->left;
            dm.screen.top = lprcMonitor->top;
            dm.screen.right = lprcMonitor->right;
            dm.screen.bottom = lprcMonitor->bottom;

            MONITORINFOEXW mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(hMonitor, &mi))
            {
                dm.work.left = mi.rcWork.left;
                dm.work.top = mi.rcWork.top;
                dm.work.right = mi.rcWork.right;
                dm.work.bottom = mi.rcWork.bottom;
                dm.deviceName = mi.szDevice;

                DISPLAY_DEVICEW ddm{};
                ddm.cb = sizeof(ddm);
                DWORD monIdx = 0;
                while (EnumDisplayDevicesW(mi.szDevice, monIdx++, &ddm, 0))
                {
                    if ((ddm.StateFlags & DISPLAY_DEVICE_ACTIVE) && (ddm.StateFlags & DISPLAY_DEVICE_ATTACHED))
                    {
                        dm.monitorName = ddm.DeviceString;
                        break;
                    }
                    ddm.cb = sizeof(ddm);
                }

                if (mi.dwFlags & MONITORINFOF_PRIMARY)
                {
                    ctx->out->primaryIndex = static_cast<int>(ctx->out->monitors.size());
                }
            }
            else
            {
                dm.work = dm.screen;
            }

            ctx->out->monitors.push_back(std::move(dm));
            return TRUE;
        }
    } // namespace

    DisplayMetrics GetDisplayMetrics()
    {
        DisplayMetrics out;
        out.virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        out.virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        out.virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        out.virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        out.primaryIndex = 0;

        DisplayEnumContext ctx{};
        ctx.out = &out;
        EnumDisplayMonitors(nullptr, nullptr, EnumDisplayMonitorsForMetrics, reinterpret_cast<LPARAM>(&ctx));

        return out;
    }

    // *****************************************************************************
    // Environment
    // *****************************************************************************
    std::wstring GetEnv(const std::wstring &name)
    {
        if (name.empty())
        {
            return L"";
        }

        const DWORD size = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
        if (size == 0)
        {
            return L"";
        }

        std::wstring value(size, L'\0');
        DWORD written = GetEnvironmentVariableW(name.c_str(), value.data(), size);
        if (written == 0)
        {
            return L"";
        }
        if (!value.empty() && value.back() == L'\0')
        {
            value.pop_back();
        }
        return value;
    }

    std::vector<std::pair<std::wstring, std::wstring>> GetAllEnv()
    {
        std::vector<std::pair<std::wstring, std::wstring>> out;
        LPWCH block = GetEnvironmentStringsW();
        if (!block)
        {
            return out;
        }

        LPCWCH p = block;
        while (*p)
        {
            std::wstring entry(p);
            const size_t eq = entry.find(L'=');
            if (eq != std::wstring::npos && eq > 0)
            {
                out.emplace_back(entry.substr(0, eq), entry.substr(eq + 1));
            }
            p += entry.size() + 1;
        }

        FreeEnvironmentStringsW(block);
        return out;
    }
    // *****************************************************************************
    // Execute Command
    // *****************************************************************************

    bool Execute(const std::wstring &target, const std::wstring &parameters, const std::wstring &workingDir, int show)
    {
        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            target.c_str(),
            parameters.empty() ? nullptr : parameters.c_str(),
            workingDir.empty() ? nullptr : workingDir.c_str(),
            show);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }

    // *****************************************************************************
    // Json
    // *****************************************************************************

    bool IsWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    void SkipWhitespaceAndComments(const std::string &text, size_t &i, size_t end)
    {
        while (i < end)
        {
            if (IsWhitespace(text[i]))
            {
                ++i;
                continue;
            }
            if (text[i] == '/' && i + 1 < end)
            {
                if (text[i + 1] == '/')
                {
                    i += 2;
                    while (i < end && text[i] != '\n')
                    {
                        ++i;
                    }
                    continue;
                }
                if (text[i + 1] == '*')
                {
                    i += 2;
                    while (i + 1 < end && !(text[i] == '*' && text[i + 1] == '/'))
                    {
                        ++i;
                    }
                    if (i + 1 < end)
                    {
                        i += 2;
                    }
                    continue;
                }
            }
            break;
        }
    }

    bool SkipString(const std::string &text, size_t &i, size_t end)
    {
        if (i >= end || text[i] != '"')
        {
            return false;
        }
        ++i;
        while (i < end)
        {
            char c = text[i++];
            if (c == '\\')
            {
                if (i < end)
                {
                    ++i;
                }
                continue;
            }
            if (c == '"')
            {
                return true;
            }
        }
        return false;
    }

    bool FindRootObjectSpan(const std::string &text, Span &span)
    {
        size_t i = 0;
        size_t end = text.size();
        while (i < end)
        {
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end)
            {
                break;
            }
            if (text[i] == '"')
            {
                if (!SkipString(text, i, end))
                {
                    return false;
                }
                continue;
            }
            if (text[i] == '{')
            {
                size_t depth = 0;
                size_t start = i;
                while (i < end)
                {
                    char c = text[i];
                    if (c == '"')
                    {
                        if (!SkipString(text, i, end))
                        {
                            return false;
                        }
                        continue;
                    }
                    if (c == '/' && i + 1 < end)
                    {
                        size_t before = i;
                        SkipWhitespaceAndComments(text, i, end);
                        if (i != before)
                        {
                            continue;
                        }
                    }
                    if (c == '{')
                    {
                        ++depth;
                    }
                    else if (c == '}')
                    {
                        --depth;
                        if (depth == 0)
                        {
                            span.start = start;
                            span.end = i + 1;
                            return true;
                        }
                    }
                    ++i;
                }
                return false;
            }
            ++i;
        }
        return false;
    }

    bool ParseValueSpan(const std::string &text, size_t &i, size_t end, Span &span)
    {
        SkipWhitespaceAndComments(text, i, end);
        if (i >= end)
        {
            return false;
        }
        span.start = i;
        char c = text[i];
        if (c == '"')
        {
            if (!SkipString(text, i, end))
            {
                return false;
            }
            span.end = i;
            return true;
        }
        if (c == '{' || c == '[')
        {
            char open = c;
            char close = (c == '{') ? '}' : ']';
            size_t depth = 0;
            while (i < end)
            {
                char ch = text[i];
                if (ch == '"')
                {
                    if (!SkipString(text, i, end))
                    {
                        return false;
                    }
                    continue;
                }
                if (ch == '/' && i + 1 < end)
                {
                    size_t before = i;
                    SkipWhitespaceAndComments(text, i, end);
                    if (i != before)
                    {
                        continue;
                    }
                }
                if (ch == open)
                {
                    ++depth;
                }
                else if (ch == close)
                {
                    --depth;
                    if (depth == 0)
                    {
                        ++i;
                        span.end = i;
                        return true;
                    }
                }
                ++i;
            }
            return false;
        }
        while (i < end)
        {
            char ch = text[i];
            if (ch == ',' || ch == '}' || ch == ']')
            {
                break;
            }
            ++i;
        }
        span.end = i;
        return true;
    }

    bool CollectObjectMembers(const std::string &text, const Span &objSpan, std::unordered_map<std::string, Span> &members, size_t &lastValueEnd)
    {
        size_t i = objSpan.start + 1;
        size_t end = objSpan.end - 1;
        lastValueEnd = objSpan.start + 1;
        while (i < end)
        {
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end)
            {
                break;
            }
            if (text[i] == '}')
            {
                break;
            }
            if (text[i] != '"')
            {
                return false;
            }
            size_t keyStart = i + 1;
            if (!SkipString(text, i, end))
            {
                return false;
            }
            size_t keyEnd = i - 1;
            std::string key = text.substr(keyStart, keyEnd - keyStart);
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end || text[i] != ':')
            {
                return false;
            }
            ++i;
            Span valueSpan;
            if (!ParseValueSpan(text, i, end, valueSpan))
            {
                return false;
            }
            members[key] = valueSpan;
            lastValueEnd = valueSpan.end;
            SkipWhitespaceAndComments(text, i, end);
            if (i < end && text[i] == ',')
            {
                ++i;
            }
        }
        return true;
    }

    size_t FindInsertPosition(const std::string &text, const Span &objSpan)
    {
        size_t i = objSpan.end - 1;
        while (i > objSpan.start)
        {
            size_t before = i;
            SkipWhitespaceAndComments(text, i, objSpan.end);
            if (i != before)
            {
                continue;
            }
            if (IsWhitespace(text[i]))
            {
                --i;
                continue;
            }
            break;
        }
        return objSpan.end - 1;
    }

    bool MergeObjectInText(std::string &text, Span &objSpan, const json &patch)
    {
        if (!patch.is_object())
        {
            return false;
        }

        std::string indent = "    ";
        bool hasMembers = true;
        size_t insertPos = FindInsertPosition(text, objSpan);

        for (auto &item : patch.items())
        {
            std::unordered_map<std::string, Span> members;
            size_t lastValueEnd = objSpan.start + 1;
            if (!CollectObjectMembers(text, objSpan, members, lastValueEnd))
            {
                return false;
            }

            const std::string &key = item.key();
            auto it = members.find(key);
            hasMembers = !members.empty();

            if (it != members.end())
            {
                Span span = it->second;
                if (item.value().is_object())
                {
                    size_t i = span.start;
                    SkipWhitespaceAndComments(text, i, span.end);
                    if (i < span.end && text[i] == '{')
                    {
                        Span nestedObj;
                        if (FindRootObjectSpan(text.substr(span.start, span.end - span.start), nestedObj))
                        {
                            Span adjusted;
                            adjusted.start = span.start + nestedObj.start;
                            adjusted.end = span.start + nestedObj.end;
                            size_t beforeSize = text.size();
                            if (MergeObjectInText(text, adjusted, item.value()))
                            {
                                size_t afterSize = text.size();
                                if (afterSize != beforeSize)
                                {
                                    objSpan.end += (afterSize - beforeSize);
                                }
                                continue;
                            }
                        }
                    }
                }

                std::string serializedValue = item.value().dump(4);
                size_t beforeSize = text.size();
                text.replace(span.start, span.end - span.start, serializedValue);
                size_t afterSize = text.size();
                if (afterSize != beforeSize)
                {
                    objSpan.end += (afterSize - beforeSize);
                }
            }
            else
            {
                std::string serializedValue = item.value().dump(4);
                std::string entry = (hasMembers ? "," : "") + std::string("\n") + indent + "\"" + key + "\": " + serializedValue;
                size_t beforeSize = text.size();
                text.insert(insertPos, entry);
                size_t afterSize = text.size();
                if (afterSize != beforeSize)
                {
                    objSpan.end += (afterSize - beforeSize);
                }
                insertPos += entry.size();
                hasMembers = true;
            }
            insertPos = FindInsertPosition(text, objSpan);
        }

        if (hasMembers)
        {
            if (insertPos < text.size() && text[insertPos] != '\n')
            {
                text.insert(insertPos, "\n");
            }
        }
        return true;
    }

    bool MergeIntoJsonText(std::string &text, const json &patch)
    {
        if (!patch.is_object())
        {
            return false;
        }

        Span root;
        if (!FindRootObjectSpan(text, root))
        {
            return false;
        }

        return MergeObjectInText(text, root, patch);
    }

    bool JsonReadTextFile(const std::wstring &path, std::string &outText)
    {
        outText.clear();
        std::ifstream f(std::filesystem::path(path), std::ios::binary);
        if (!f.is_open())
        {
            return false;
        }
        outText.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        return true;
    }

    bool JsonWriteTextFile(const std::wstring &path, const std::string &text)
    {
        std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            return false;
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        return true;
    }

    bool JsonMergePatchFile(const std::wstring &path, const std::string &patchText)
    {
        try
        {
            const nlohmann::json patch = nlohmann::json::parse(patchText, nullptr, true, true);
            if (!patch.is_object())
            {
                return false;
            }

            std::string current;
            if (novadesk::shared::system::JsonReadTextFile(path, current) && current.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                if (::novadesk::shared::system::MergeIntoJsonText(current, patch))
                {
                    return novadesk::shared::system::JsonWriteTextFile(path, current);
                }
                ::Logging::Log(::LogLevel::Warn, L"JSON merge failed; rewriting without preserving comments: %s", path.c_str());
            }

            return novadesk::shared::system::JsonWriteTextFile(path, patch.dump(4));
        }
        catch (const nlohmann::json::parse_error &e)
        {
            ::Logging::Log(::LogLevel::Error, L"JSON parse error in merge patch: %S", e.what());
            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    // *****************************************************************************
    // Memory Metrics
    // *****************************************************************************

    bool GetMemoryStats(MemoryStats &outStats)
    {
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(mem);
        if (!GlobalMemoryStatusEx(&mem))
        {
            return false;
        }

        outStats.total = static_cast<double>(mem.ullTotalPhys);
        outStats.available = static_cast<double>(mem.ullAvailPhys);
        outStats.used = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys);
        outStats.percent = static_cast<int>(mem.dwMemoryLoad);
        return true;
    }

    // *****************************************************************************
    // Network Metrics
    // *****************************************************************************

    bool GetNetworkStats(NetworkStats &outStats)
    {
        ULONG size = 0;
        if (GetIfTable(nullptr, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
        {
            return false;
        }

        std::vector<BYTE> buffer(size);
        auto *table = reinterpret_cast<PMIB_IFTABLE>(buffer.data());
        if (GetIfTable(table, &size, FALSE) != NO_ERROR)
        {
            return false;
        }

        ULONGLONG totalIn = 0;
        ULONGLONG totalOut = 0;
        for (DWORD i = 0; i < table->dwNumEntries; ++i)
        {
            const MIB_IFROW &row = table->table[i];
            if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
            {
                totalIn += row.dwInOctets;
                totalOut += row.dwOutOctets;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        double netIn = 0.0;
        double netOut = 0.0;
        if (g_lastNetworkSample != std::chrono::steady_clock::time_point::min())
        {
            const double dt = std::chrono::duration<double>(now - g_lastNetworkSample).count();
            if (dt > 0.0)
            {
                netIn = static_cast<double>(totalIn - g_lastTotalIn) / dt;
                netOut = static_cast<double>(totalOut - g_lastTotalOut) / dt;
            }
        }

        g_lastTotalIn = totalIn;
        g_lastTotalOut = totalOut;
        g_lastNetworkSample = now;

        outStats.netIn = netIn;
        outStats.netOut = netOut;
        outStats.totalIn = static_cast<double>(totalIn);
        outStats.totalOut = static_cast<double>(totalOut);
        return true;
    }

    // *****************************************************************************
    // Power
    // *****************************************************************************

    bool GetPowerStatus(PowerStatus &outStatus)
    {
        SYSTEM_POWER_STATUS sps{};
        if (!GetSystemPowerStatus(&sps))
        {
            return false;
        }

        outStatus.acline = (sps.ACLineStatus == 1) ? 1 : 0;

        int status = 4;
        if (sps.BatteryFlag & 128)
            status = 0;
        else if (sps.BatteryFlag & 8)
            status = 1;
        else if (sps.BatteryFlag & 4)
            status = 2;
        else if (sps.BatteryFlag & 2)
            status = 3;

        outStatus.status = status;
        outStatus.status2 = sps.BatteryFlag;
        outStatus.lifetime = static_cast<double>(sps.BatteryLifeTime);
        outStatus.percent = (sps.BatteryLifePercent == 255) ? 0 : sps.BatteryLifePercent;

        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        const UINT cpuCount = static_cast<UINT>(si.dwNumberOfProcessors);
        outStatus.mhz = 0.0;

        if (cpuCount > 0)
        {
            auto *ppi = new PROCESSOR_POWER_INFORMATION_LOCAL[cpuCount];
            if (CallNtPowerInformation(
                    ProcessorInformation,
                    nullptr,
                    0,
                    ppi,
                    sizeof(PROCESSOR_POWER_INFORMATION_LOCAL) * cpuCount) == 0)
            {
                outStatus.mhz = static_cast<double>(ppi[0].CurrentMhz);
            }
            delete[] ppi;
        }

        outStatus.hz = outStatus.mhz * 1000000.0;
        return true;
    }

    // *****************************************************************************
    // Registry
    // *****************************************************************************

    HKEY GetRegistryRootKey(const std::wstring &root)
    {
        if (root == L"HKCU" || root == L"HKEY_CURRENT_USER")
            return HKEY_CURRENT_USER;
        if (root == L"HKLM" || root == L"HKEY_LOCAL_MACHINE")
            return HKEY_LOCAL_MACHINE;
        if (root == L"HKCR" || root == L"HKEY_CLASSES_ROOT")
            return HKEY_CLASSES_ROOT;
        if (root == L"HKU" || root == L"HKEY_USERS")
            return HKEY_USERS;
        return nullptr;
    }

    bool SplitRegistryPath(const std::wstring &fullPath, HKEY &outRoot, std::wstring &outSubKey)
    {
        const size_t p = fullPath.find(L'\\');
        if (p == std::wstring::npos)
        {
            return false;
        }
        const std::wstring rootPart = fullPath.substr(0, p);
        outSubKey = fullPath.substr(p + 1);
        outRoot = GetRegistryRootKey(rootPart);
        return outRoot != nullptr && !outSubKey.empty();
    }

    bool RegistryReadData(const std::wstring &fullPath, const std::wstring &valueName, RegistryValue &outValue)
    {
        outValue = RegistryValue{};

        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return false;
        }

        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            return false;
        }

        if (type == REG_SZ || type == REG_EXPAND_SZ)
        {
            std::vector<wchar_t> buf(size / sizeof(wchar_t) + 1, L'\0');
            if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS)
            {
                outValue.type = RegistryValueType::String;
                outValue.stringValue = buf.data();
                RegCloseKey(key);
                return true;
            }
        }
        else if (type == REG_DWORD)
        {
            DWORD value = 0;
            DWORD dwordSize = sizeof(value);
            if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&value), &dwordSize) == ERROR_SUCCESS)
            {
                outValue.type = RegistryValueType::Number;
                outValue.numberValue = static_cast<double>(value);
                RegCloseKey(key);
                return true;
            }
        }

        RegCloseKey(key);
        return false;
    }

    bool RegistryWriteString(const std::wstring &fullPath, const std::wstring &valueName, const std::wstring &value)
    {
        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        {
            return false;
        }

        const auto *raw = reinterpret_cast<const BYTE *>(value.c_str());
        const DWORD rawSize = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
        const bool ok = (RegSetValueExW(key, valueName.c_str(), 0, REG_SZ, raw, rawSize) == ERROR_SUCCESS);
        RegCloseKey(key);
        return ok;
    }

    bool RegistryWriteNumber(const std::wstring &fullPath, const std::wstring &valueName, double value)
    {
        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        {
            return false;
        }

        const DWORD dword = static_cast<DWORD>(value);
        const bool ok = (RegSetValueExW(
                             key,
                             valueName.c_str(),
                             0,
                             REG_DWORD,
                             reinterpret_cast<const BYTE *>(&dword),
                             sizeof(dword)) == ERROR_SUCCESS);
        RegCloseKey(key);
        return ok;
    }

    // *****************************************************************************
    // Wallpaper
    // *****************************************************************************

    bool SetWallpaper(const std::wstring &imagePath, const std::wstring &style)
    {
        const wchar_t *wallpaperStyle = L"10";
        const wchar_t *tileWallpaper = L"0";
        if (style == L"fill")
        {
            wallpaperStyle = L"10";
            tileWallpaper = L"0";
        }
        else if (style == L"fit")
        {
            wallpaperStyle = L"6";
            tileWallpaper = L"0";
        }
        else if (style == L"stretch")
        {
            wallpaperStyle = L"2";
            tileWallpaper = L"0";
        }
        else if (style == L"tile")
        {
            wallpaperStyle = L"0";
            tileWallpaper = L"1";
        }
        else if (style == L"center")
        {
            wallpaperStyle = L"0";
            tileWallpaper = L"0";
        }
        else if (style == L"span")
        {
            wallpaperStyle = L"22";
            tileWallpaper = L"0";
        }

        HKEY key = nullptr;
        const LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &key);
        if (openRes == ERROR_SUCCESS)
        {
            RegSetValueExW(
                key,
                L"WallpaperStyle",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE *>(wallpaperStyle),
                static_cast<DWORD>((wcslen(wallpaperStyle) + 1) * sizeof(wchar_t)));
            RegSetValueExW(
                key,
                L"TileWallpaper",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE *>(tileWallpaper),
                static_cast<DWORD>((wcslen(tileWallpaper) + 1) * sizeof(wchar_t)));
            RegCloseKey(key);
        }

        return SystemParametersInfoW(
                   SPI_SETDESKWALLPAPER,
                   0,
                   const_cast<wchar_t *>(imagePath.c_str()),
                   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) == TRUE;
    }

    bool GetCurrentWallpaperPath(std::wstring &outPath)
    {
        outPath.assign(MAX_PATH, L'\0');
        const BOOL ok = SystemParametersInfoW(
            SPI_GETDESKWALLPAPER,
            static_cast<UINT>(outPath.size()),
            outPath.data(),
            0);
        if (!ok)
        {
            outPath.clear();
            return false;
        }
        outPath.resize(wcslen(outPath.c_str()));
        return !outPath.empty();
    }

    // *****************************************************************************
    // Webfetch
    // *****************************************************************************

    bool WebFetch(const std::wstring &url, std::string &outData)
    {
        outData.clear();
        if (url.empty())
        {
            return false;
        }

        const bool isHttp = (url.rfind(L"http://", 0) == 0 || url.rfind(L"https://", 0) == 0);
        if (!isHttp)
        {
            std::wstring path = url;
            if (url.rfind(L"file://", 0) == 0)
            {
                path = url.substr(7);
                if (!path.empty() && path[0] == L'/' && path.size() > 2 && path[2] == L':')
                {
                    path = path.substr(1);
                }
            }

            std::ifstream f(std::filesystem::path(path), std::ios::binary);
            if (!f.is_open())
            {
                return false;
            }
            outData.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            return true;
        }

        HINTERNET hInternet = InternetOpenW(L"Novadesk WebFetch", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
        if (!hInternet)
        {
            return false;
        }

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (url.rfind(L"https://", 0) == 0)
        {
            flags |= INTERNET_FLAG_SECURE;
        }
        HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0, flags, 0);
        if (!hUrl)
        {
            InternetCloseHandle(hInternet);
            return false;
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        {
            outData.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return true;
    }

} // namespace novadesk::shared::system
