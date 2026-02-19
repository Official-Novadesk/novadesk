/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "NowPlayingMonitor.h"
#include "PathUtils.h"
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <roapi.h>
#include <thread>
#include <vector>
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

#pragma comment(lib, "runtimeobject.lib")

namespace {
    enum class NowPlayingAction {
        Play,
        Pause,
        PlayPause,
        Stop,
        Next,
        Previous,
        SetPosition,
        SetShuffle,
        ToggleShuffle,
        SetRepeat
    };

    struct ActionItem {
        NowPlayingAction action;
        int value;
        bool flag;
    };
}

struct NowPlayingMonitor::Impl {
    std::mutex statsMutex;
    Stats cachedStats{};

    std::mutex actionMutex;
    std::queue<ActionItem> actions;
    std::condition_variable actionSignal;

    bool stopWorker = false;
    std::thread worker;

    GlobalSystemMediaTransportControlsSessionManager manager{ nullptr };
    std::chrono::steady_clock::time_point lastRefreshTime{};
    bool hasLastRefreshTime = false;
    double localPositionSeconds = 0.0;
    std::chrono::steady_clock::time_point localPositionTime{};
    bool hasLocalPosition = false;
    std::wstring coverPath;
    std::wstring coverTrackKey;

    Impl() {
        ResetStats(cachedStats);
        worker = std::thread(&Impl::WorkerMain, this);
    }

    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(actionMutex);
            stopWorker = true;
        }
        actionSignal.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    static void ResetStats(Stats& stats) {
        stats.available = false;
        stats.player.clear();
        stats.artist.clear();
        stats.album.clear();
        stats.title.clear();
        stats.thumbnail.clear();
        stats.duration = 0;
        stats.position = 0;
        stats.progress = 0;
        stats.state = 0;
        stats.status = 0;
        stats.shuffle = false;
        stats.repeat = false;
    }

    static int TimeSpanToSeconds(TimeSpan span) {
        return (int)std::chrono::duration_cast<std::chrono::seconds>(span).count();
    }

    std::wstring BuildTrackKey(const Stats& stats) const {
        return stats.player + L"|" + stats.artist + L"|" + stats.album + L"|" + stats.title;
    }

    std::wstring EnsureCoverPath() {
        if (!coverPath.empty()) return coverPath;
        std::wstring baseDir = PathUtils::GetAppDataPath();
        if (baseDir.empty()) return L"";
        std::wstring dir = baseDir + L"NowPlaying\\";
        CreateDirectoryW(baseDir.c_str(), nullptr);
        CreateDirectoryW(dir.c_str(), nullptr);
        coverPath = dir + L"cover.jpg";
        return coverPath;
    }

    bool SaveThumbnail(IRandomAccessStreamReference const& thumb) {
        try {
            auto path = EnsureCoverPath();
            if (path.empty()) return false;
            auto stream = thumb.OpenReadAsync().get();
            uint64_t size = stream.Size();
            if (size == 0 || size > (16ULL * 1024ULL * 1024ULL)) return false;

            Buffer buffer((uint32_t)size);
            auto readBuffer = stream.ReadAsync(buffer, (uint32_t)size, InputStreamOptions::None).get();
            uint32_t length = readBuffer.Length();
            if (length == 0) return false;

            DataReader reader = DataReader::FromBuffer(readBuffer);
            std::vector<uint8_t> bytes(length);
            reader.ReadBytes(bytes);

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return false;
            out.write((const char*)bytes.data(), bytes.size());
            out.close();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void WorkerMain() {
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
        bool uninitialize = SUCCEEDED(hr);

        auto nextRefresh = std::chrono::steady_clock::now();

        while (true) {
            std::queue<ActionItem> pending;
            {
                std::unique_lock<std::mutex> lock(actionMutex);
                actionSignal.wait_until(lock, nextRefresh, [&] {
                    return stopWorker || !actions.empty();
                });

                if (stopWorker) {
                    break;
                }

                std::swap(pending, actions);
            }

            if (manager == nullptr) {
                try {
                    manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
                }
                catch (...) {
                    manager = nullptr;
                }
            }

            ProcessActions(pending);

            auto now = std::chrono::steady_clock::now();
            if (now >= nextRefresh) {
                RefreshStats();
                nextRefresh = now + std::chrono::milliseconds(500);
            }
        }

        if (uninitialize) {
            RoUninitialize();
        }
    }

    GlobalSystemMediaTransportControlsSession GetSession() {
        if (manager == nullptr) {
            return nullptr;
        }

        try {
            return manager.GetCurrentSession();
        }
        catch (...) {
            return nullptr;
        }
    }

    void ProcessActions(std::queue<ActionItem>& pending) {
        while (!pending.empty()) {
            auto action = pending.front();
            pending.pop();
            try {
                auto session = GetSession();
                switch (action.action) {
                case NowPlayingAction::Play:
                    if (session != nullptr) session.TryPlayAsync();
                    break;
                case NowPlayingAction::Pause:
                    if (session != nullptr) session.TryPauseAsync();
                    break;
                case NowPlayingAction::PlayPause:
                    if (session != nullptr) session.TryTogglePlayPauseAsync();
                    break;
                case NowPlayingAction::Stop:
                    if (session != nullptr) session.TryStopAsync();
                    break;
                case NowPlayingAction::Next:
                    if (session != nullptr) session.TrySkipNextAsync();
                    break;
                case NowPlayingAction::Previous:
                    if (session != nullptr) session.TrySkipPreviousAsync();
                    break;
                case NowPlayingAction::SetPosition:
                    if (session != nullptr) {
                        auto timeline = session.GetTimelineProperties();
                        int duration = 0;
                        if (timeline != nullptr) {
                            duration = (std::max)(0, TimeSpanToSeconds(timeline.EndTime()) - TimeSpanToSeconds(timeline.StartTime()));
                        }
                        int seconds = action.flag ? ((duration > 0) ? (duration * action.value / 100) : 0) : action.value;
                        seconds = (std::max)(0, (std::min)(duration > 0 ? duration : seconds, seconds));
                        auto ticks = TimeSpan(std::chrono::seconds(seconds)).count();
                        session.TryChangePlaybackPositionAsync(ticks);
                    }
                    break;
                case NowPlayingAction::SetShuffle:
                    if (session != nullptr) session.TryChangeShuffleActiveAsync(action.flag);
                    break;
                case NowPlayingAction::ToggleShuffle:
                    if (session != nullptr) {
                        auto playback = session.GetPlaybackInfo();
                        bool current = false;
                        if (playback != nullptr) {
                            auto shuffle = playback.IsShuffleActive();
                            if (shuffle != nullptr) current = shuffle.Value();
                        }
                        session.TryChangeShuffleActiveAsync(!current);
                    }
                    break;
                case NowPlayingAction::SetRepeat:
                    if (session != nullptr) {
                        MediaPlaybackAutoRepeatMode mode = MediaPlaybackAutoRepeatMode::None;
                        if (action.value == 1) mode = MediaPlaybackAutoRepeatMode::Track;
                        else if (action.value == 2) mode = MediaPlaybackAutoRepeatMode::List;
                        session.TryChangeAutoRepeatModeAsync(mode);
                    }
                    break;
                }
            }
            catch (...) {}
        }
    }

    void RefreshStats() {
        Stats stats{};
        ResetStats(stats);
        auto nowSteady = std::chrono::steady_clock::now();
        Stats previous = ReadStats();

        auto session = GetSession();
        if (session != nullptr) {
            stats.available = true;
            stats.status = 1;

            try {
                stats.player = session.SourceAppUserModelId().c_str();
            }
            catch (...) {}

            try {
                auto props = session.TryGetMediaPropertiesAsync().get();
                if (props != nullptr) {
                    stats.artist = props.Artist().c_str();
                    stats.album = props.AlbumTitle().c_str();
                    stats.title = props.Title().c_str();

                    std::wstring newTrackKey = BuildTrackKey(stats);
                    auto thumbnail = props.Thumbnail();
                    if (thumbnail != nullptr) {
                        if (newTrackKey != coverTrackKey || coverPath.empty()) {
                            if (SaveThumbnail(thumbnail)) {
                                coverTrackKey = newTrackKey;
                            }
                        }
                        stats.thumbnail = coverPath;
                    } else {
                        coverTrackKey.clear();
                    }
                }
            }
            catch (...) {}

            try {
                auto timeline = session.GetTimelineProperties();
                if (timeline != nullptr) {
                    stats.duration = (std::max)(0, TimeSpanToSeconds(timeline.EndTime()) - TimeSpanToSeconds(timeline.StartTime()));
                    stats.position = (std::max)(0, TimeSpanToSeconds(timeline.Position()));
                }
            }
            catch (...) {}

            try {
                auto playback = session.GetPlaybackInfo();
                if (playback != nullptr) {
                    auto playbackStatus = playback.PlaybackStatus();
                    if (playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                        stats.state = 1;
                    }
                    else if (playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused) {
                        stats.state = 2;
                    }
                    else {
                        stats.state = 0;
                    }

                    auto shuffle = playback.IsShuffleActive();
                    if (shuffle != nullptr) {
                        stats.shuffle = shuffle.Value();
                    }

                    auto repeat = playback.AutoRepeatMode();
                    if (repeat != nullptr) {
                        stats.repeat = repeat.Value() != MediaPlaybackAutoRepeatMode::None;
                    }
                }
            }
            catch (...) {}

            // Smooth timeline progress because some players update Position infrequently.
            if (stats.state == 1 && stats.duration > 0) {
                try {
                    auto timeline = session.GetTimelineProperties();
                    if (timeline != nullptr) {
                        auto lastUpdated = timeline.LastUpdatedTime().time_since_epoch();
                        auto now = winrt::clock::now().time_since_epoch();
                        int elapsed = (int)std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdated).count();
                        if (elapsed > 0 && elapsed <= 3) {
                            stats.position += elapsed;
                        }
                    }
                }
                catch (...) {}

                bool sameTrack =
                    !previous.title.empty() &&
                    previous.player == stats.player &&
                    previous.artist == stats.artist &&
                    previous.album == stats.album &&
                    previous.title == stats.title &&
                    previous.duration == stats.duration;
                if (!sameTrack || !hasLocalPosition) {
                    localPositionSeconds = (double)stats.position;
                    localPositionTime = nowSteady;
                    hasLocalPosition = true;
                } else {
                    double elapsedLocal = std::chrono::duration<double>(nowSteady - localPositionTime).count();
                    if (elapsedLocal > 0.0 && elapsedLocal < 5.0) {
                        localPositionSeconds += elapsedLocal;
                    }
                    localPositionTime = nowSteady;

                    int predicted = (int)localPositionSeconds;
                    if (stats.duration > 0) {
                        predicted = (std::min)(predicted, stats.duration);
                    }

                    // Prefer monotonic local movement when provider timeline is stale/broken.
                    if (stats.position < predicted || (stats.duration > 0 && stats.position >= stats.duration && predicted < stats.duration)) {
                        stats.position = predicted;
                    } else {
                        // Re-anchor only when provider is meaningfully ahead; don't reset on equal stale values.
                        if (stats.position > predicted + 1) {
                            localPositionSeconds = (double)stats.position;
                            localPositionTime = nowSteady;
                        }
                    }
                }
            } else {
                hasLocalPosition = false;
            }

            if (stats.duration > 0) {
                stats.position = (std::min)(stats.position, stats.duration);
                stats.progress = (std::min)(100, (stats.position * 100) / stats.duration);
            }
        }

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            cachedStats = stats;
        }
        lastRefreshTime = nowSteady;
        hasLastRefreshTime = true;
    }

    void QueueAction(NowPlayingAction action, int value = 0, bool flag = false) {
        std::lock_guard<std::mutex> lock(actionMutex);
        if (stopWorker) {
            return;
        }
        actions.push(ActionItem{ action, value, flag });
        actionSignal.notify_one();
    }

    Stats ReadStats() {
        std::lock_guard<std::mutex> lock(statsMutex);
        return cachedStats;
    }
};

