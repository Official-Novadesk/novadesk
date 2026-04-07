/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <NovadeskAPI/novadesk_addon.h>

#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cwctype>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

const NovadeskHostAPI* g_Host = nullptr;
static HWND g_MessageWindow = nullptr;

namespace
{
    struct JsFunctionHandleMirror
    {
        void *ctx = nullptr;
        void *fn = nullptr;
    };

    struct HotkeyEntry
    {
        UINT modifiers = 0;
        UINT vk = 0;
        bool pressed = false;
        void* onKeyDown = nullptr;
        void* onKeyUp = nullptr;
    };

    std::unordered_map<int, HotkeyEntry> g_hotkeys;
    std::mutex g_hotkeyMutex;
    int g_nextHotkeyId = 10000;
    HHOOK g_keyboardHook = nullptr;

    struct DispatchEvent
    {
        int id = -1;
        bool down = false;
    };

    std::wstring TrimUpperCopy(std::wstring s)
    {
        auto isSpace = [](wchar_t c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; };
        size_t start = 0;
        while (start < s.size() && isSpace(s[start]))
            ++start;
        size_t end = s.size();
        while (end > start && isSpace(s[end - 1]))
            --end;
        s = s.substr(start, end - start);
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(towupper(c)); });
        return s;
    }

    std::wstring Utf8ToWide(const char *s)
    {
        if (!s || !*s)
            return L"";
        const int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (len <= 1)
            return L"";
        std::wstring out(static_cast<size_t>(len - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
        return out;
    }

    bool ParseHotkeyString(const std::wstring &hotkey, UINT &modifiers, UINT &vk)
    {
        modifiers = 0;
        vk = 0;

        size_t start = 0;
        while (start <= hotkey.size())
        {
            size_t plus = hotkey.find(L'+', start);
            std::wstring token = (plus == std::wstring::npos) ? hotkey.substr(start) : hotkey.substr(start, plus - start);
            token = TrimUpperCopy(token);

            if (token == L"CTRL" || token == L"CONTROL")
                modifiers |= MOD_CONTROL;
            else if (token == L"ALT")
                modifiers |= MOD_ALT;
            else if (token == L"SHIFT")
                modifiers |= MOD_SHIFT;
            else if (token == L"WIN" || token == L"WINDOWS")
                modifiers |= MOD_WIN;
            else if (token.size() == 1 && token[0] >= L'A' && token[0] <= L'Z')
                vk = static_cast<UINT>(token[0]);
            else if (token.size() == 1 && token[0] >= L'0' && token[0] <= L'9')
                vk = static_cast<UINT>(token[0]);
            else if (token.size() >= 2 && token[0] == L'F')
            {
                int f = _wtoi(token.c_str() + 1);
                if (f >= 1 && f <= 24)
                    vk = VK_F1 + (f - 1);
            }
            else if (token == L"SPACE")
                vk = VK_SPACE;
            else if (token == L"ENTER" || token == L"RETURN")
                vk = VK_RETURN;
            else if (token == L"TAB")
                vk = VK_TAB;
            else if (token == L"ESC" || token == L"ESCAPE")
                vk = VK_ESCAPE;
            else if (token == L"BACKSPACE")
                vk = VK_BACK;
            else if (token == L"DELETE" || token == L"DEL")
                vk = VK_DELETE;
            else if (token == L"INSERT" || token == L"INS")
                vk = VK_INSERT;
            else if (token == L"HOME")
                vk = VK_HOME;
            else if (token == L"END")
                vk = VK_END;
            else if (token == L"PAGEUP" || token == L"PGUP")
                vk = VK_PRIOR;
            else if (token == L"PAGEDOWN" || token == L"PGDN")
                vk = VK_NEXT;
            else if (token == L"LEFT")
                vk = VK_LEFT;
            else if (token == L"RIGHT")
                vk = VK_RIGHT;
            else if (token == L"UP")
                vk = VK_UP;
            else if (token == L"DOWN")
                vk = VK_DOWN;

            if (plus == std::wstring::npos)
                break;
            start = plus + 1;
        }
        return vk != 0;
    }

    bool IsModifierVk(UINT vk)
    {
        return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
               vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
               vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
               vk == VK_LWIN || vk == VK_RWIN;
    }

    bool IsDownWithEvent(UINT checkVk, UINT eventVk, bool eventDown)
    {
        if (checkVk == VK_CONTROL)
        {
            if (eventVk == VK_LCONTROL || eventVk == VK_RCONTROL || eventVk == VK_CONTROL)
                return eventDown;
            return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        }
        if (checkVk == VK_MENU)
        {
            if (eventVk == VK_LMENU || eventVk == VK_RMENU || eventVk == VK_MENU)
                return eventDown;
            return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        }
        if (checkVk == VK_SHIFT)
        {
            if (eventVk == VK_LSHIFT || eventVk == VK_RSHIFT || eventVk == VK_SHIFT)
                return eventDown;
            return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        }
        if (checkVk == VK_LWIN || checkVk == VK_RWIN)
        {
            if (eventVk == checkVk)
                return eventDown;
            return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
        }
        if (eventVk == checkVk)
            return eventDown;
        return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
    }

    bool IsHotkeyPressedNow(const HotkeyEntry &hk, UINT eventVk, bool eventDown)
    {
        if ((hk.modifiers & MOD_CONTROL) && !IsDownWithEvent(VK_CONTROL, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_ALT) && !IsDownWithEvent(VK_MENU, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_SHIFT) && !IsDownWithEvent(VK_SHIFT, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_WIN) &&
            !IsDownWithEvent(VK_LWIN, eventVk, eventDown) &&
            !IsDownWithEvent(VK_RWIN, eventVk, eventDown))
            return false;
        return IsDownWithEvent(hk.vk, eventVk, eventDown);
    }

    void DispatchHotkey(void *data)
    {
        auto *evt = reinterpret_cast<DispatchEvent *>(data);
        if (!evt)
            return;

        HotkeyEntry entry{};
        {
            std::lock_guard<std::mutex> lock(g_hotkeyMutex);
            auto it = g_hotkeys.find(evt->id);
            if (it != g_hotkeys.end())
            {
                entry = it->second;
            }
        }

        void *fn = evt->down ? entry.onKeyDown : entry.onKeyUp;
        auto *handle = reinterpret_cast<JsFunctionHandleMirror *>(fn);
        if (handle && handle->ctx)
        {
            g_Host->JsCallFunctionNoArgs(nullptr, fn);
        }
        delete evt;
    }

    void PostHotkeyEvent(int id, bool down)
    {
        if (!g_MessageWindow)
            return;
        auto *evt = new DispatchEvent{};
        evt->id = id;
        evt->down = down;
        PostMessageW(g_MessageWindow, WM_USER + 101, reinterpret_cast<WPARAM>(&DispatchHotkey), reinterpret_cast<LPARAM>(evt));
    }

    LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION && lParam)
        {
            const auto *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
            const UINT vk = kb->vkCode;
            const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
            if (isDown || isUp)
            {
                const bool relevant = IsModifierVk(vk);
                std::lock_guard<std::mutex> lock(g_hotkeyMutex);
                for (auto &kv : g_hotkeys)
                {
                    HotkeyEntry &hk = kv.second;
                    if (!relevant && vk != hk.vk)
                    {
                        continue;
                    }

                    const bool nowPressed = IsHotkeyPressedNow(hk, vk, isDown);
                    if (nowPressed && !hk.pressed)
                    {
                        hk.pressed = true;
                        PostHotkeyEvent(kv.first, true);
                    }
                    else if (!nowPressed && hk.pressed)
                    {
                        hk.pressed = false;
                        PostHotkeyEvent(kv.first, false);
                    }
                }
            }
        }
        return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
    }

    void EnsureKeyboardHook()
    {
        if (!g_keyboardHook)
        {
            g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandleW(nullptr), 0);
        }
    }

    void MaybeRemoveKeyboardHook()
    {
        if (g_hotkeys.empty() && g_keyboardHook)
        {
            UnhookWindowsHookEx(g_keyboardHook);
            g_keyboardHook = nullptr;
        }
    }

    int RegisterHotkey(const std::wstring &hotkey, void *keyDownFn, void *keyUpFn)
    {
        UINT modifiers = 0;
        UINT vk = 0;
        if (!ParseHotkeyString(hotkey, modifiers, vk))
            return -1;

        EnsureKeyboardHook();
        if (!g_keyboardHook)
            return -1;

        std::lock_guard<std::mutex> lock(g_hotkeyMutex);
        int id = g_nextHotkeyId++;
        HotkeyEntry entry{};
        entry.modifiers = modifiers;
        entry.vk = vk;
        entry.onKeyDown = keyDownFn;
        entry.onKeyUp = keyUpFn;
        g_hotkeys[id] = entry;
        return id;
    }

    bool UnregisterHotkey(int id)
    {
        std::lock_guard<std::mutex> lock(g_hotkeyMutex);
        auto it = g_hotkeys.find(id);
        if (it == g_hotkeys.end())
            return false;
        g_hotkeys.erase(it);
        MaybeRemoveKeyboardHook();
        return true;
    }

    void ClearHotkeys()
    {
        std::lock_guard<std::mutex> lock(g_hotkeyMutex);
        g_hotkeys.clear();
        MaybeRemoveKeyboardHook();
    }

    bool ReadHandler(novadesk_context ctx, int argIndex, void *&outDown, void *&outUp)
    {
        outDown = nullptr;
        outUp = nullptr;

        if (g_Host->IsFunction(ctx, argIndex))
        {
            outDown = g_Host->JsGetFunctionPtr(ctx, argIndex);
            return outDown != nullptr;
        }

        if (!g_Host->IsObject(ctx, argIndex))
            return false;

        const int before = g_Host->GetTop(ctx);
        g_Host->GetProperty(ctx, argIndex, "onKeyDown");
        if (g_Host->GetTop(ctx) > before && g_Host->IsFunction(ctx, -1))
        {
            outDown = g_Host->JsGetFunctionPtr(ctx, -1);
        }
        if (g_Host->GetTop(ctx) > before)
            g_Host->Pop(ctx);

        g_Host->GetProperty(ctx, argIndex, "onKeyUp");
        if (g_Host->GetTop(ctx) > before && g_Host->IsFunction(ctx, -1))
        {
            outUp = g_Host->JsGetFunctionPtr(ctx, -1);
        }
        if (g_Host->GetTop(ctx) > before)
            g_Host->Pop(ctx);

        return outDown != nullptr || outUp != nullptr;
    }

    int JsHotkeyRegister(novadesk_context ctx)
    {
        const int top = g_Host->GetTop(ctx);
        if (top < 2 || !g_Host->IsString(ctx, 0))
        {
            g_Host->ThrowError(ctx, "hotkey.register(hotkey, handler) requires hotkey string and handler");
            return 0;
        }

        const char *hotkeyC = g_Host->GetString(ctx, 0);
        if (!hotkeyC)
        {
            g_Host->ThrowError(ctx, "hotkey.register: invalid hotkey string");
            return 0;
        }

        void *downFn = nullptr;
        void *upFn = nullptr;
        if (!ReadHandler(ctx, 1, downFn, upFn))
        {
            g_Host->ThrowError(ctx, "hotkey.register handler must be function or object");
            return 0;
        }

        int id = RegisterHotkey(Utf8ToWide(hotkeyC), downFn, upFn);
        if (id < 0)
        {
            g_Host->ThrowError(ctx, "hotkey.register failed to register hotkey");
            return 0;
        }
        g_Host->PushNumber(ctx, static_cast<double>(id));
        return 1;
    }

    int JsHotkeyUnregister(novadesk_context ctx)
    {
        const int top = g_Host->GetTop(ctx);
        if (top < 1 || !g_Host->IsNumber(ctx, 0))
        {
            g_Host->ThrowError(ctx, "hotkey.unregister(id) expects number");
            return 0;
        }
        int id = static_cast<int>(g_Host->GetNumber(ctx, 0));
        const bool ok = UnregisterHotkey(id);
        g_Host->PushBool(ctx, ok ? 1 : 0);
        return 1;
    }
} // namespace

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host)
{
    g_Host = host;
    g_MessageWindow = hMsgWnd;
    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "Hotkey");
    addon.RegisterString("version", "1.0.0");
    addon.RegisterFunction("register", JsHotkeyRegister, 2);
    addon.RegisterFunction("unregister", JsHotkeyUnregister, 1);
}

NOVADESK_ADDON_UNLOAD()
{
    ClearHotkeys();
}
