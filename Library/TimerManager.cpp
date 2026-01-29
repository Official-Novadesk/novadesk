/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "TimerManager.h"
#include "Novadesk.h"
#include <map>

namespace TimerManager {
    static HWND s_Window = nullptr;
    static std::map<UINT_PTR, TimerInfo> s_Timers;
    static UINT_PTR s_NextId = 1000;
    static std::function<void(int)> s_JSHandler;

    static const UINT WM_TIMER_ACTION = WM_USER + 100;

    /*
    ** Initialize the timer manager with the message window.
    */
    void Initialize(HWND hWnd) {
        s_Window = hWnd;
    }

    /*
    ** Set the handler function to call when a timer triggers.
    ** This bridges to the JS API layer.
    */
    void SetCallbackHandler(std::function<void(int)> handler) {
        s_JSHandler = handler;
    }

    /*
    ** Register a new timer (timeout or interval).
    ** Returns a unique ID for the timer.
    */
    int Register(int ms, int callbackIdx, bool repeating) {
        if (!s_Window) return -1;

        UINT_PTR id = s_NextId++;
        s_Timers[id] = { repeating, callbackIdx };
        
        SetTimer(s_Window, id, ms, NULL);
        return (int)id;
    }

    /*
    ** Clear a specific timer by its ID.
    */
    void Clear(int id) {
        if (!s_Window) return;

        auto it = s_Timers.find((UINT_PTR)id);
        if (it != s_Timers.end()) {
            KillTimer(s_Window, (UINT_PTR)id);
            s_Timers.erase(it);
        }
    }

    /*
    ** Clear all active timers.
    */
    void ClearAll() {
        if (!s_Window) return;

        for (auto const& pair : s_Timers) {
            KillTimer(s_Window, pair.first);
        }
        s_Timers.clear();
    }

    /*
    ** Queue a high-priority immediate callback (setImmediate).
    */
    int PushImmediate(int callbackIdx) {
        if (!s_Window) return -1;

        UINT_PTR id = s_NextId++;
        s_Timers[id] = { false, callbackIdx };
        
        PostMessage(s_Window, WM_TIMER_ACTION, id, 0);
        return (int)id;
    }

    /*
    ** Handle WM_TIMER messages.
    */
    void HandleTimer(UINT_PTR id) {
        auto it = s_Timers.find(id);
        if (it != s_Timers.end()) {
            TimerInfo info = it->second;
            
            if (!info.repeating) {
                KillTimer(s_Window, id);
                int cbIdx = info.callbackIdx;
                s_Timers.erase(it); // Erase first to be safe
                if (s_JSHandler) s_JSHandler(cbIdx);
            } else {
                if (s_JSHandler) s_JSHandler(info.callbackIdx);
            }
        }
    }

    /*
    ** Handle custom window messages (for setImmediate).
    */
    void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
         if (message == WM_TIMER_ACTION) {
             UINT_PTR id = (UINT_PTR)wParam;
             auto it = s_Timers.find(id);
             if (it != s_Timers.end()) {
                 int cbIdx = it->second.callbackIdx;
                 s_Timers.erase(it);
                 if (s_JSHandler) s_JSHandler(cbIdx);
             }
         }
     }
 
     HWND GetWindow() {
         return s_Window;
     }
 }
