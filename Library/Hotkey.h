/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <functional>

struct Hotkey {
    int id;
    std::vector<short> virtualKeys; // Sorted list of VK codes
    int keyDownCallbackIdx;        // Callback ID in JS-side map (-1 if none)
    int keyUpCallbackIdx;          // Callback ID in JS-side map (-1 if none)
    bool isPressed;                // Current state tracked by hook
};

class HotkeyManager {
public:
    static int Register(const std::wstring& hotkeyStr, int keyDownIdx, int keyUpIdx);
    static bool Unregister(int id);
    static void UnregisterAll();
    
    // Callback to be set by JSApi to bridge back to JavaScript
    static void SetCallbackHandler(std::function<void(int)> handler);

private:
    static LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static void CallJS(int callbackIdx);

    static std::vector<Hotkey> s_Hotkeys;
    static HHOOK s_KeyboardHook;
    static int s_NextHotkeyId;
    static std::function<void(int)> s_JSHandler;
};