NowPlayingMonitor::NowPlayingMonitor() : m_Impl(new Impl()) {}

NowPlayingMonitor::~NowPlayingMonitor() {
    delete m_Impl;
    m_Impl = nullptr;
}

NowPlayingMonitor::Stats NowPlayingMonitor::GetStats() {
    return m_Impl->ReadStats();
}

bool NowPlayingMonitor::Play() {
    m_Impl->QueueAction(NowPlayingAction::Play);
    return true;
}

bool NowPlayingMonitor::Pause() {
    m_Impl->QueueAction(NowPlayingAction::Pause);
    return true;
}

bool NowPlayingMonitor::PlayPause() {
    m_Impl->QueueAction(NowPlayingAction::PlayPause);
    return true;
}

bool NowPlayingMonitor::Stop() {
    m_Impl->QueueAction(NowPlayingAction::Stop);
    return true;
}

bool NowPlayingMonitor::Next() {
    m_Impl->QueueAction(NowPlayingAction::Next);
    return true;
}

bool NowPlayingMonitor::Previous() {
    m_Impl->QueueAction(NowPlayingAction::Previous);
    return true;
}

bool NowPlayingMonitor::SetPosition(int value, bool isPercent) {
    m_Impl->QueueAction(NowPlayingAction::SetPosition, value, isPercent);
    return true;
}

bool NowPlayingMonitor::SetShuffle(bool enabled) {
    m_Impl->QueueAction(NowPlayingAction::SetShuffle, 0, enabled);
    return true;
}

bool NowPlayingMonitor::ToggleShuffle() {
    m_Impl->QueueAction(NowPlayingAction::ToggleShuffle);
    return true;
}

bool NowPlayingMonitor::SetRepeat(int mode) {
    m_Impl->QueueAction(NowPlayingAction::SetRepeat, mode, false);
    return true;
}
