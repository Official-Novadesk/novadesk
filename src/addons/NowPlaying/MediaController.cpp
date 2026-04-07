/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "MediaController.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <chrono>
#include <algorithm>
#include <string_view>

using namespace winrt;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Foundation;

MediaController::MediaController()
{
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&m_gdiToken, &gdiplusStartupInput, NULL);

    m_worker = std::thread(&MediaController::WorkerThread, this);
}

MediaController::~MediaController()
{
    {
        std::lock_guard<std::mutex> lock(m_actionMutex);
        m_stop = true;
    }
    m_actionCv.notify_all();
    if (m_worker.joinable())
        m_worker.join();

    Gdiplus::GdiplusShutdown(m_gdiToken);
}

MediaStats MediaController::GetStats()
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void MediaController::QueueAction(MediaAction action, int value, bool flag)
{
    {
        std::lock_guard<std::mutex> lock(m_actionMutex);
        m_actions.push({ action, value, flag });
    }
    m_actionCv.notify_one();
}

void MediaController::WorkerThread()
{
    // Initialize WinRT for this thread
    winrt::init_apartment();

    try {
        m_manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    } catch (...) {
        m_manager = nullptr;
    }

    auto nextRefresh = std::chrono::steady_clock::now();

    while (true)
    {
        std::queue<MediaActionItem> pending;
        {
            std::unique_lock<std::mutex> lock(m_actionMutex);
            m_actionCv.wait_until(lock, nextRefresh, [this] { return m_stop || !m_actions.empty(); });

            if (m_stop)
                break;

            if (!m_actions.empty())
            {
                std::swap(pending, m_actions);
            }
        }

        if (!pending.empty())
        {
            ProcessActions(pending);
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= nextRefresh)
        {
            UpdateData();
            nextRefresh = now + std::chrono::milliseconds(500);
        }
    }

    winrt::uninit_apartment();
}

void MediaController::UpdateData()
{
    if (!m_manager) {
        try {
            m_manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        } catch (...) {
            return;
        }
    }

    if (!m_manager) return;

    MediaStats newStats;
    auto session = m_manager.GetCurrentSession();
    
    // Fallback: If no current session, try to find ANY playing session from the list
    if (!session)
    {
        try {
            auto sessions = m_manager.GetSessions();
            for (auto const& s : sessions)
            {
                if (s) {
                    auto pb = s.GetPlaybackInfo();
                    if (pb && pb.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                        session = s;
                        break;
                    }
                }
            }
            // If still no session found, just take the first one available
            if (!session && sessions.Size() > 0) {
                session = sessions.GetAt(0);
            }
        } catch (...) {}
    }

    if (session)
    {
        try
        {
            newStats.available = true;
            newStats.status = 1;
            newStats.player = winrt::to_string(session.SourceAppUserModelId());
            // 1. Get Playback Info first (State, Shuffle, Repeat)
            auto pbInfo = session.GetPlaybackInfo();
            if (pbInfo)
            {
                auto statusCode = pbInfo.PlaybackStatus();
                if (statusCode == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
                    newStats.state = 1;
                else if (statusCode == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused)
                    newStats.state = 2;
                else
                    newStats.state = 0;

                auto shuffle = pbInfo.IsShuffleActive();
                if (shuffle) newStats.shuffle = shuffle.Value();

                auto repeat = pbInfo.AutoRepeatMode();
                if (repeat) newStats.repeat = repeat.Value() != winrt::Windows::Media::MediaPlaybackAutoRepeatMode::None;
            }

            // 2. Get Metadata (Title, Artist, etc.)
            auto props = session.TryGetMediaPropertiesAsync().get();
            if (props)
            {
                newStats.artist = winrt::to_string(props.Artist());
                newStats.album = winrt::to_string(props.AlbumTitle());
                newStats.title = winrt::to_string(props.Title());

                std::string genres;
                for (auto g : props.Genres())
                {
                    if (!genres.empty()) genres += ", ";
                    genres += winrt::to_string(g);
                }
                newStats.genres = genres;

                UpdateCover(session.SourceAppUserModelId(), props);
                newStats.thumbnail = winrt::to_string(m_lastCoverPath);
            }

            // 3. Get Timeline (Duration, Position)
            auto timeline = session.GetTimelineProperties();
            if (timeline)
            {
                auto duration = timeline.EndTime() - timeline.StartTime();
                newStats.duration = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());

                std::wstring currentTrackId = session.SourceAppUserModelId().c_str();
                currentTrackId += L"|";
                if (props) {
                    currentTrackId += props.Artist().c_str();
                    currentTrackId += L"|";
                    currentTrackId += props.Title().c_str();
                }
                currentTrackId += L"|";
                currentTrackId += std::to_wstring(newStats.duration);

                bool trackChanged = (currentTrackId != m_prevSyncTrackId);
                m_prevSyncTrackId = currentTrackId;

                if (trackChanged || m_lockedStartTime == -1)
                {
                    m_lockedStartTime = timeline.StartTime().count();
                    m_hasLocalPos = false;
                }

                int64_t relativeTicks = timeline.Position().count() - m_lockedStartTime;
                newStats.position = static_cast<int>(relativeTicks / 10000000);
                if (newStats.position < 0) newStats.position = 0;

                const auto nowSteady = std::chrono::steady_clock::now();
                if (newStats.state == 1 && newStats.duration > 0)
                {
                    if (!m_hasLocalPos)
                    {
                        m_localPosSec = static_cast<double>(newStats.position);
                        m_localPosTime = nowSteady;
                        m_hasLocalPos = true;
                    }
                    else
                    {
                        double elapsed = std::chrono::duration<double>(nowSteady - m_localPosTime).count();
                        m_localPosTime = nowSteady;
                        if (elapsed > 0 && elapsed < 5.0) m_localPosSec += elapsed;

                        int predicted = static_cast<int>(m_localPosSec);

                        if (newStats.position > predicted)
                            m_localPosSec = static_cast<double>(newStats.position);
                        else if (predicted > newStats.position + 10)
                            m_localPosSec = static_cast<double>(newStats.position);
                        else
                            newStats.position = (std::min)(predicted, newStats.duration);
                    }
                }
                else
                {
                    m_hasLocalPos = false;
                }

                if (newStats.duration > 0)
                {
                    newStats.position = (std::min)(newStats.position, newStats.duration);
                    newStats.progress = (newStats.position * 100) / newStats.duration;
                }
            }
        }
        catch (...) {}
    }

    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats = newStats;
}

