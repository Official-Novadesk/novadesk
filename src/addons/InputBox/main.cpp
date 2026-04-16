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
#include <CommCtrl.h>
#include "../../apps/novadesk/shared/ColorUtil.h"
#include <algorithm>
#include <cwctype>
#include <string>
#include <unordered_map>
#include <vector>

const NovadeskHostAPI *g_Host = nullptr;

namespace
{
    enum class InputType
    {
        Any,
        Integer,
        Float,
        Letters,
        Alnum,
        Hex,
        Email,
        Custom
    };
    struct Opt
    {
        int x = 100;
        int y = 100;
        int w = 300;
        int h = 40;

        int maxLen = 0;
        int border = 1;
        int fontSize = 14;
        int align = ES_LEFT;

        intptr_t widgetHwnd = 0;

        bool topMost = true;
        bool dismissOnBlur = true;
        bool multiline = false;
        bool password = false;
        bool scroll = false;
        bool bold = false;
        bool italic = false;
        bool borderVisible = true;

        COLORREF bg = RGB(32, 32, 32);
        COLORREF fg = RGB(240, 240, 240);
        COLORREF bd = RGB(90, 90, 90);
        COLORREF msgColor = RGB(200, 200, 200);

        std::wstring msg;
        std::wstring def;
        std::wstring placeholder;
        std::wstring font = L"Segoe UI";
        std::wstring allowed;

        InputType type = InputType::Any;

        bool hasRange = false;
        double minV = 0;
        double maxV = 0;
    };
    struct Cb
    {
        void *onEnter = nullptr;
        void *onEsc = nullptr;
        void *onDismiss = nullptr;
        void *onInvalid = nullptr;
        void *onChange = nullptr;
    };
    struct S
    {
        int id = 0;
        Opt o;
        Cb cb;
        HWND w = nullptr, e = nullptr, l = nullptr;
        HFONT f = nullptr;
        HBRUSH b = nullptr;
        WNDPROC oldEdit = nullptr;
        bool closing = false;
    };
    static std::unordered_map<int, S *> g_map;
    static int g_nextId = 1;
    static std::wstring g_lastTextW;
    static std::string g_lastTextU8;
    static int g_lastId = 0;
    static int g_lastReason = 0;

