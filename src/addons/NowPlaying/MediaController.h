/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <winrt/Windows.Media.Control.h>
#include "ImageUtils.h"

namespace winrt
{
    using namespace Windows::Media::Control;
}

struct MediaStats
{
    bool available = false;
    std::string player;
    std::string artist;
    std::string album;
    std::string title;
    std::string thumbnail;
    int duration = 0;
    int position = 0;
    int progress = 0;
    int state = 0; // 0=Stopped, 1=Playing, 2=Paused
    int status = 0; // 0=Closed, 1=Opened
    bool shuffle = false;
    bool repeat = false;
    std::string genres;
};

enum class MediaAction
{
    Play,
    Pause,
    PlayPause,
    Stop,
    Next,
    Previous,
    SetPosition,
    SetShuffle,
    SetRepeat
};

struct MediaActionItem
{
    MediaAction action;
    int value;
    bool flag;
};

class MediaController
{
public:
    MediaController();
    ~MediaController();

    MediaStats GetStats();
    void QueueAction(MediaAction action, int value = 0, bool flag = false);

private:
    void WorkerThread();
    void UpdateData();
    void UpdateCover(const winrt::hstring& playerAppId, const winrt::GlobalSystemMediaTransportControlsSessionMediaProperties& props);
    void ProcessActions(std::queue<MediaActionItem> &pending);

    winrt::GlobalSystemMediaTransportControlsSessionManager m_manager{ nullptr };
    
    std::mutex m_statsMutex;
    MediaStats m_stats;

    std::mutex m_actionMutex;
    std::condition_variable m_actionCv;
    std::queue<MediaActionItem> m_actions;
    bool m_stop = false;
    std::thread m_worker;

    // GDI+
    ULONG_PTR m_gdiToken;

    // Tracker for cover art
    winrt::hstring m_lastCoverPath;
    std::wstring m_lastTrackId;

    // Tracker for position prediction (robust handling for Chrome/Edge)
    double m_localPosSec = 0.0;
    std::chrono::steady_clock::time_point m_localPosTime{};
    bool m_hasLocalPos = false;
    std::wstring m_prevSyncTrackId;
    int m_prevSyncDuration = 0;
    int64_t m_lockedStartTime = -1;
};