void MediaController::UpdateCover(const winrt::hstring& playerAppId, const winrt::GlobalSystemMediaTransportControlsSessionMediaProperties& props)
{
    std::wstring trackId = playerAppId.c_str();
    trackId += L"|";
    trackId += props.Artist().c_str();
    trackId += L"|";
    trackId += props.Title().c_str();

    if (trackId == m_lastTrackId && !m_lastCoverPath.empty())
        return;

    m_lastTrackId = trackId;
    auto thumb = props.Thumbnail();
    if (thumb)
    {
        auto path = ImageUtils::SaveCover(thumb);
        if (!path.empty())
        {
            // Advanced cropping from reference project
            // Spotify check (Reference says Spotify.exe, SMTC usually gives AppId)
            bool isSpotify = (std::wstring_view(playerAppId).find(L"Spotify") != std::wstring_view::npos);
            
            if (isSpotify && ImageUtils::CoverHasTransparentBorder(path))
            {
                path = ImageUtils::CropCover(path);
            }
            m_lastCoverPath = path;
        }
        else
        {
            m_lastCoverPath = L"";
        }
    }
    else
    {
        m_lastCoverPath = L"";
    }
}

void MediaController::ProcessActions(std::queue<MediaActionItem> &pending)
{
    auto session = m_manager ? m_manager.GetCurrentSession() : nullptr;
    if (!session) return;

    while (!pending.empty())
    {
        auto item = pending.front();
        pending.pop();

        try
        {
            switch (item.action)
            {
            case MediaAction::Play: session.TryPlayAsync(); break;
            case MediaAction::Pause: session.TryPauseAsync(); break;
            case MediaAction::PlayPause: session.TryTogglePlayPauseAsync(); break;
            case MediaAction::Stop: session.TryStopAsync(); break;
            case MediaAction::Next: session.TrySkipNextAsync(); break;
            case MediaAction::Previous: session.TrySkipPreviousAsync(); break;
            case MediaAction::SetPosition:
            {
                auto timeline = session.GetTimelineProperties();
                if (timeline)
                {
                    auto duration = timeline.EndTime() - timeline.StartTime();
                    auto durSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                    int64_t targetSec = item.value;
                    if (item.flag) // isPercent
                    {
                        targetSec = (durSec * item.value) / 100;
                    }
                    auto absPos = timeline.StartTime() + std::chrono::seconds(targetSec);
                    session.TryChangePlaybackPositionAsync(absPos.count());
                }
                break;
            }
            case MediaAction::SetShuffle:
                session.TryChangeShuffleActiveAsync(item.flag);
                break;
            case MediaAction::SetRepeat:
            {
                auto mode = winrt::Windows::Media::MediaPlaybackAutoRepeatMode::None;
                if (item.value == 1) mode = winrt::Windows::Media::MediaPlaybackAutoRepeatMode::Track;
                else if (item.value == 2) mode = winrt::Windows::Media::MediaPlaybackAutoRepeatMode::List;
                session.TryChangeAutoRepeatModeAsync(mode);
                break;
            }
            }
        }
        catch (...) {}
    }
}