    std::wstring W(const char *s)
    {
        if (!s || !*s)
            return L"";
        int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        if (n <= 1)
            return L"";
        std::wstring o((size_t)n - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s, -1, o.data(), n);
        return o;
    }
    std::string U(const std::wstring &w)
    {
        if (w.empty())
            return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        if (n <= 0)
            return {};
        std::string o((size_t)n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), o.data(), n, nullptr, nullptr);
        return o;
    }
    int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    std::wstring trim(std::wstring s)
    {
        auto sp = [](wchar_t c)
        { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; };
        size_t a = 0, b = s.size();
        while (a < b && sp(s[a]))
            ++a;
        while (b > a && sp(s[b - 1]))
            --b;
        return s.substr(a, b - a);
    }
    std::wstring up(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c)
                       { return (wchar_t)towupper(c); });
        return s;
    }

    bool parseColor(const std::wstring &in, COLORREF &out)
    {
        BYTE alpha = 255;
        return ColorUtil::ParseRGBA(trim(in), out, alpha);
    }
    InputType parseType(const std::wstring &in)
    {
        auto s = up(trim(in));
        if (s == L"INTEGER" || s == L"INT")
            return InputType::Integer;
        if (s == L"FLOAT" || s == L"NUMBER")
            return InputType::Float;
        if (s == L"LETTERS")
            return InputType::Letters;
        if (s == L"ALPHANUMERIC")
            return InputType::Alnum;
        if (s == L"HEX" || s == L"HEXADECIMAL")
            return InputType::Hex;
        if (s == L"EMAIL")
            return InputType::Email;
        if (s == L"CUSTOM")
            return InputType::Custom;
        return InputType::Any;
    }

    bool getStr(novadesk_context c, int i, const char *n, std::wstring &v, bool &f)
    {
        f = false;
        int b = g_Host->GetTop(c);
        g_Host->GetProperty(c, i, n);
        if (g_Host->GetTop(c) <= b)
            return true;
        if (g_Host->IsNull(c, -1))
        {
            g_Host->Pop(c);
            return true;
        }
        if (!g_Host->IsString(c, -1))
        {
            g_Host->Pop(c);
            return false;
        }
        v = W(g_Host->GetString(c, -1));
        f = true;
        g_Host->Pop(c);
        return true;
    }
    bool getBool(novadesk_context c, int i, const char *n, bool &v, bool &f)
    {
        f = false;
        int b = g_Host->GetTop(c);
        g_Host->GetProperty(c, i, n);
        if (g_Host->GetTop(c) <= b)
            return true;
        if (g_Host->IsNull(c, -1))
        {
            g_Host->Pop(c);
            return true;
        }
        if (!g_Host->IsBool(c, -1))
        {
            g_Host->Pop(c);
            return false;
        }
        v = g_Host->GetBool(c, -1) != 0;
        f = true;
        g_Host->Pop(c);
        return true;
    }
    bool getInt(novadesk_context c, int i, const char *n, int &v, bool &f)
    {
        f = false;
        int b = g_Host->GetTop(c);
        g_Host->GetProperty(c, i, n);
        if (g_Host->GetTop(c) <= b)
            return true;
        if (g_Host->IsNull(c, -1))
        {
            g_Host->Pop(c);
            return true;
        }
        if (!g_Host->IsNumber(c, -1))
        {
            g_Host->Pop(c);
            return false;
        }
        v = (int)g_Host->GetNumber(c, -1);
        f = true;
        g_Host->Pop(c);
        return true;
    }
    bool getNum(novadesk_context c, int i, const char *n, double &v, bool &f)
    {
        f = false;
        int b = g_Host->GetTop(c);
        g_Host->GetProperty(c, i, n);
        if (g_Host->GetTop(c) <= b)
            return true;
        if (g_Host->IsNull(c, -1))
        {
            g_Host->Pop(c);
            return true;
        }
        if (!g_Host->IsNumber(c, -1))
        {
            g_Host->Pop(c);
            return false;
        }
        v = g_Host->GetNumber(c, -1);
        f = true;
        g_Host->Pop(c);
        return true;
    }
    bool getFn(novadesk_context c, int i, const char *n, void *&fn)
    {
        int b = g_Host->GetTop(c);
        g_Host->GetProperty(c, i, n);
        if (g_Host->GetTop(c) <= b)
            return true;
        if (g_Host->IsNull(c, -1))
        {
            g_Host->Pop(c);
            return true;
        }
        if (!g_Host->IsFunction(c, -1))
        {
            g_Host->Pop(c);
            return false;
        }
        fn = g_Host->JsGetFunctionPtr(c, -1);
        g_Host->Pop(c);
        return true;
    }

    enum class EventReason
    {
        None = 0,
        Enter = 1,
        Esc = 2,
        Dismiss = 3,
        Invalid = 4,
        Change = 5
    };

    void setLastEvent(S *s, EventReason reason, const std::wstring &text)
    {
        g_lastId = s ? s->id : 0;
        g_lastReason = static_cast<int>(reason);
        g_lastTextW = text;
        g_lastTextU8 = U(g_lastTextW);
    }

    void call0(void *fn)
    {
        if (!fn)
            return;
        g_Host->JsCallFunctionNoArgs(nullptr, fn);
    }
    std::wstring getText(HWND e)
    {
        int n = GetWindowTextLengthW(e);
        std::wstring t((size_t)std::max(0, n), L'\0');
        if (n > 0)
            GetWindowTextW(e, t.data(), n + 1);
        return t;
    }

    bool validChar(const S *s, wchar_t ch)
    {
        if (!s || ch < 32)
            return true;
        switch (s->o.type)
        {
        case InputType::Any:
            return true;
        case InputType::Letters:
            return iswalpha(ch) != 0;
        case InputType::Alnum:
            return iswalnum(ch) != 0;
        case InputType::Hex:
        {
            wchar_t u = (wchar_t)towupper(ch);
            return (u >= L'0' && u <= L'9') || (u >= L'A' && u <= L'F');
        }
        case InputType::Email:
            return iswalnum(ch) || ch == L'@' || ch == L'.' || ch == L'-' || ch == L'_' || ch == L'+';
        case InputType::Custom:
            return s->o.allowed.find(ch) != std::wstring::npos;
        case InputType::Integer:
            return (ch >= L'0' && ch <= L'9') || ch == L'-';
        case InputType::Float:
            return (ch >= L'0' && ch <= L'9') || ch == L'-' || ch == L'.';
        }
        return true;
    }
    bool validFinal(S *s, const std::wstring &t)
    {
        if (!s)
            return true;
        if (s->o.type == InputType::Integer)
        {
            wchar_t *e = nullptr;
            long v = wcstol(t.c_str(), &e, 10);
            if (!e || *e != 0)
                return false;
            if (s->o.hasRange)
                return (double)v >= s->o.minV && (double)v <= s->o.maxV;
            return true;
        }
        if (s->o.type == InputType::Float)
        {
            wchar_t *e = nullptr;
            double v = wcstod(t.c_str(), &e);
            if (!e || *e != 0)
                return false;
            if (s->o.hasRange)
                return v >= s->o.minV && v <= s->o.maxV;
            return true;
        }
        return true;
    }

    void removeMap(S *s)
    {
        if (!s)
            return;
        auto it = g_map.find(s->id);
        if (it != g_map.end() && it->second == s)
            g_map.erase(it);
    }
    void destroyS(S *s)
    {
        if (!s || s->closing)
            return;
        s->closing = true;
        if (s->w && IsWindow(s->w))
            DestroyWindow(s->w);
    }
    void closeReason(S *s, int reason)
    {
        if (!s || s->closing)
            return;
        auto txt = getText(s->e);
        if (reason == 1)
        {
            if (validFinal(s, txt))
            {
                setLastEvent(s, EventReason::Enter, txt);
                call0(s->cb.onEnter);
            }
            else
            {
                setLastEvent(s, EventReason::Invalid, txt);
                call0(s->cb.onInvalid);
                return;
            }
        }
        else if (reason == 2)
        {
            setLastEvent(s, EventReason::Esc, txt);
            call0(s->cb.onEsc);
        }
        else
        {
            setLastEvent(s, EventReason::Dismiss, txt);
            call0(s->cb.onDismiss);
        }
        destroyS(s);
    }

    LRESULT CALLBACK EditProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        auto *s = (S *)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (!s || !s->oldEdit)
            return DefWindowProcW(h, m, w, l);
        if (m == WM_CHAR)
        {
            wchar_t c = (wchar_t)w;
            if (!validChar(s, c))
            {
                setLastEvent(s, EventReason::Invalid, getText(h));
                call0(s->cb.onInvalid);
                return 0;
            }
        }
        else if (m == WM_KEYDOWN)
        {
            if (w == VK_ESCAPE)
            {
                closeReason(s, 2);
                return 0;
            }
            if (w == VK_RETURN && (!s->o.multiline || (GetKeyState(VK_CONTROL) & 0x8000)))
            {
                closeReason(s, 1);
                return 0;
            }
        }
        return CallWindowProcW(s->oldEdit, h, m, w, l);
    }

    LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        auto *s = (S *)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (m == WM_NCCREATE)
        {
            auto *cs = (CREATESTRUCTW *)l;
            auto *x = (S *)(cs ? cs->lpCreateParams : nullptr);
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)x);
            return TRUE;
        }
        switch (m)
        {
        case WM_CREATE:
        {
            if (!s)
                return -1;
            int b = s->o.borderVisible ? clampi(s->o.border, 0, 12) : 0;
            int mg = b, lh = s->o.msg.empty() ? 0 : 18, tg = 0, top = mg + lh + tg, eh = s->o.h - top - mg;
            if (eh < 24)
                eh = 24;
            HDC dc = GetDC(h);
            int fh = -MulDiv(s->o.fontSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
            ReleaseDC(h, dc);
            s->f = CreateFontW(fh, 0, 0, 0, s->o.bold ? FW_BOLD : FW_NORMAL, s->o.italic, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, s->o.font.c_str());
            if (lh)
            {
                s->l = CreateWindowExW(0, L"STATIC", s->o.msg.c_str(), WS_CHILD | WS_VISIBLE, mg, mg, s->o.w - (mg * 2), lh, h, nullptr, GetModuleHandleW(nullptr), nullptr);
                if (s->l && s->f)
                    SendMessageW(s->l, WM_SETFONT, (WPARAM)s->f, TRUE);
            }
            DWORD es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | s->o.align;
            if (s->o.multiline)
            {
                es |= ES_MULTILINE | ES_AUTOVSCROLL;
                if (s->o.scroll)
                    es |= WS_VSCROLL;
            }
            else
                es |= ES_AUTOHSCROLL;
            if (s->o.password)
                es |= ES_PASSWORD;
            DWORD editEx = s->o.borderVisible ? WS_EX_CLIENTEDGE : 0;
            s->e = CreateWindowExW(editEx, L"EDIT", s->o.def.c_str(), es, mg, top, s->o.w - (mg * 2), eh, h, (HMENU)1001, GetModuleHandleW(nullptr), nullptr);
            if (s->e && s->o.maxLen > 0)
                SendMessageW(s->e, EM_LIMITTEXT, (WPARAM)s->o.maxLen, 0);
            if (s->e && !s->o.placeholder.empty())
                SendMessageW(s->e, EM_SETCUEBANNER, TRUE, (LPARAM)s->o.placeholder.c_str());
            if (s->e && s->f)
                SendMessageW(s->e, WM_SETFONT, (WPARAM)s->f, TRUE);
            if (s->e)
            {
                SetWindowLongPtrW(s->e, GWLP_USERDATA, (LONG_PTR)s);
                s->oldEdit = (WNDPROC)SetWindowLongPtrW(s->e, GWLP_WNDPROC, (LONG_PTR)EditProc);
                SetFocus(s->e);
                SendMessageW(s->e, EM_SETSEL, 0, -1);
            }
            s->b = CreateSolidBrush(s->o.bg);
            return 0;
        }
        case WM_COMMAND:
            if (HIWORD(w) == EN_UPDATE && s && s->e && (HWND)l == s->e)
            {
                setLastEvent(s, EventReason::Change, getText(s->e));
                call0(s->cb.onChange);
            }
            break;
        case WM_ACTIVATE:
            if (s && s->o.dismissOnBlur && LOWORD(w) == WA_INACTIVE)
            {
                closeReason(s, 3);
                return 0;
            }
            break;
        case WM_CTLCOLOREDIT:
            if (s)
            {
                HDC dc = (HDC)w;
                SetTextColor(dc, s->o.fg);
                SetBkColor(dc, s->o.bg);
                return (LRESULT)(s->b ? s->b : GetStockObject(WHITE_BRUSH));
            }
            break;
        case WM_CTLCOLORSTATIC:
            if (s && (HWND)l == s->l)
            {
                HDC dc = (HDC)w;
                SetTextColor(dc, s->o.msgColor);
                SetBkColor(dc, s->o.bg);
                return (LRESULT)(s->b ? s->b : GetStockObject(WHITE_BRUSH));
            }
            break;
        case WM_PAINT:
            if (s)
            {
                PAINTSTRUCT ps{};
                HDC dc = BeginPaint(h, &ps);
                RECT rc{};
                GetClientRect(h, &rc);
                HBRUSH bg = CreateSolidBrush(s->o.bg);
                FillRect(dc, &rc, bg);
                DeleteObject(bg);
                int bt = s->o.borderVisible ? clampi(s->o.border, 0, 12) : 0;
                if (bt > 0)
                {
                    HPEN p = CreatePen(PS_SOLID, bt, s->o.bd);
                    HGDIOBJ op = SelectObject(dc, p), ob = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
                    Rectangle(dc, 0, 0, rc.right, rc.bottom);
                    SelectObject(dc, ob);
                    SelectObject(dc, op);
                    DeleteObject(p);
                }
                EndPaint(h, &ps);
                return 0;
            }
            break;
        case WM_CLOSE:
            if (s)
            {
                closeReason(s, 3);
                return 0;
            }
            break;
        case WM_NCDESTROY:
            if (s)
            {
                if (s->e && s->oldEdit)
                    SetWindowLongPtrW(s->e, GWLP_WNDPROC, (LONG_PTR)s->oldEdit);
                if (s->b)
                    DeleteObject(s->b);
                if (s->f)
                    DeleteObject(s->f);
                removeMap(s);
                delete s;
                SetWindowLongPtrW(h, GWLP_USERDATA, 0);
            }
            return 0;
        }
        return DefWindowProcW(h, m, w, l);
    }

    bool regClass()
    {
        static bool ok = false;
        if (ok)
            return true;
        WNDCLASSW wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"NovadeskInputBoxOverlay";
        wc.hCursor = LoadCursorW(nullptr, IDC_IBEAM);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        ok = true;
        return true;
    }

    bool parse(novadesk_context c, Opt &o, Cb &cb)
    {
        int t = g_Host->GetTop(c);
        if (t < 1 || g_Host->IsNull(c, 0))
            return true;
        if (g_Host->IsString(c, 0))
        {
            o.def = W(g_Host->GetString(c, 0));
            return true;
        }
        if (!g_Host->IsObject(c, 0))
        {
            g_Host->ThrowError(c, "InputBox.show(options) expects object|string");
            return false;
        }
        bool f = false, bv = false;
        int iv = 0;
        double dv = 0;
        std::wstring s;
        if (!getInt(c, 0, "x", iv, f))
            return false;
        if (f)
            o.x = iv;
        if (!getInt(c, 0, "y", iv, f))
            return false;
        if (f)
            o.y = iv;
        if (!getInt(c, 0, "width", iv, f))
            return false;
        if (f)
            o.w = clampi(iv, 120, 1200);
        if (!getInt(c, 0, "w", iv, f))
            return false;
        if (f)
            o.w = clampi(iv, 120, 1200);
        if (!getInt(c, 0, "height", iv, f))
            return false;
        if (f)
            o.h = clampi(iv, 28, 800);
        if (!getInt(c, 0, "h", iv, f))
            return false;
        if (f)
            o.h = clampi(iv, 28, 800);
        if (!getNum(c, 0, "widgetHwnd", dv, f))
            return false;
        if (f)
            o.widgetHwnd = (intptr_t)dv;
        if (!getNum(c, 0, "hwnd", dv, f))
            return false;
        if (f)
            o.widgetHwnd = (intptr_t)dv;
        if (!getBool(c, 0, "topMost", bv, f))
            return false;
        if (f)
            o.topMost = bv;
        if (!getBool(c, 0, "unfocusDismiss", bv, f))
            return false;
        if (f)
            o.dismissOnBlur = bv;
        if (!getBool(c, 0, "multiline", bv, f))
            return false;
        if (f)
            o.multiline = bv;
        if (!getBool(c, 0, "password", bv, f))
            return false;
        if (f)
            o.password = bv;
        if (!getBool(c, 0, "allowScroll", bv, f))
            return false;
        if (f)
            o.scroll = bv;
        if (!getInt(c, 0, "maxLength", iv, f))
            return false;
        if (f)
            o.maxLen = clampi(iv, 0, 32766);
        if (!getInt(c, 0, "fontSize", iv, f))
            return false;
        if (f)
            o.fontSize = clampi(iv, 8, 72);
        if (!getInt(c, 0, "borderThickness", iv, f))
            return false;
        if (f)
            o.border = clampi(iv, 0, 12);
        if (!getBool(c, 0, "borderVisible", bv, f))
            return false;
        if (f)
            o.borderVisible = bv;
        if (!getBool(c, 0, "bold", bv, f))
            return false;
        if (f)
            o.bold = bv;
        if (!getBool(c, 0, "italic", bv, f))
            return false;
        if (f)
            o.italic = bv;
        if (!getStr(c, 0, "fontFace", s, f))
            return false;
        if (f && !s.empty())
            o.font = s;
        if (!getStr(c, 0, "message", s, f))
            return false;
        if (f)
            o.msg = s;
        if (!getStr(c, 0, "defaultValue", s, f))
            return false;
        if (f)
            o.def = s;
        if (!getStr(c, 0, "placeholder", s, f))
            return false;
        if (f)
            o.placeholder = s;
        if (!getStr(c, 0, "allowedChars", s, f))
            return false;
        if (f)
            o.allowed = s;
        if (!getStr(c, 0, "inputType", s, f))
            return false;
        if (f)
            o.type = parseType(s);
        if (!getNum(c, 0, "minValue", dv, f))
            return false;
        if (f)
        {
            o.minV = dv;
            o.hasRange = true;
        }
        if (!getNum(c, 0, "maxValue", dv, f))
            return false;
        if (f)
        {
            o.maxV = dv;
            o.hasRange = true;
        }
        if (!getStr(c, 0, "fontColor", s, f))
            return false;
        if (f)
            parseColor(s, o.fg);
        if (!getStr(c, 0, "textColor", s, f))
            return false;
        if (f)
            parseColor(s, o.fg);
        if (!getStr(c, 0, "backgroundColor", s, f))
            return false;
        if (f)
            parseColor(s, o.bg);
        if (!getStr(c, 0, "bgColor", s, f))
            return false;
        if (f)
            parseColor(s, o.bg);
        if (!getStr(c, 0, "borderColor", s, f))
            return false;
        if (f)
            parseColor(s, o.bd);
        if (!getStr(c, 0, "messageColor", s, f))
            return false;
        if (f)
            parseColor(s, o.msgColor);
        if (!getStr(c, 0, "align", s, f))
            return false;
        if (f)
        {
            auto a = up(trim(s));
            o.align = (a == L"CENTER") ? ES_CENTER : ((a == L"RIGHT") ? ES_RIGHT : ES_LEFT);
        }
        if (!getFn(c, 0, "onEnter", cb.onEnter))
            return false;
        if (!getFn(c, 0, "onEsc", cb.onEsc))
            return false;
        if (!getFn(c, 0, "onDismiss", cb.onDismiss))
            return false;
        if (!getFn(c, 0, "onInvalid", cb.onInvalid))
            return false;
        if (!getFn(c, 0, "onChange", cb.onChange))
            return false;
        if (o.password && o.multiline)
            o.multiline = false;
        return true;
    }

    int create(const Opt &o, const Cb &cb)
    {
        if (!regClass())
            return 0;
        S *s = new S();
        s->id = g_nextId++;
        s->o = o;
        s->cb = cb;
        RECT wa{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        int x = o.x, y = o.y;
        HWND anchor = (HWND)o.widgetHwnd;
        if (anchor && IsWindow(anchor))
        {
            RECT wr{};
            if (GetWindowRect(anchor, &wr))
            {
                x = wr.left + o.x;
                y = wr.top + o.y;
            }
        }
        if (x < wa.left)
            x = wa.left;
        if (y < wa.top)
            y = wa.top;
        s->w = CreateWindowExW(WS_EX_TOOLWINDOW | (o.topMost ? WS_EX_TOPMOST : 0), L"NovadeskInputBoxOverlay", L"", WS_POPUP | WS_VISIBLE, x, y, o.w, o.h, nullptr, nullptr, GetModuleHandleW(nullptr), s);
        if (!s->w)
        {
            delete s;
            return 0;
        }
        g_map[s->id] = s;
        ShowWindow(s->w, SW_SHOW);
        UpdateWindow(s->w);
        if (s->e)
            SetFocus(s->e);
        return s->id;
    }

    int JsShow(novadesk_context c)
    {
        Opt o;
        Cb cb;
        if (!parse(c, o, cb))
            return 0;
        int id = create(o, cb);
        g_Host->PushNumber(c, (double)id);
        return 1;
    }
    int JsClose(novadesk_context c)
    {
        if (g_Host->GetTop(c) < 1 || !g_Host->IsNumber(c, 0))
        {
            g_Host->ThrowError(c, "InputBox.close(id) expects numeric id");
            return 0;
        }
        int id = (int)g_Host->GetNumber(c, 0);
        auto it = g_map.find(id);
        if (it == g_map.end())
        {
            g_Host->PushBool(c, 0);
            return 1;
        }
        destroyS(it->second);
        g_Host->PushBool(c, 1);
        return 1;
    }
    int JsCloseAll(novadesk_context c)
    {
        std::vector<int> ids;
        ids.reserve(g_map.size());
        for (auto &kv : g_map)
            ids.push_back(kv.first);
        for (int id : ids)
        {
            auto it = g_map.find(id);
            if (it != g_map.end())
                destroyS(it->second);
        }
        g_Host->PushBool(c, 1);
        return 1;
    }
    int JsLastText(novadesk_context c)
    {
        g_Host->PushString(c, g_lastTextU8.c_str());
        return 1;
    }
    int JsLastReason(novadesk_context c)
    {
        g_Host->PushNumber(c, (double)g_lastReason);
        return 1;
    }
    int JsLastId(novadesk_context c)
    {
        g_Host->PushNumber(c, (double)g_lastId);
        return 1;
    }
    void CloseAll()
    {
        std::vector<int> ids;
        ids.reserve(g_map.size());
        for (auto &kv : g_map)
            ids.push_back(kv.first);
        for (int id : ids)
        {
            auto it = g_map.find(id);
            if (it != g_map.end())
                destroyS(it->second);
        }
    }
}

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host)
{
    InitCommonControls();
    g_Host = host;
    (void)hMsgWnd;
    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "InputBox");
    addon.RegisterString("version", "2.1.0");
    addon.RegisterFunction("show", JsShow, 1);
    addon.RegisterFunction("open", JsShow, 1);
    addon.RegisterFunction("close", JsClose, 1);
    addon.RegisterFunction("closeAll", JsCloseAll, 0);
    addon.RegisterFunction("lastText", JsLastText, 0);
    addon.RegisterFunction("lastReason", JsLastReason, 0);
    addon.RegisterFunction("lastId", JsLastId, 0);
}
NOVADESK_ADDON_UNLOAD() { CloseAll(); }
