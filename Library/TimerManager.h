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

    void Initialize(HWND hWnd);

    void SetCallbackHandler(std::function<void(int)> handler);

    int Register(int ms, int callbackIdx, bool repeating);

    void Clear(int id);

    void ClearAll();

    int PushImmediate(int callbackIdx);

    void HandleTimer(UINT_PTR id);

    void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
 
     HWND GetWindow();
 }
