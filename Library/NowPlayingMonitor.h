/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

class NowPlayingMonitor {
public:
    struct Stats {
        bool available;
        std::wstring player;
        std::wstring artist;
        std::wstring album;
        std::wstring title;
        std::wstring thumbnail;
        int duration;
        int position;
        int progress;
        int state;    // 0 stopped/unknown, 1 playing, 2 paused
        int status;   // 0 closed, 1 opened
        bool shuffle;
        bool repeat;
    };

    NowPlayingMonitor();
    ~NowPlayingMonitor();

    Stats GetStats();
    bool Play();
    bool Pause();
    bool PlayPause();
    bool Stop();
    bool Next();
    bool Previous();
    bool SetPosition(int value, bool isPercent);
    bool SetShuffle(bool enabled);
    bool ToggleShuffle();
    bool SetRepeat(int mode); // 0 none, 1 one, 2 all

private:
    struct Impl;
    Impl* m_Impl;
};
