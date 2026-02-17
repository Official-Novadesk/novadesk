/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "AppVolumeControl.h"
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <algorithm>
#include <shellapi.h>
#include <cstdio>

namespace {
    struct ComInit {
        HRESULT hr = E_FAIL;
        ComInit() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComInit() { if (SUCCEEDED(hr)) CoUninitialize(); }
        bool Ok() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
    };

    std::wstring ToLowerCopy(const std::wstring& s)
    {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::towlower);
        return out;
    }

    std::wstring FileNameFromPath(const std::wstring& path)
    {
        size_t pos = path.find_last_of(L"\\/");
        return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
    }

    std::wstring GetProcessNameByPid(DWORD pid)
    {
        std::wstring result, dummy;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) return result;

        wchar_t buf[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buf, &size)) {
            result = FileNameFromPath(buf);
        }
        CloseHandle(hProcess);
        return result;
    }

    std::wstring GetProcessPathByPid(DWORD pid)
    {
        std::wstring result;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) return result;

        wchar_t buf[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buf, &size)) {
            result = buf;
        }
        CloseHandle(hProcess);
        return result;
    }

    bool SaveIconToIcoFile(HICON hIcon, FILE* fp)
    {
        ICONINFO iconInfo = {};
        BITMAP bmColor = {};
        BITMAP bmMask = {};
        if (!fp || !hIcon || !GetIconInfo(hIcon, &iconInfo) ||
            !GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
            !GetObject(iconInfo.hbmMask, sizeof(bmMask), &bmMask)) {
            if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
            if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
            return false;
        }

        // Match plugin constraints: only 16/32-bit icons.
        if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32) {
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        HDC dc = GetDC(nullptr);
        if (!dc) {
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = {};
        BITMAPINFO* bmi = (BITMAPINFO*)bmiBytes;

        // Color bits
        memset(bmi, 0, sizeof(BITMAPINFO));
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
        int colorBytesCount = (int)bmi->bmiHeader.biSizeImage;
        if (colorBytesCount <= 0 || colorBytesCount > (64 * 1024 * 1024)) {
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        BYTE* colorBits = new BYTE[colorBytesCount];
        if (!GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi, DIB_RGB_COLORS)) {
            delete[] colorBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }

        // Mask bits
        memset(bmi, 0, sizeof(BITMAPINFO));
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
        int maskBytesCount = (int)bmi->bmiHeader.biSizeImage;
        if (maskBytesCount <= 0 || maskBytesCount > (64 * 1024 * 1024)) {
            delete[] colorBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        BYTE* maskBits = new BYTE[maskBytesCount];
        if (!GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi, DIB_RGB_COLORS)) {
            delete[] colorBits;
            delete[] maskBits;
            ReleaseDC(nullptr, dc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return false;
        }
        ReleaseDC(nullptr, dc);

        #pragma pack(push, 1)
        struct ICONDIRENTRY_LOCAL {
            BYTE bWidth;
            BYTE bHeight;
            BYTE bColorCount;
            BYTE bReserved;
            WORD wPlanes;
            WORD wBitCount;
            DWORD dwBytesInRes;
            DWORD dwImageOffset;
        };
        struct ICONDIR_LOCAL {
            WORD idReserved;
            WORD idType;
            WORD idCount;
            ICONDIRENTRY_LOCAL idEntries[1];
        };
        #pragma pack(pop)

        static_assert(sizeof(ICONDIRENTRY_LOCAL) == 16, "Invalid ICONDIRENTRY size");
        static_assert(sizeof(ICONDIR_LOCAL) == 22, "Invalid ICONDIR size");

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

    bool ExtractIconToIco(const std::wstring& filePath, int size, const std::wstring& outIcoPath)
    {
        HICON icon = nullptr;
        UINT extracted = PrivateExtractIconsW(
            filePath.c_str(),
            0,
            size,
            size,
            &icon,
            nullptr,
            1,
            LR_LOADTRANSPARENT);

        if (extracted == 0 || !icon) {
            SHFILEINFO shFileInfo = {};
            UINT flags = SHGFI_ICON;
            flags |= (size <= 16) ? SHGFI_SMALLICON : SHGFI_LARGEICON;
            if (!SHGetFileInfoW(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), flags)) {
                return false;
            }
            icon = shFileInfo.hIcon;
            if (!icon) return false;
        }

        FILE* fp = nullptr;
        errno_t error = _wfopen_s(&fp, outIcoPath.c_str(), L"wb");
        bool ok = false;
        if (error == 0 && fp) {
            ok = SaveIconToIcoFile(icon, fp);
            fclose(fp);
        }

        if (!ok) {
            FILE* clearFp = nullptr;
            if (_wfopen_s(&clearFp, outIcoPath.c_str(), L"wb") == 0 && clearFp) {
                // Clear previous icon file content to avoid stale icon.
                fwrite(outIcoPath.c_str(), 1, 1, clearFp);
                fclose(clearFp);
            }
        }
        DestroyIcon(icon);
        return ok;
    }

    bool EnsureIconDir(std::wstring& outDir)
    {
        wchar_t tempPath[MAX_PATH] = {};
        DWORD len = GetTempPathW(MAX_PATH, tempPath);
        if (len == 0 || len >= MAX_PATH) return false;
        outDir = std::wstring(tempPath) + L"Novadesk\\AppIcons\\";
        CreateDirectoryW((std::wstring(tempPath) + L"Novadesk").c_str(), nullptr);
        CreateDirectoryW(outDir.c_str(), nullptr);
        return true;
    }
}

bool AppVolumeControl::ListSessions(std::vector<SessionInfo>& sessions)
{
    sessions.clear();
    ComInit com;
    if (!com.Ok()) return false;

    IMMDeviceEnumerator* deviceEnum = nullptr;
    IMMDevice* device = nullptr;
    IAudioSessionManager2* manager2 = nullptr;
    IAudioSessionEnumerator* sessionEnum = nullptr;
    bool success = false;

    do {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum);
        if (FAILED(hr) || !deviceEnum) break;

        hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        if (FAILED(hr) || !device) break;

        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&manager2);
        if (FAILED(hr) || !manager2) break;

        hr = manager2->GetSessionEnumerator(&sessionEnum);
        if (FAILED(hr) || !sessionEnum) break;

        int count = 0;
        hr = sessionEnum->GetCount(&count);
        if (FAILED(hr)) break;

        for (int i = 0; i < count; ++i) {
            IAudioSessionControl* control = nullptr;
            IAudioSessionControl2* control2 = nullptr;
            ISimpleAudioVolume* simpleVol = nullptr;
            IAudioMeterInformation* meterInfo = nullptr;

            if (FAILED(sessionEnum->GetSession(i, &control)) || !control) continue;
            if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&control2)) || !control2) {
                control->Release();
                continue;
            }
            if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVol)) || !simpleVol) {
                control2->Release();
                control->Release();
                continue;
            }
            control->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&meterInfo);

            DWORD pid = 0;
            control2->GetProcessId(&pid);

            AudioSessionState state;
            if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive) {
                simpleVol->Release();
                control2->Release();
                control->Release();
                continue;
            }

            float vol = 0.0f;
            float peak = 0.0f;
            BOOL mute = FALSE;
            simpleVol->GetMasterVolume(&vol);
            simpleVol->GetMute(&mute);
            if (meterInfo) meterInfo->GetPeakValue(&peak);

            LPWSTR displayName = nullptr;
            std::wstring display;
            if (SUCCEEDED(control->GetDisplayName(&displayName)) && displayName) {
                display = displayName;
                CoTaskMemFree(displayName);
            }

            SessionInfo info;
            info.pid = pid;
            info.processName = GetProcessNameByPid(pid);
            info.filePath = GetProcessPathByPid(pid);
            info.fileName = FileNameFromPath(info.filePath);
            if (!info.filePath.empty()) {
                std::wstring iconDir;
                if (EnsureIconDir(iconDir)) {
                    info.iconPath = iconDir + L"pid_" + std::to_wstring(pid) + L"_48.ico";
                    if (!ExtractIconToIco(info.filePath, 48, info.iconPath)) {
                        info.iconPath.clear();
                    }
                }
            }
            info.displayName = display;
            info.volume = vol;
            info.peak = peak;
            info.muted = mute != FALSE;
            sessions.push_back(info);

            if (meterInfo) meterInfo->Release();
            simpleVol->Release();
            control2->Release();
            control->Release();
        }

        success = true;
    } while (false);

    if (sessionEnum) sessionEnum->Release();
    if (manager2) manager2->Release();
    if (device) device->Release();
    if (deviceEnum) deviceEnum->Release();
    return success;
}

