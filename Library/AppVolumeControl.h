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
#include <string>
#include <vector>

class AppVolumeControl {
public:
    struct SessionInfo {
        DWORD pid = 0;
        std::wstring processName;
        std::wstring fileName;
        std::wstring filePath;
        std::wstring iconPath;
        std::wstring displayName;
        float volume = 0.0f;   // 0..1
        float peak = 0.0f;     // 0..1
        bool muted = false;
    };

    static bool ListSessions(std::vector<SessionInfo>& sessions);
    static bool GetByPid(DWORD pid, float& outVolume, bool& outMuted, float& outPeak);
    static bool GetByProcessName(const std::wstring& processName, float& outVolume, bool& outMuted, float& outPeak);
    static bool SetVolumeByPid(DWORD pid, float volume01);
    static bool SetVolumeByProcessName(const std::wstring& processName, float volume01);
    static bool SetMuteByPid(DWORD pid, bool mute);
    static bool SetMuteByProcessName(const std::wstring& processName, bool mute);
};
