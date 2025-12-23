/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <Windows.h>
#include <functional>
#include <map>

namespace TimerManager {
    struct TimerInfo {
        bool repeating;
        int callbackIdx;
    };

    /*
    ** Initialize the timer manager with the message window.
    */
    void Initialize(HWND hWnd);

    /*
    ** Set the handler function to call when a timer triggers.
    ** This bridges to the JS API layer.
    */
    void SetCallbackHandler(std::function<void(int)> handler);

    /*
    ** Register a new timer (timeout or interval).
    ** Returns a unique ID for the timer.
    */
    int Register(int ms, int callbackIdx, bool repeating);

    /*
    ** Clear a specific timer by its ID.
    */
    void Clear(int id);

    /*
    ** Clear all active timers.
    */
    void ClearAll();

    /*
    ** Queue a high-priority immediate callback (setImmediate).
    */
    int PushImmediate(int callbackIdx);

    /*
    ** Handle WM_TIMER messages.
    */
    void HandleTimer(UINT_PTR id);

    /*
    ** Handle custom window messages (for setImmediate).
    */
    void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
}
