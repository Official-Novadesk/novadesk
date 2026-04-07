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
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <objbase.h>
#include <shellapi.h>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

const NovadeskHostAPI *g_Host = nullptr;

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
static const IID IID_IAudioMeterInformation_Local =
    {0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}};

namespace
{
    struct AppVolumeSessionInfo
    {
        uint32_t pid = 0;
        std::wstring processName;
        std::wstring fileName;
        std::wstring filePath;
        std::wstring iconPath;
        std::wstring displayName;
        float volume = 0.0f;
        float peak = 0.0f;
        bool muted = false;
    };

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

    std::wstring ToLowerCopy(const std::wstring &s)
    {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::towlower);
        return out;
    }

    std::wstring FileNameFromPath(const std::wstring &path)
    {
        size_t pos = path.find_last_of(L"\\/");
        return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
    }

    std::wstring GetProcessPathByPid(DWORD pid)
    {
        std::wstring result;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process)
            return result;
        wchar_t buf[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(process, 0, buf, &size))
        {
            result = buf;
        }
        CloseHandle(process);
        return result;
    }

    std::wstring Utf8ToWide(const char *s)
    {
        if (!s || !*s)
            return L"";
        const int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (len <= 1)
            return L"";
        std::wstring out(static_cast<size_t>(len - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
        return out;
    }

    std::string WideToUtf8(const std::wstring &s)
    {
        if (s.empty())
            return std::string();
        const int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1)
            return std::string();
        std::string out(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), len, nullptr, nullptr);
        return out;
    }

    bool SaveIconToIcoFile(HICON hIcon, FILE *fp)
    {
        ICONINFO iconInfo = {};
        BITMAP bmColor = {};
        BITMAP bmMask = {};
        if (!fp || !hIcon || !GetIconInfo(hIcon, &iconInfo) ||
            !GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
            !GetObject(iconInfo.hbmMask, sizeof(bmMask), &bmMask))
        {
            if (iconInfo.hbmColor)
                DeleteObject(iconInfo.hbmColor);
            if (iconInfo.hbmMask)
                DeleteObject(iconInfo.hbmMask);
            return false;
        }

        if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32)
        {
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        HDC dc = GetDC(nullptr);
        if (!dc)
        {
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = {};
        BITMAPINFO *bmi = (BITMAPINFO *)bmiBytes;

        memset(bmi, 0, sizeof(BITMAPINFO));
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
        int colorBytesCount = (int)bmi->bmiHeader.biSizeImage;
        if (colorBytesCount <= 0 || colorBytesCount > (64 * 1024 * 1024))
        {
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        BYTE *colorBits = new BYTE[colorBytesCount];
        if (!GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi, DIB_RGB_COLORS))
        {
            delete[] colorBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        memset(bmi, 0, sizeof(BITMAPINFO));
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
        int maskBytesCount = (int)bmi->bmiHeader.biSizeImage;
        if (maskBytesCount <= 0 || maskBytesCount > (64 * 1024 * 1024))
        {
            delete[] colorBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        BYTE *maskBits = new BYTE[maskBytesCount];
        if (!GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi, DIB_RGB_COLORS))
        {
            delete[] colorBits;
            delete[] maskBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        ReleaseDC(nullptr, dc);

#pragma pack(push, 1)
        struct ICONDIRENTRY_LOCAL
        {
            BYTE bWidth;
            BYTE bHeight;
            BYTE bColorCount;
            BYTE bReserved;
            WORD wPlanes;
            WORD wBitCount;
            DWORD dwBytesInRes;
            DWORD dwImageOffset;
        };
        struct ICONDIR_LOCAL
        {
            WORD idReserved;
            WORD idType;
            WORD idCount;
            ICONDIRENTRY_LOCAL idEntries[1];
        };
#pragma pack(pop)

        BITMAPINFOHEADER bmihIcon = {};
        bmihIcon.biSize = sizeof(BITMAPINFOHEADER);
        bmihIcon.biWidth = bmColor.bmWidth;
        bmihIcon.biHeight = bmColor.bmHeight * 2;
        bmihIcon.biPlanes = bmColor.bmPlanes;
        bmihIcon.biBitCount = bmColor.bmBitsPixel;
        bmihIcon.biSizeImage = colorBytesCount + maskBytesCount;

        ICONDIR_LOCAL dir = {};
        dir.idReserved = 0;
        dir.idType = 1;
        dir.idCount = 1;
        dir.idEntries[0].bWidth = (BYTE)bmColor.bmWidth;
        dir.idEntries[0].bHeight = (BYTE)bmColor.bmHeight;
        dir.idEntries[0].bColorCount = 0;
        dir.idEntries[0].bReserved = 0;
        dir.idEntries[0].wPlanes = bmColor.bmPlanes;
        dir.idEntries[0].wBitCount = bmColor.bmBitsPixel;
        dir.idEntries[0].dwBytesInRes = sizeof(bmihIcon) + bmihIcon.biSizeImage;
        dir.idEntries[0].dwImageOffset = sizeof(ICONDIR_LOCAL);

        fwrite(&dir, 1, sizeof(dir), fp);
        fwrite(&bmihIcon, 1, sizeof(bmihIcon), fp);
        fwrite(colorBits, 1, colorBytesCount, fp);
        fwrite(maskBits, 1, maskBytesCount, fp);

        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);
        delete[] colorBits;
        delete[] maskBits;
        return true;
    }

    bool ExtractFileIconToIco(const std::wstring &filePath, const std::wstring &outIcoPath, int size)
    {
        if (filePath.empty() || outIcoPath.empty())
            return false;
        if (size <= 0)
            size = 64;
        if (size > 256)
            size = 256;

        const int candidates[] = {size, 32, 48, 64};
        for (int s : candidates)
        {
            if (s <= 0 || s > 256)
                continue;

            HICON icon = nullptr;
            UINT extracted = PrivateExtractIconsW(
                filePath.c_str(),
                0,
                s,
                s,
                &icon,
                nullptr,
                1,
                LR_LOADTRANSPARENT);

            if (extracted == 0 || !icon)
            {
                SHFILEINFO shFileInfo = {};
                UINT flags = SHGFI_ICON;
                flags |= (s <= 16) ? SHGFI_SMALLICON : SHGFI_LARGEICON;
                if (!SHGetFileInfoW(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), flags))
                {
                    continue;
                }
                icon = shFileInfo.hIcon;
                if (!icon)
                    continue;
            }

            FILE *fp = nullptr;
            errno_t error = _wfopen_s(&fp, outIcoPath.c_str(), L"wb");
            bool ok = false;
            if (error == 0 && fp)
            {
                ok = SaveIconToIcoFile(icon, fp);
                fclose(fp);
            }
            DestroyIcon(icon);

            if (ok)
                return true;
        }

        FILE *clearFp = nullptr;
        if (_wfopen_s(&clearFp, outIcoPath.c_str(), L"wb") == 0 && clearFp)
        {
            fwrite(outIcoPath.c_str(), 1, 1, clearFp);
            fclose(clearFp);
        }
        return false;
    }

    bool EnsureDirectory(const std::wstring &path)
    {
        if (path.empty())
            return false;
        DWORD attrs = GetFileAttributesW(path.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
            return true;
        return CreateDirectoryW(path.c_str(), nullptr) != 0;
    }

    bool FileExistsNonEmpty(const std::wstring &path)
    {
        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
            return false;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return false;
        ULONGLONG size = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        return size > 0;
    }

    std::wstring GetIconCachePath(const std::wstring &filePath)
    {
        wchar_t tmpPath[MAX_PATH] = {};
        if (!GetTempPathW(MAX_PATH, tmpPath))
            return L"";

        std::wstring base = tmpPath;
        if (!base.empty() && base.back() != L'\\')
            base.push_back(L'\\');
        base += L"novadesk_appvolume_icons";
        if (!EnsureDirectory(base))
            return L"";

        std::hash<std::wstring> hasher;
        size_t h = hasher(filePath);
        std::wstring name = L"icon_";
        name += std::to_wstring(static_cast<unsigned long long>(h));
        name += L"_64.ico";
        return base + L"\\" + name;
    }

    std::wstring ResolveIconPath(const std::wstring &filePath)
    {
        if (filePath.empty())
            return L"";

        std::wstring outPath = GetIconCachePath(filePath);
        if (outPath.empty())
            return L"";

        static std::unordered_map<std::wstring, std::wstring> cache;
        auto it = cache.find(filePath);
        if (it != cache.end() && FileExistsNonEmpty(it->second))
            return it->second;

        if (FileExistsNonEmpty(outPath) || ExtractFileIconToIco(filePath, outPath, 64))
        {
            cache[filePath] = outPath;
            return outPath;
        }
        return L"";
    }

    bool SetForMatchingSessions(DWORD pid, const std::wstring &processName, bool byPid, float *setVolume, bool *setMute)
    {
        ComInit com;
        if (!com.Ok())
            return false;

        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *device = nullptr;
        IAudioSessionManager2 *manager2 = nullptr;
        IAudioSessionEnumerator *sessionEnum = nullptr;
        bool anySet = false;
        std::wstring target = byPid ? L"" : ToLowerCopy(processName);

        do
        {
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&deviceEnum);
            if (FAILED(hr) || !deviceEnum)
                break;
            hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
            if (FAILED(hr) || !device)
                break;
            hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void **)&manager2);
            if (FAILED(hr) || !manager2)
                break;
            hr = manager2->GetSessionEnumerator(&sessionEnum);
            if (FAILED(hr) || !sessionEnum)
                break;

            int count = 0;
            hr = sessionEnum->GetCount(&count);
            if (FAILED(hr))
                break;

            for (int i = 0; i < count; ++i)
            {
                IAudioSessionControl *control = nullptr;
                IAudioSessionControl2 *control2 = nullptr;
                ISimpleAudioVolume *simpleVol = nullptr;

                if (FAILED(sessionEnum->GetSession(i, &control)) || !control)
                    continue;
                if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&control2)) || !control2)
                {
                    control->Release();
                    continue;
                }
                if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&simpleVol)) || !simpleVol)
                {
                    control2->Release();
                    control->Release();
                    continue;
                }

                DWORD curPid = 0;
                control2->GetProcessId(&curPid);
                bool match = false;
                if (byPid)
                {
                    match = (curPid == pid);
                }
                else
                {
                    std::wstring curName = ToLowerCopy(FileNameFromPath(GetProcessPathByPid(curPid)));
                    match = (curName == target);
                }

                if (match)
                {
                    if (setVolume)
                        simpleVol->SetMasterVolume(*setVolume, nullptr);
                    if (setMute)
                        simpleVol->SetMute(*setMute ? TRUE : FALSE, nullptr);
                    anySet = true;
                }

                simpleVol->Release();
                control2->Release();
                control->Release();
            }
        } while (false);

        if (sessionEnum)
            sessionEnum->Release();
        if (manager2)
            manager2->Release();
        if (device)
            device->Release();
        if (deviceEnum)
            deviceEnum->Release();
        return anySet;
    }

    bool AppVolumeListSessions(std::vector<AppVolumeSessionInfo> &sessions)
    {
        sessions.clear();
        ComInit com;
        if (!com.Ok())
            return false;

        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *device = nullptr;
        IAudioSessionManager2 *manager2 = nullptr;
        IAudioSessionEnumerator *sessionEnum = nullptr;
        bool success = false;

        do
        {
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&deviceEnum);
            if (FAILED(hr) || !deviceEnum)
                break;

            hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
            if (FAILED(hr) || !device)
                break;

            hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void **)&manager2);
            if (FAILED(hr) || !manager2)
                break;

            hr = manager2->GetSessionEnumerator(&sessionEnum);
            if (FAILED(hr) || !sessionEnum)
                break;

            int count = 0;
            hr = sessionEnum->GetCount(&count);
            if (FAILED(hr))
                break;

            for (int i = 0; i < count; ++i)
            {
                IAudioSessionControl *control = nullptr;
                IAudioSessionControl2 *control2 = nullptr;
                ISimpleAudioVolume *simpleVol = nullptr;
                IAudioMeterInformation *meter = nullptr;

                if (FAILED(sessionEnum->GetSession(i, &control)) || !control)
                    continue;
                if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&control2)) || !control2)
                {
                    control->Release();
                    continue;
                }
                if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&simpleVol)) || !simpleVol)
                {
                    control2->Release();
                    control->Release();
                    continue;
                }

                AudioSessionState state{};
                if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive)
                {
                    simpleVol->Release();
                    control2->Release();
                    control->Release();
                    continue;
                }

                DWORD pid = 0;
                control2->GetProcessId(&pid);

                float volume = 0.0f;
                float peak = 0.0f;
                BOOL muted = FALSE;
                simpleVol->GetMasterVolume(&volume);
                simpleVol->GetMute(&muted);
                if (SUCCEEDED(control2->QueryInterface(IID_IAudioMeterInformation_Local, (void **)&meter)) && meter)
                {
                    float meterPeak = 0.0f;
                    if (SUCCEEDED(meter->GetPeakValue(&meterPeak)))
                    {
                        peak = meterPeak;
                    }
                }

                LPWSTR displayName = nullptr;
                std::wstring display;
                if (SUCCEEDED(control->GetDisplayName(&displayName)) && displayName)
                {
                    display = displayName;
                    CoTaskMemFree(displayName);
                }

                AppVolumeSessionInfo info;
                info.pid = static_cast<uint32_t>(pid);
                info.filePath = GetProcessPathByPid(pid);
                info.fileName = FileNameFromPath(info.filePath);
                info.processName = info.fileName;
                info.iconPath = ResolveIconPath(info.filePath);
                info.displayName = display;
                info.volume = volume;
                info.peak = peak;
                info.muted = muted != FALSE;

                sessions.push_back(info);

                if (meter)
                    meter->Release();
                simpleVol->Release();
                control2->Release();
                control->Release();
            }

            success = true;
        } while (false);

        if (sessionEnum)
            sessionEnum->Release();
        if (manager2)
            manager2->Release();
        if (device)
            device->Release();
        if (deviceEnum)
            deviceEnum->Release();
        return success;
    }

    bool AppVolumeGetByPid(uint32_t pid, float &outVolume, bool &outMuted, float &outPeak)
    {
        std::vector<AppVolumeSessionInfo> sessions;
        if (!AppVolumeListSessions(sessions))
            return false;

        double sum = 0.0;
        int count = 0;
        bool mutedAny = false;
        double peakMax = 0.0;
        for (const auto &s : sessions)
        {
            if (s.pid == pid)
            {
                sum += s.volume;
                mutedAny = mutedAny || s.muted;
                if (s.peak > peakMax)
                    peakMax = s.peak;
                ++count;
            }
        }
        if (count == 0)
            return false;

        outVolume = static_cast<float>(sum / static_cast<double>(count));
        outMuted = mutedAny;
        outPeak = static_cast<float>(peakMax);
        return true;
    }

    bool AppVolumeGetByProcessName(const std::wstring &processName, float &outVolume, bool &outMuted, float &outPeak)
    {
        std::vector<AppVolumeSessionInfo> sessions;
        if (!AppVolumeListSessions(sessions))
            return false;

        std::wstring target = ToLowerCopy(processName);
        double sum = 0.0;
        int count = 0;
        bool mutedAny = false;
        double peakMax = 0.0;
        for (const auto &s : sessions)
        {
            if (ToLowerCopy(s.processName) == target)
            {
                sum += s.volume;
                mutedAny = mutedAny || s.muted;
                if (s.peak > peakMax)
                    peakMax = s.peak;
                ++count;
            }
        }
        if (count == 0)
            return false;

        outVolume = static_cast<float>(sum / static_cast<double>(count));
        outMuted = mutedAny;
        outPeak = static_cast<float>(peakMax);
        return true;
    }

    bool AppVolumeSetVolumeByPid(uint32_t pid, float volume01)
    {
        if (volume01 < 0.0f)
            volume01 = 0.0f;
        if (volume01 > 1.0f)
            volume01 = 1.0f;
        return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, &volume01, nullptr);
    }

    bool AppVolumeSetVolumeByProcessName(const std::wstring &processName, float volume01)
    {
        if (volume01 < 0.0f)
            volume01 = 0.0f;
        if (volume01 > 1.0f)
            volume01 = 1.0f;
        return SetForMatchingSessions(0, processName, false, &volume01, nullptr);
    }

    bool AppVolumeSetMuteByPid(uint32_t pid, bool mute)
    {
        return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, nullptr, &mute);
    }

    bool AppVolumeSetMuteByProcessName(const std::wstring &processName, bool mute)
    {
        return SetForMatchingSessions(0, processName, false, nullptr, &mute);
    }

    void RegisterSessionProps(novadesk_context ctx, const AppVolumeSessionInfo &s)
    {
        const std::string processName = WideToUtf8(s.processName);
        const std::string fileName = WideToUtf8(s.fileName);
        const std::string filePath = WideToUtf8(s.filePath);
        const std::string iconPath = WideToUtf8(s.iconPath);
        const std::string displayName = WideToUtf8(s.displayName);

        g_Host->RegisterNumber(ctx, "pid", static_cast<double>(s.pid));
        g_Host->RegisterString(ctx, "processName", processName.c_str());
        g_Host->RegisterString(ctx, "fileName", fileName.c_str());
        g_Host->RegisterString(ctx, "filePath", filePath.c_str());
        g_Host->RegisterString(ctx, "iconPath", iconPath.c_str());
        g_Host->RegisterString(ctx, "displayName", displayName.c_str());
        g_Host->RegisterNumber(ctx, "volume", static_cast<double>(s.volume));
        g_Host->RegisterNumber(ctx, "peak", static_cast<double>(s.peak));
        g_Host->RegisterBool(ctx, "muted", s.muted ? 1 : 0);
    }

    int JsAppVolumeListSessions(novadesk_context ctx)
    {
        std::vector<AppVolumeSessionInfo> sessions;
        if (!AppVolumeListSessions(sessions))
        {
            g_Host->PushArray(ctx);
            return 1;
        }

        g_Host->PushArray(ctx);
        for (const auto &s : sessions)
        {
            g_Host->ArrayPushObject(ctx);
            RegisterSessionProps(ctx, s);
            g_Host->Pop(ctx);
        }
        return 1;
    }

    int JsAppVolumeGetByPid(novadesk_context ctx)
    {
        if (!g_Host->IsNumber(ctx, 0))
        {
            g_Host->ThrowError(ctx, "appVolume.getByPid(pid) requires pid");
            return 0;
        }
        int pid = static_cast<int>(g_Host->GetNumber(ctx, 0));
        float volume = 0.0f;
        float peak = 0.0f;
        bool muted = false;
        if (!AppVolumeGetByPid(static_cast<uint32_t>(pid), volume, muted, peak))
        {
            g_Host->PushNull(ctx);
            return 1;
        }
        g_Host->PushObject(ctx);
        g_Host->RegisterNumber(ctx, "volume", static_cast<double>(volume));
        g_Host->RegisterNumber(ctx, "peak", static_cast<double>(peak));
        g_Host->RegisterBool(ctx, "muted", muted ? 1 : 0);
        return 1;
    }

    int JsAppVolumeGetByProcessName(novadesk_context ctx)
    {
        if (!g_Host->IsString(ctx, 0))
        {
            g_Host->ThrowError(ctx, "appVolume.getByProcessName(name) requires name");
            return 0;
        }
        const char *nameC = g_Host->GetString(ctx, 0);
        float volume = 0.0f;
        float peak = 0.0f;
        bool muted = false;
        if (!nameC || !AppVolumeGetByProcessName(Utf8ToWide(nameC), volume, muted, peak))
        {
            g_Host->PushNull(ctx);
            return 1;
        }
        g_Host->PushObject(ctx);
        g_Host->RegisterNumber(ctx, "volume", static_cast<double>(volume));
        g_Host->RegisterNumber(ctx, "peak", static_cast<double>(peak));
        g_Host->RegisterBool(ctx, "muted", muted ? 1 : 0);
        return 1;
    }

    int JsAppVolumeSetVolumeByPid(novadesk_context ctx)
    {
        if (!g_Host->IsNumber(ctx, 0) || !g_Host->IsNumber(ctx, 1))
        {
            g_Host->ThrowError(ctx, "appVolume.setVolumeByPid(pid, volume01) requires pid and volume");
            return 0;
        }
        int pid = static_cast<int>(g_Host->GetNumber(ctx, 0));
        float volume = static_cast<float>(g_Host->GetNumber(ctx, 1));
        g_Host->PushBool(ctx, AppVolumeSetVolumeByPid(static_cast<uint32_t>(pid), volume) ? 1 : 0);
        return 1;
    }

    int JsAppVolumeSetVolumeByProcessName(novadesk_context ctx)
    {
        if (!g_Host->IsString(ctx, 0) || !g_Host->IsNumber(ctx, 1))
        {
            g_Host->ThrowError(ctx, "appVolume.setVolumeByProcessName(name, volume01) requires name and volume");
            return 0;
        }
        const char *nameC = g_Host->GetString(ctx, 0);
        float volume = static_cast<float>(g_Host->GetNumber(ctx, 1));
        g_Host->PushBool(ctx, (nameC && AppVolumeSetVolumeByProcessName(Utf8ToWide(nameC), volume)) ? 1 : 0);
        return 1;
    }

    int JsAppVolumeSetMuteByPid(novadesk_context ctx)
    {
        if (!g_Host->IsNumber(ctx, 0) || g_Host->GetTop(ctx) < 2)
        {
            g_Host->ThrowError(ctx, "appVolume.setMuteByPid(pid, mute) requires pid and mute");
            return 0;
        }
        int pid = static_cast<int>(g_Host->GetNumber(ctx, 0));
        int mute = g_Host->GetBool(ctx, 1);
        g_Host->PushBool(ctx, AppVolumeSetMuteByPid(static_cast<uint32_t>(pid), mute != 0) ? 1 : 0);
        return 1;
    }

    int JsAppVolumeSetMuteByProcessName(novadesk_context ctx)
    {
        if (!g_Host->IsString(ctx, 0) || g_Host->GetTop(ctx) < 2)
        {
            g_Host->ThrowError(ctx, "appVolume.setMuteByProcessName(name, mute) requires name and mute");
            return 0;
        }
        const char *nameC = g_Host->GetString(ctx, 0);
        int mute = g_Host->GetBool(ctx, 1);
        g_Host->PushBool(ctx, (nameC && AppVolumeSetMuteByProcessName(Utf8ToWide(nameC), mute != 0)) ? 1 : 0);
        return 1;
    }
}

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host)
{
    (void)hMsgWnd;
    g_Host = host;

    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "AppVolume");
    addon.RegisterString("version", "1.0.0");

    addon.RegisterFunction("listSessions", JsAppVolumeListSessions, 0);
    addon.RegisterFunction("getByPid", JsAppVolumeGetByPid, 1);
    addon.RegisterFunction("getByProcessName", JsAppVolumeGetByProcessName, 1);
    addon.RegisterFunction("setVolumeByPid", JsAppVolumeSetVolumeByPid, 2);
    addon.RegisterFunction("setVolumeByProcessName", JsAppVolumeSetVolumeByProcessName, 2);
    addon.RegisterFunction("setMuteByPid", JsAppVolumeSetMuteByPid, 2);
    addon.RegisterFunction("setMuteByProcessName", JsAppVolumeSetMuteByProcessName, 2);
}

NOVADESK_ADDON_UNLOAD()
{
}
