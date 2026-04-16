#include <NovadeskAPI/novadesk_addon.h>

#include <Windows.h>
#include <VersionHelpers.h>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <cstring>

const NovadeskHostAPI* g_Host = nullptr;
HMODULE g_User32 = nullptr;
HMODULE g_DwmApi = nullptr;

static const int WCA_ACCENT_POLICY = 19;
static const DWORD DWMWA_WINDOW_CORNER_PREFERENCE_VALUE = 33;

enum class AccentState {
    DISABLED = 0,
    BLURBEHIND = 3,
    ACRYLIC = 4
};

enum DwmWindowCornerPreference {
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
};

struct ACCENTPOLICY {
    int nAccentState;
    int nFlags;
    unsigned int nColor;
    int nAnimationId;
};

struct WINCOMPATTRDATA {
    int nAttribute;
    PVOID pData;
    ULONG ulDataSize;
};

typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
typedef HRESULT(WINAPI* pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

pSetWindowCompositionAttribute g_SetWindowCompositionAttribute = nullptr;
pDwmSetWindowAttribute g_DwmSetWindowAttribute = nullptr;

static bool LoadApis() {
    if (!g_SetWindowCompositionAttribute) {
        g_User32 = LoadLibraryW(L"user32.dll");
        if (g_User32) {
            g_SetWindowCompositionAttribute =
                (pSetWindowCompositionAttribute)GetProcAddress(g_User32, "SetWindowCompositionAttribute");
        }
    }

    if (!g_DwmSetWindowAttribute) {
        g_DwmApi = LoadLibraryW(L"dwmapi.dll");
        if (g_DwmApi) {
            g_DwmSetWindowAttribute =
                (pDwmSetWindowAttribute)GetProcAddress(g_DwmApi, "DwmSetWindowAttribute");
        }
    }

    return g_SetWindowCompositionAttribute != nullptr;
}

static void UnloadApis() {
    g_SetWindowCompositionAttribute = nullptr;
    g_DwmSetWindowAttribute = nullptr;

    if (g_User32) {
        FreeLibrary(g_User32);
        g_User32 = nullptr;
    }
    if (g_DwmApi) {
        FreeLibrary(g_DwmApi);
        g_DwmApi = nullptr;
    }
}

static bool IsWindowsBuildOrGreater(WORD major, WORD minor, DWORD build) {
    OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, build };
    DWORDLONG mask = 0;
    mask = VerSetConditionMask(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask) != FALSE;
}

static bool SetAccent(HWND hwnd, int borderFlags, AccentState state) {
    if (!hwnd || !IsWindow(hwnd) || !LoadApis()) return false;

    ACCENTPOLICY policy = {};
    policy.nAccentState = static_cast<int>(state);
    policy.nFlags = borderFlags;
    policy.nColor = 0x01000000;
    policy.nAnimationId = 1;

    WINCOMPATTRDATA data = {};
    data.nAttribute = WCA_ACCENT_POLICY;
    data.pData = &policy;
    data.ulDataSize = sizeof(policy);

    return g_SetWindowCompositionAttribute(hwnd, &data) != FALSE;
}

static bool SetWindowCorner(HWND hwnd, DwmWindowCornerPreference corner) {
    if (!hwnd || !g_DwmSetWindowAttribute) return false;
    return SUCCEEDED(g_DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE_VALUE, &corner, sizeof(corner)));
}

static HWND ReadHwndArg(novadesk_context ctx, int idx) {
    if (g_Host->IsString(ctx, idx)) {
        const char* s = g_Host->GetString(ctx, idx);
        if (!s || !*s) return nullptr;
        return reinterpret_cast<HWND>(_strtoui64(s, nullptr, 0));
    }
    if (g_Host->IsNumber(ctx, idx)) {
        double d = g_Host->GetNumber(ctx, idx);
        if (d <= 0) return nullptr;
        return reinterpret_cast<HWND>(static_cast<uintptr_t>(d));
    }
    return nullptr;
}

static bool IsAllDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

static bool IsAllHexDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        const bool dec = (c >= '0' && c <= '9');
        const bool hex = (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!dec && !hex) return false;
    }
    return true;
}

static uintptr_t ParseHandleString(const char* raw) {
    if (!raw) return 0;
    std::string s(raw);
    if (s.empty()) return 0;

    // Trim spaces
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    if (s.empty()) return 0;

    // 0x-prefixed => hex
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 16));
    }

    // Zero-padded handles from host are usually hex without prefix (e.g. 0000000000290386).
    if (IsAllHexDigits(s) && s.size() >= 8) {
        return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 16));
    }

    if (IsAllDigits(s)) {
        return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 10));
    }

    return 0;
}