bool AppVolumeControl::GetByPid(DWORD pid, float& outVolume, bool& outMuted, float& outPeak)
{
    std::vector<SessionInfo> sessions;
    if (!ListSessions(sessions)) return false;

    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    double peakMax = 0.0;
    for (const auto& s : sessions) {
        if (s.pid == pid) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            if (s.peak > peakMax) peakMax = s.peak;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = (float)(sum / (double)count);
    outMuted = mutedAny;
    outPeak = (float)peakMax;
    return true;
}

bool AppVolumeControl::GetByProcessName(const std::wstring& processName, float& outVolume, bool& outMuted, float& outPeak)
{
    std::vector<SessionInfo> sessions;
    if (!ListSessions(sessions)) return false;

    std::wstring target = ToLowerCopy(processName);
    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    double peakMax = 0.0;
    for (const auto& s : sessions) {
        if (ToLowerCopy(s.processName) == target) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            if (s.peak > peakMax) peakMax = s.peak;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = (float)(sum / (double)count);
    outMuted = mutedAny;
    outPeak = (float)peakMax;
    return true;
}

static bool SetForMatchingSessions(DWORD pid, const std::wstring& processName, bool byPid, float* setVolume, bool* setMute)
{
    ComInit com;
    if (!com.Ok()) return false;

    IMMDeviceEnumerator* deviceEnum = nullptr;
    IMMDevice* device = nullptr;
    IAudioSessionManager2* manager2 = nullptr;
    IAudioSessionEnumerator* sessionEnum = nullptr;
    bool anySet = false;
    std::wstring target = byPid ? L"" : ToLowerCopy(processName);

    do {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum);
        if (FAILED(hr) || !deviceEnum) break;
        hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        if (FAILED(hr) || !device) break;
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&manager2);
        if (FAILED(hr) || !manager2) break;
        hr = manager2->GetSessionEnumerator(&sessionEnum);
        if (FAILED(hr) || !sessionEnum) break;

        int count = 0;
        hr = sessionEnum->GetCount(&count);
        if (FAILED(hr)) break;

        for (int i = 0; i < count; ++i) {
            IAudioSessionControl* control = nullptr;
            IAudioSessionControl2* control2 = nullptr;
            ISimpleAudioVolume* simpleVol = nullptr;

            if (FAILED(sessionEnum->GetSession(i, &control)) || !control) continue;
            if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&control2)) || !control2) {
                control->Release();
                continue;
            }
            if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVol)) || !simpleVol) {
                control2->Release();
                control->Release();
                continue;
            }

            DWORD curPid = 0;
            control2->GetProcessId(&curPid);
            bool match = false;
            if (byPid) {
                match = (curPid == pid);
            } else {
                std::wstring name = ToLowerCopy(GetProcessNameByPid(curPid));
                match = (name == target);
            }

            if (match) {
                if (setVolume) simpleVol->SetMasterVolume(*setVolume, nullptr);
                if (setMute) simpleVol->SetMute(*setMute ? TRUE : FALSE, nullptr);
                anySet = true;
            }

            simpleVol->Release();
            control2->Release();
            control->Release();
        }
    } while (false);

    if (sessionEnum) sessionEnum->Release();
    if (manager2) manager2->Release();
    if (device) device->Release();
    if (deviceEnum) deviceEnum->Release();
    return anySet;
}

bool AppVolumeControl::SetVolumeByPid(DWORD pid, float volume01)
{
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(pid, L"", true, &volume01, nullptr);
}

bool AppVolumeControl::SetVolumeByProcessName(const std::wstring& processName, float volume01)
{
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(0, processName, false, &volume01, nullptr);
}

bool AppVolumeControl::SetMuteByPid(DWORD pid, bool mute)
{
    return SetForMatchingSessions(pid, L"", true, nullptr, &mute);
}

bool AppVolumeControl::SetMuteByProcessName(const std::wstring& processName, bool mute)
{
    return SetForMatchingSessions(0, processName, false, nullptr, &mute);
}
