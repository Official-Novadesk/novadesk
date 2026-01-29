/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Hotkey.h"
#include "Logging.h"
#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <sstream>
#include <map>

std::vector<Hotkey> HotkeyManager::s_Hotkeys;
HHOOK HotkeyManager::s_KeyboardHook = nullptr;
int HotkeyManager::s_NextHotkeyId = 1;
std::function<void(int)> HotkeyManager::s_JSHandler = nullptr;

// Mapping for special keys
static const std::map<std::wstring, short> s_KeyMap = {
    {L"BACKSPACE", VK_BACK}, {L"BACK", VK_BACK},
    {L"TAB", VK_TAB},
    {L"ENTER", VK_RETURN}, {L"RETURN", VK_RETURN},
    {L"SHIFT", VK_SHIFT}, {L"LSHIFT", VK_LSHIFT}, {L"RSHIFT", VK_RSHIFT},
    {L"CTRL", VK_CONTROL}, {L"LCTRL", VK_LCONTROL}, {L"RCTRL", VK_RCONTROL},
    {L"CONTROL", VK_CONTROL},
    {L"ALT", VK_MENU}, {L"LALT", VK_LMENU}, {L"RALT", VK_RMENU},
    {L"MENU", VK_MENU},
    {L"PAUSE", VK_PAUSE},
    {L"CAPSLOCK", VK_CAPITAL}, {L"CAPS", VK_CAPITAL},
    {L"ESC", VK_ESCAPE}, {L"ESCAPE", VK_ESCAPE},
    {L"SPACE", VK_SPACE},
    {L"PAGEUP", VK_PRIOR}, {L"PGUP", VK_PRIOR},
    {L"PAGEDOWN", VK_NEXT}, {L"PGDN", VK_NEXT},
    {L"END", VK_END},
    {L"HOME", VK_HOME},
    {L"LEFT", VK_LEFT}, {L"UP", VK_UP}, {L"RIGHT", VK_RIGHT}, {L"DOWN", VK_DOWN},
    {L"INS", VK_INSERT}, {L"INSERT", VK_INSERT},
    {L"DEL", VK_DELETE}, {L"DELETE", VK_DELETE},
    {L"WIN", VK_LWIN}, {L"LWIN", VK_LWIN}, {L"RWIN", VK_RWIN},
    {L"APPS", VK_APPS},
    {L"0", 0x30}, {L"1", 0x31}, {L"2", 0x32}, {L"3", 0x33}, {L"4", 0x34},
    {L"5", 0x35}, {L"6", 0x36}, {L"7", 0x37}, {L"8", 0x38}, {L"9", 0x39},
    {L"A", 0x41}, {L"B", 0x42}, {L"C", 0x43}, {L"D", 0x44}, {L"E", 0x45},
    {L"F", 0x46}, {L"G", 0x47}, {L"H", 0x48}, {L"I", 0x49}, {L"J", 0x4A},
    {L"K", 0x4B}, {L"L", 0x4C}, {L"M", 0x4D}, {L"N", 0x4E}, {L"O", 0x4F},
    {L"P", 0x50}, {L"Q", 0x51}, {L"R", 0x52}, {L"S", 0x53}, {L"T", 0x54},
    {L"U", 0x55}, {L"V", 0x56}, {L"W", 0x57}, {L"X", 0x58}, {L"Y", 0x59},
    {L"Z", 0x5A},
    {L"F1", VK_F1}, {L"F2", VK_F2}, {L"F3", VK_F3}, {L"F4", VK_F4}, {L"F5", VK_F5},
    {L"F6", VK_F6}, {L"F7", VK_F7}, {L"F8", VK_F8}, {L"F9", VK_F9}, {L"F10", VK_F10},
    {L"F11", VK_F11}, {L"F12", VK_F12}
};

void HotkeyManager::SetCallbackHandler(std::function<void(int)> handler) {
    s_JSHandler = handler;
}

int HotkeyManager::Register(const std::wstring& hotkeyStr, int keyDownIdx, int keyUpIdx) {
    std::vector<short> vks;
    std::wstringstream ss(hotkeyStr);
    std::wstring token;

    while (std::getline(ss, token, L'+')) {
        // Trim whitespace using iswspace from <cwctype>
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](wchar_t ch) { return !iswspace(ch); }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](wchar_t ch) { return !iswspace(ch); }).base(), token.end());

        // Convert to uppercase
        std::transform(token.begin(), token.end(), token.begin(), ::towupper);

        auto it = s_KeyMap.find(token);
        if (it != s_KeyMap.end()) {
            vks.push_back(it->second);
        } else if (token.length() == 1) {
            vks.push_back(LOBYTE(VkKeyScanW(token[0])));
        }
    }

    if (vks.empty()) return -1;

    Hotkey hk;
    hk.id = s_NextHotkeyId++;
    hk.virtualKeys = vks;
    hk.isPressed = false;
    hk.keyDownCallbackIdx = keyDownIdx;
    hk.keyUpCallbackIdx = keyUpIdx;

    s_Hotkeys.push_back(hk);

    if (!s_KeyboardHook) {
        s_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, GetModuleHandle(NULL), 0);
        if (!s_KeyboardHook) {
            Logging::Log(LogLevel::Error, L"Failed to start keyboard hook: %d", GetLastError());
        }
    }

    return hk.id;
}

bool HotkeyManager::Unregister(int id) {
    auto it = std::remove_if(s_Hotkeys.begin(), s_Hotkeys.end(), [id](const Hotkey& hk) { return hk.id == id; });
    if (it != s_Hotkeys.end()) {
        s_Hotkeys.erase(it, s_Hotkeys.end());
        
        if (s_Hotkeys.empty() && s_KeyboardHook) {
            UnhookWindowsHookEx(s_KeyboardHook);
            s_KeyboardHook = nullptr;
        }
        return true;
    }
    return false;
}

void HotkeyManager::UnregisterAll() {
    if (s_KeyboardHook) {
        UnhookWindowsHookEx(s_KeyboardHook);
        s_KeyboardHook = nullptr;
    }
    s_Hotkeys.clear();
}

LRESULT CALLBACK HotkeyManager::LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        short vk = static_cast<short>(kbd->vkCode);

        for (auto& hotkey : s_Hotkeys) {
            auto it = std::find(hotkey.virtualKeys.begin(), hotkey.virtualKeys.end(), vk);
            if (it != hotkey.virtualKeys.end()) {
                bool allOthersPressed = true;
                for (short key : hotkey.virtualKeys) {
                    if (key != vk) {
                        if (!(GetAsyncKeyState(key) & 0x8000)) {
                            allOthersPressed = false;
                            break;
                        }
                    }
                }

                if (allOthersPressed) {
                    if (isDown && !hotkey.isPressed) {
                        hotkey.isPressed = true;
                        if (hotkey.keyDownCallbackIdx >= 0) CallJS(hotkey.keyDownCallbackIdx);
                    } else if (isUp && hotkey.isPressed) {
                        hotkey.isPressed = false;
                        if (hotkey.keyUpCallbackIdx >= 0) CallJS(hotkey.keyUpCallbackIdx);
                    }
                }
            }
        }
    }
    return CallNextHookEx(s_KeyboardHook, nCode, wParam, lParam);
}

void HotkeyManager::CallJS(int callbackIdx) {
    if (s_JSHandler) {
        s_JSHandler(callbackIdx);
    }
}