static HWND FindHwndArg(novadesk_context ctx, int* outIdx) {
    const int top = g_Host->GetTop(ctx);
    const int maxScan = top < 4 ? top : 4;
    for (int i = 0; i < maxScan; ++i) {
        uintptr_t parsed = 0;
        if (g_Host->IsString(ctx, i)) {
            parsed = ParseHandleString(g_Host->GetString(ctx, i));
        } else if (g_Host->IsNumber(ctx, i)) {
            double d = g_Host->GetNumber(ctx, i);
            if (d > 0) parsed = static_cast<uintptr_t>(d);
        }

        if (parsed != 0 && IsWindow(reinterpret_cast<HWND>(parsed))) {
            if (outIdx) *outIdx = i;
            return reinterpret_cast<HWND>(parsed);
        }
    }

    if (outIdx) *outIdx = -1;
    return nullptr;
}

static int ResolveArgBase(novadesk_context ctx) {
    // Some host bindings pass method arguments starting at index 1 (index 0 = this).
    if (g_Host->GetTop(ctx) >= 2 && !g_Host->IsString(ctx, 0) && !g_Host->IsNumber(ctx, 0)) {
        return 1;
    }
    return 0;
}

static AccentState ReadAccentArg(novadesk_context ctx, int idx) {
    if (!g_Host->IsString(ctx, idx)) return AccentState::BLURBEHIND;
    const char* s = g_Host->GetString(ctx, idx);
    if (!s) return AccentState::BLURBEHIND;
    if (_stricmp(s, "none") == 0 || _stricmp(s, "disabled") == 0) return AccentState::DISABLED;
    if (_stricmp(s, "acrylic") == 0) return AccentState::ACRYLIC;
    return AccentState::BLURBEHIND;
}

static DwmWindowCornerPreference ReadCornerArg(novadesk_context ctx, int idx) {
    if (!g_Host->IsString(ctx, idx)) return DWMWCP_DEFAULT;
    const char* s = g_Host->GetString(ctx, idx);
    if (!s) return DWMWCP_DEFAULT;
    if (_stricmp(s, "round") == 0) return DWMWCP_ROUND;
    if (_stricmp(s, "roundsmall") == 0) return DWMWCP_ROUNDSMALL;
    if (_stricmp(s, "none") == 0) return DWMWCP_DONOTROUND;
    return DWMWCP_DEFAULT;
}

static int JsApply(novadesk_context ctx) {
    if (g_Host->GetTop(ctx) > 0 && g_Host->IsObject(ctx, 0)) {
        g_Host->ThrowError(ctx, "apply(hwnd, type?, corner?): object config is not supported by current host API");
        return 0;
    }

    int hwndIdx = -1;
    HWND hwnd = FindHwndArg(ctx, &hwndIdx);
    if (!hwnd) {
        g_Host->ThrowError(ctx, "apply(hwnd, type?, corner?): invalid hwnd");
        return 0;
    }

    int base = (hwndIdx >= 0) ? hwndIdx : ResolveArgBase(ctx);
    AccentState state = ReadAccentArg(ctx, base + 1);
    DwmWindowCornerPreference corner = ReadCornerArg(ctx, base + 2);

    bool ok = SetAccent(hwnd, 0, AccentState::DISABLED) && SetAccent(hwnd, 0, state);
    if (ok && corner != DWMWCP_DEFAULT && corner != DWMWCP_DONOTROUND) {
        SetWindowCorner(hwnd, corner);
    } else if (ok && corner == DWMWCP_DONOTROUND) {
        SetWindowCorner(hwnd, DWMWCP_DONOTROUND);
    }

    g_Host->PushBool(ctx, ok ? 1 : 0);
    return 1;
}

static int JsDisable(novadesk_context ctx) {
    int hwndIdx = -1;
    HWND hwnd = FindHwndArg(ctx, &hwndIdx);
    if (!hwnd) {
        g_Host->ThrowError(ctx, "disable(hwnd): invalid hwnd");
        return 0;
    }
    bool ok = SetAccent(hwnd, 0, AccentState::DISABLED);
    g_Host->PushBool(ctx, ok ? 1 : 0);
    return 1;
}

static int JsSetCorner(novadesk_context ctx) {
    int hwndIdx = -1;
    HWND hwnd = FindHwndArg(ctx, &hwndIdx);
    if (!hwnd) {
        g_Host->ThrowError(ctx, "setCorner(hwnd, corner): invalid hwnd");
        return 0;
    }

    int base = (hwndIdx >= 0) ? hwndIdx : ResolveArgBase(ctx);
    DwmWindowCornerPreference corner = ReadCornerArg(ctx, base + 1);
    bool ok = SetWindowCorner(hwnd, corner);
    g_Host->PushBool(ctx, ok ? 1 : 0);
    return 1;
}

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host) {
    (void)hMsgWnd;
    g_Host = host;
    LoadApis();

    novadesk::Addon addon(ctx, host);
    addon.RegisterString("name", "BlurBehind");
    addon.RegisterString("version", "2.0.0");
    addon.RegisterFunction("apply", JsApply, 3);
    addon.RegisterFunction("disable", JsDisable, 1);
    addon.RegisterFunction("setCorner", JsSetCorner, 2);
}

NOVADESK_ADDON_UNLOAD() {
    UnloadApis();
}
