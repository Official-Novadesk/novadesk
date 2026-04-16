#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shellapi.h>
#include <wininet.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iterator>
#include <cwctype>
#include "../../third_party/json/json.hpp"

#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "version.lib")


struct WidgetEntry
{
    std::wstring name;
    std::wstring scriptPath;
    bool loaded = false;
};

struct AddonEntry
{
    std::wstring name;
    std::wstring author;
    std::wstring version;
    std::wstring copyright;
};

static HWND g_list = nullptr;
static std::vector<WidgetEntry> g_widgets;
static HFONT g_buttonFont = nullptr;
static const int kWindowWidth = 720;
static const int kWindowHeight = 480;
static const UINT kTrayMessage = WM_APP + 1;
static const UINT kTrayMenuCommand = WM_APP + 2;
static const int kTrayMenuExitId = 500;
static const int kTrayMenuShowHideId = 501;
static const int kTrayMenuHomeWebsiteId = 510;
static const int kTrayMenuHomeDonateId = 511;
static const int kTrayMenuDocsMainId = 520;
static const int kTrayMenuDocsApiId = 521;
static const int kTrayMenuSettingsOpenId = 530;
static const int kTrayMenuSettingsCheckUpdatesId = 531;
static NOTIFYICONDATAW g_tray{};
static PROCESS_INFORMATION g_novadeskProcess{};
static bool g_novadeskRunning = false;
static HMENU g_trayMenu = nullptr;
static HWND g_btnRefreshList = nullptr;
static HWND g_btnClearLogs = nullptr;
static HWND g_btnLoad = nullptr;
static HWND g_btnRefresh = nullptr;
static HWND g_btnRefreshAll = nullptr;
static HWND g_btnClose = nullptr;
static HWND g_tab = nullptr;
static HWND g_logsList = nullptr;
static HWND g_addonsList = nullptr;
static HWND g_settingsPanel = nullptr;
static HWND g_aboutPanel = nullptr;
static HWND g_settingsTitle = nullptr;
static HWND g_chkRunOnStartup = nullptr;
static HWND g_btnCheckUpdates = nullptr;
static HWND g_chkAutoUpdate = nullptr;
static HWND g_chkEnableLogging = nullptr;
static HWND g_chkEnableDebugging = nullptr;
static HWND g_chkSaveLogToFile = nullptr;
static HWND g_chkUseHardwareAcceleration = nullptr;
static HWND g_aboutLogo = nullptr;
static HWND g_aboutAppName = nullptr;
static HWND g_aboutVersion = nullptr;
static HWND g_aboutCopyright = nullptr;
static HWND g_aboutDivider = nullptr;
static HWND g_aboutHomeLabel = nullptr;
static HWND g_aboutHomeLink = nullptr;
static HWND g_aboutDocsLabel = nullptr;
static HWND g_aboutDocsLink = nullptr;
static HWND g_aboutSupportGroup = nullptr;
static HWND g_aboutSupportTitle = nullptr;
static HWND g_aboutSupportText = nullptr;
static HWND g_aboutDonateButton = nullptr;
static HWND g_aboutBottomNote = nullptr;
static HICON g_windowIconLarge = nullptr;
static HICON g_windowIconSmall = nullptr;
static HWND g_mainWindow = nullptr;
static HANDLE g_novadeskLogRead = nullptr;
static HANDLE g_novadeskLogWrite = nullptr;
static HANDLE g_novadeskLogThread = nullptr;
static std::wstring g_pendingLogLine;
static int g_activeTab = 0;
static int g_logsMessageColumnWidth = 600;
static std::unordered_map<std::wstring, std::wstring> g_savedLoadedScripts;
static bool g_manageAutoCheckForUpdates = false;
static std::wstring g_lastNotifiedUpdateVersion;
static bool g_updateDialogOpen = false;
static bool g_isCheckingForUpdates = false;
static bool g_forceExitOnClose = false;
static ULONG_PTR g_gdiplusToken = 0;
static HBITMAP g_aboutLogoBitmap = nullptr;
static HFONT g_aboutTitleFont = nullptr;
static HFONT g_aboutSectionFont = nullptr;
static HFONT g_aboutBodyFont = nullptr;

static const int kControlIdRefreshList = 101;
static const int kControlIdClearLogs = 108;
static const int kControlIdLoad = 102;
static const int kControlIdRefresh = 104;
static const int kControlIdRefreshAll = 105;
static const int kControlIdClose = 107;
static const int kControlIdTabs = 106;
static const int kControlIdRunOnStartup = 301;
static const int kControlIdCheckUpdates = 302;
static const int kControlIdAutoCheckUpdates = 303;
static const int kControlIdEnableLogging = 304;
static const int kControlIdEnableDebugging = 305;
static const int kControlIdSaveLogToFile = 306;
static const int kControlIdUseHardwareAcceleration = 307;
static const int kControlIdAboutHome = 201;
static const int kControlIdAboutDocs = 202;
static const int kControlIdAboutDonate = 203;
static const int kLogMenuCopyMessage = 401;
static const int kLogMenuCopyFullLog = 402;
static const UINT kLogAppendMessage = WM_APP + 20;
static const UINT_PTR kLogsRefreshTimerId = 1;
static const UINT_PTR kAutoUpdateTimerId = 2;
static const UINT_PTR kStartupSyncTimerId = 3;
static const UINT kStartupSyncDelayMs = 120;
static const UINT kAutoUpdateIntervalMs = 60 * 1000; // 1 minute
static const int kMaxLogRows = 2000;
static const wchar_t *kCurrentVersion = L"0.9.0.0";
static const wchar_t *kSingleInstanceLockArg = L"--request-single-instance-lock";
static const wchar_t *kManageWindowClassName = L"NovadeskManagerWindow";
static const wchar_t *kManageCloseMessageName = L"Novadesk.Manage.RequestClose";
static UINT g_manageCloseMessage = 0;
static int ShowManageMessageBox(const std::wstring &message, const std::wstring &title, UINT type);
static bool ExecuteNovadeskCommand(const std::wstring &cmd, const std::wstring &path);
static bool ExecuteNovadeskCommandNoPath(const std::wstring &cmd);
static std::wstring GetAboutLogoPath();
static void ConfigureAutoUpdateTimer(HWND hWnd);
static void RequestAppExit(HWND hWnd);

static void InitGdiPlus()
{
    if (g_gdiplusToken != 0)
    {
        return;
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
}

static void ShutdownGdiPlus()
{
    if (g_gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

static HBITMAP LoadScaledBitmapFromPng(const std::wstring &path, int maxWidth, int maxHeight)
{
    if (path.empty() || maxWidth <= 0 || maxHeight <= 0)
    {
        return nullptr;
    }

    Gdiplus::Bitmap source(path.c_str());
    if (source.GetLastStatus() != Gdiplus::Ok)
    {
        return nullptr;
    }

    const UINT srcW = source.GetWidth();
    const UINT srcH = source.GetHeight();
    if (srcW == 0 || srcH == 0)
    {
        return nullptr;
    }

    const double scaleW = static_cast<double>(maxWidth) / static_cast<double>(srcW);
    const double scaleH = static_cast<double>(maxHeight) / static_cast<double>(srcH);
    const double scale = (scaleW < scaleH) ? scaleW : scaleH;
    int dstW = static_cast<int>(srcW * scale);
    int dstH = static_cast<int>(srcH * scale);
    if (dstW < 1)
        dstW = 1;
    if (dstH < 1)
        dstH = 1;

    Gdiplus::Bitmap canvas(dstW, dstH, PixelFormat32bppPARGB);
    Gdiplus::Graphics graphics(&canvas);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.DrawImage(&source, Gdiplus::Rect(0, 0, dstW, dstH));

    HBITMAP outBitmap = nullptr;
    if (canvas.GetHBITMAP(Gdiplus::Color::Transparent, &outBitmap) != Gdiplus::Ok)
    {
        return nullptr;
    }
    return outBitmap;
}

static void UpdateAboutLogo()
{
    if (!g_aboutLogo)
    {
        return;
    }

    if (g_aboutLogoBitmap)
    {
        DeleteObject(g_aboutLogoBitmap);
        g_aboutLogoBitmap = nullptr;
    }

    g_aboutLogoBitmap = LoadScaledBitmapFromPng(GetAboutLogoPath(), 92, 92);
    SendMessageW(g_aboutLogo, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(g_aboutLogoBitmap));
}

static void OpenUrl(const wchar_t *url)
{
    if (!url || !url[0])
    {
        return;
    }
    ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH + 1] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::filesystem::path p(buf);
    return p.parent_path().wstring();
}

static std::wstring JoinPath(const std::wstring &a, const std::wstring &b)
{
    std::filesystem::path p(a);
    p /= b;
    return p.wstring();
}

static std::wstring GetAboutLogoPath()
{
    return JoinPath(GetExeDir(), L"images\\Novadesk.png");
}

static std::wstring EnsureSingleInstanceArg(const std::wstring &args)
{
    if (args.find(kSingleInstanceLockArg) != std::wstring::npos)
    {
        return args;
    }
    if (args.empty())
    {
        return std::wstring(kSingleInstanceLockArg);
    }
    return args + L" " + kSingleInstanceLockArg;
}

static bool LaunchRestartNovadesk()
{
    const std::wstring restartExe = JoinPath(GetExeDir(), L"restart_novadesk.exe");
    DWORD attrs = GetFileAttributesW(restartExe.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        ShowManageMessageBox(L"restart_novadesk.exe not found in the app folder.", L"Manage Novadesk", MB_OK | MB_ICONWARNING);
        return false;
    }

    std::wstring cmdLine = L"\"" + restartExe + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, GetExeDir().c_str(), &si, &pi))
    {
        ShowManageMessageBox(L"Failed to launch restart_novadesk.exe.", L"Manage Novadesk", MB_OK | MB_ICONWARNING);
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

static void PromptRestartForHardwareAcceleration()
{
    const int kRestartNowButtonId = 1001;
    const TASKDIALOG_BUTTON buttons[] = {
        {kRestartNowButtonId, L"Restart Now"},
        {IDCANCEL, L"Later"},
    };

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = g_mainWindow;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.dwCommonButtons = 0;
    config.pszWindowTitle = L"Manage Novadesk";
    config.pszMainInstruction = L"Novadesk needs restart for this change.";
    config.pszContent = L"Hardware acceleration changes apply after restarting Novadesk.";
    config.cButtons = ARRAYSIZE(buttons);
    config.pButtons = buttons;
    config.nDefaultButton = kRestartNowButtonId;

    int selectedButton = IDCANCEL;
    HRESULT hr = TaskDialogIndirect(&config, &selectedButton, nullptr, nullptr);
    if (SUCCEEDED(hr))
    {
        if (selectedButton == kRestartNowButtonId)
        {
            LaunchRestartNovadesk();
        }
        return;
    }

    const int fallback = ShowManageMessageBox(
        L"Novadesk needs restart for this change.\n\nRestart now?",
        L"Manage Novadesk",
        MB_YESNO | MB_ICONINFORMATION);
    if (fallback == IDYES)
    {
        LaunchRestartNovadesk();
    }
}

static void LoadWindowIcons(HINSTANCE hInstance)
{
    if (!g_windowIconLarge)
    {
        g_windowIconLarge = static_cast<HICON>(
            LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
                       GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    }
    if (!g_windowIconSmall)
    {
        g_windowIconSmall = static_cast<HICON>(
            LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
                       GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    }
}

static void LogLine(const std::wstring &line)
{
    OutputDebugStringW((line + L"\n").c_str());
}

static std::wstring Win32ErrorToString(DWORD err)
{
    wchar_t buf[512] = {};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, 512, nullptr);
    return std::wstring(buf);
}

static std::wstring NormalizePathLower(const std::wstring &p)
{
    std::error_code ec;
    std::filesystem::path fp(p);
    fp = std::filesystem::absolute(fp, ec).lexically_normal();
    std::wstring out = fp.wstring();
    std::transform(out.begin(), out.end(), out.begin(), ::towlower);
    return out;
}

static std::wstring BytesToWide(const std::string &bytes)
{
    if (bytes.empty())
        return L"";

    auto convert = [&](UINT cp) -> std::wstring
    {
        int needed = MultiByteToWideChar(cp, 0, bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
        if (needed <= 0)
            return L"";
        std::wstring wide(needed, L'\0');
        MultiByteToWideChar(cp, 0, bytes.c_str(), static_cast<int>(bytes.size()), wide.data(), needed);
        return wide;
    };

    std::wstring utf8 = convert(CP_UTF8);
    if (!utf8.empty())
        return utf8;
    return convert(CP_ACP);
}

static std::string WideToUtf8(const std::wstring &wide)
{
    if (wide.empty())
        return std::string();
    int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
        return std::string();
    std::string out(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), needed, nullptr, nullptr);
    return out;
}

static std::wstring Utf8ToWide(const std::string &utf8)
{
    if (utf8.empty())
        return std::wstring();
    int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0)
        return std::wstring();
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), out.data(), needed);
    return out;
}

static DWORD WINAPI NovadeskLogReaderThreadProc(LPVOID)
{
    HANDLE readPipe = g_novadeskLogRead;
    if (!readPipe)
        return 0;

    std::vector<char> buffer(4096);
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
    {
        std::wstring text = BytesToWide(std::string(buffer.data(), bytesRead));
        if (text.empty())
            continue;

        auto *payload = new std::wstring(std::move(text));
        if (!PostMessageW(g_mainWindow, kLogAppendMessage, 0, reinterpret_cast<LPARAM>(payload)))
        {
            delete payload;
            break;
        }
    }
    return 0;
}

static void StopNovadeskLogCapture()
{
    if (g_novadeskLogRead)
    {
        CloseHandle(g_novadeskLogRead);
        g_novadeskLogRead = nullptr;
    }
    if (g_novadeskLogWrite)
    {
        CloseHandle(g_novadeskLogWrite);
        g_novadeskLogWrite = nullptr;
    }
    if (g_novadeskLogThread)
    {
        WaitForSingleObject(g_novadeskLogThread, 1500);
        CloseHandle(g_novadeskLogThread);
        g_novadeskLogThread = nullptr;
    }
}

static void RequestAppExit(HWND hWnd)
{
    g_forceExitOnClose = true;
    PostMessageW(hWnd, WM_CLOSE, 0, 0);
}

static bool IsPortableMode()
{
    const std::wstring exeDir = GetExeDir();
    const std::wstring settingsPath = JoinPath(exeDir, L"manage_novadesk_settings.json");
    std::error_code ec;
    if (!std::filesystem::exists(settingsPath, ec) || ec)
    {
        return false;
    }

    std::ifstream in(std::filesystem::path(settingsPath), std::ios::binary);
    if (!in.is_open())
    {
        return true;
    }

    nlohmann::json doc;
    try
    {
        in >> doc;
    }
    catch (...)
    {
        return true;
    }

    if (doc.is_object() && doc.contains("isPortable") && doc["isPortable"].is_boolean())
    {
        return doc["isPortable"].get<bool>();
    }

    return true;
}

static std::wstring GetWidgetsRoot()
{
    if (IsPortableMode())
    {
        return JoinPath(GetExeDir(), L"Widgets");
    }

    PWSTR docs = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs)
    {
        out = std::wstring(docs) + L"\\Novadesk\\Widgets";
        CoTaskMemFree(docs);
    }
    return out;
}

static std::wstring GetAddonsRoot()
{
    if (IsPortableMode())
    {
        return JoinPath(GetExeDir(), L"Addons");
    }

    PWSTR docs = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs)
    {
        out = std::wstring(docs) + L"\\Novadesk\\Addons";
        CoTaskMemFree(docs);
    }
    return out;
}

static std::wstring GetAddonVersionField(const std::wstring &dllPath, const wchar_t *fieldName)
{
    if (!fieldName || !fieldName[0])
    {
        return L"";
    }

    DWORD handle = 0;
    const DWORD infoSize = GetFileVersionInfoSizeW(dllPath.c_str(), &handle);
    if (infoSize == 0)
    {
        return L"";
    }

    std::vector<BYTE> buffer(infoSize);
    if (!GetFileVersionInfoW(dllPath.c_str(), 0, infoSize, buffer.data()))
    {
        return L"";
    }

    struct LangCodePage
    {
        WORD language;
        WORD codePage;
    };

    LangCodePage *translation = nullptr;
    UINT translationLen = 0;
    if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<LPVOID *>(&translation), &translationLen) ||
        !translation || translationLen < sizeof(LangCodePage))
    {
        return L"";
    }

    wchar_t queryPath[256] = {};
    swprintf_s(
        queryPath,
        L"\\StringFileInfo\\%04x%04x\\%s",
        translation[0].language,
        translation[0].codePage,
        fieldName);

    LPWSTR value = nullptr;
    UINT valueLen = 0;
    if (!VerQueryValueW(buffer.data(), queryPath, reinterpret_cast<LPVOID *>(&value), &valueLen) || !value || valueLen == 0)
    {
        return L"";
    }

    return std::wstring(value);
}

static std::vector<AddonEntry> LoadAddons()
{
    std::vector<AddonEntry> addons;
    const std::wstring root = GetAddonsRoot();
    if (root.empty())
    {
        return addons;
    }

    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || ec)
    {
        return addons;
    }

    for (const auto &entry : std::filesystem::directory_iterator(root, ec))
    {
        if (ec)
        {
            break;
        }
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::wstring ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext != L".dll")
        {
            continue;
        }

        const std::wstring dllPath = entry.path().wstring();
        const std::wstring fileStem = entry.path().stem().wstring();
        AddonEntry addon{};
        addon.name = GetAddonVersionField(dllPath, L"FileDescription");
        if (addon.name.empty())
        {
            addon.name = fileStem;
        }
        addon.author = GetAddonVersionField(dllPath, L"CompanyName");
        if (addon.author.empty())
        {
            addon.author = L"-";
        }
        addon.version = GetAddonVersionField(dllPath, L"FileVersion");
        if (addon.version.empty())
        {
            addon.version = L"-";
        }
        addon.copyright = GetAddonVersionField(dllPath, L"LegalCopyright");
        if (addon.copyright.empty())
        {
            addon.copyright = L"-";
        }

        addons.push_back(std::move(addon));
    }

    std::sort(addons.begin(), addons.end(), [](const AddonEntry &a, const AddonEntry &b) {
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return addons;
}

static void RefreshAddonsView()
{
    if (!g_addonsList)
    {
        return;
    }

    ListView_DeleteAllItems(g_addonsList);
    const auto addons = LoadAddons();
    for (int i = 0; i < static_cast<int>(addons.size()); ++i)
    {
        const auto &addon = addons[static_cast<size_t>(i)];
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = const_cast<wchar_t *>(addon.name.c_str());
        const int row = ListView_InsertItem(g_addonsList, &item);
        ListView_SetItemText(g_addonsList, row, 1, const_cast<wchar_t *>(addon.author.c_str()));
        ListView_SetItemText(g_addonsList, row, 2, const_cast<wchar_t *>(addon.version.c_str()));
        ListView_SetItemText(g_addonsList, row, 3, const_cast<wchar_t *>(addon.copyright.c_str()));
    }
}

static std::wstring GetNovadeskExePath()
{
    return JoinPath(GetExeDir(), L"Novadesk.exe");
}

static std::wstring GetManageExePath()
{
    wchar_t buf[MAX_PATH + 1] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::wstring(buf);
}

static std::wstring GetManageSettingsPath()
{
    const std::wstring exePath = JoinPath(GetExeDir(), L"manage_novadesk_settings.json");
    std::error_code ec;
    if (std::filesystem::exists(std::filesystem::path(exePath), ec) && !ec)
    {
        return exePath;
    }

    PWSTR roaming = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming)) && roaming)
    {
        const std::wstring appDataDir = std::wstring(roaming) + L"\\Novadesk";
        CoTaskMemFree(roaming);
        std::filesystem::create_directories(std::filesystem::path(appDataDir), ec);
        if (!ec)
        {
            return JoinPath(appDataDir, L"manage_novadesk_settings.json");
        }
    }

    // Fallback: keep using EXE directory path if AppData path is unavailable.
    return exePath;
}

static std::wstring GetNovadeskAppDataDir()
{
    PWSTR roaming = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming)) && roaming)
    {
        const std::wstring appDataDir = std::wstring(roaming) + L"\\Novadesk";
        CoTaskMemFree(roaming);
        return appDataDir;
    }
    return L"";
}

static std::wstring GetNovadeskSettingsPath()
{
    const std::wstring exePath = JoinPath(GetExeDir(), L"settings.json");
    std::error_code ec;
    if (std::filesystem::exists(std::filesystem::path(exePath), ec) && !ec)
    {
        return exePath;
    }

    const std::wstring appDataDir = GetNovadeskAppDataDir();
    if (!appDataDir.empty())
    {
        const std::wstring appDataPath = JoinPath(appDataDir, L"settings.json");
        if (std::filesystem::exists(std::filesystem::path(appDataPath), ec) && !ec)
        {
            return appDataPath;
        }
        return appDataPath;
    }

    return exePath;
}

static bool LoadNovadeskSettings(nlohmann::json &doc, std::wstring &pathUsed)
{
    pathUsed = GetNovadeskSettingsPath();
    std::ifstream in(std::filesystem::path(pathUsed), std::ios::binary);
    if (!in.is_open())
    {
        doc = nlohmann::json::object();
        return false;
    }

    try
    {
        in >> doc;
        if (!doc.is_object())
        {
            doc = nlohmann::json::object();
        }
        return true;
    }
    catch (...)
    {
        doc = nlohmann::json::object();
        return false;
    }
}

static bool SaveNovadeskSettings(const nlohmann::json &doc, const std::wstring &path)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }
    out << doc.dump(4);
    return true;
}

static bool GetNovadeskSettingBool(const std::string &key, bool defaultValue)
{
    nlohmann::json doc;
    std::wstring path;
    LoadNovadeskSettings(doc, path);
    if (doc.contains(key) && doc[key].is_boolean())
    {
        return doc[key].get<bool>();
    }
    return defaultValue;
}

static bool SetNovadeskSettingBool(const std::string &key, bool value)
{
    nlohmann::json doc;
    std::wstring path;
    LoadNovadeskSettings(doc, path);
    doc[key] = value;
    return SaveNovadeskSettings(doc, path);
}

static bool IsRunOnStartupEnabled()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t value[2048] = {};
    DWORD valueSize = sizeof(value);
    DWORD type = 0;
    const LONG rc = RegQueryValueExW(hKey, L"Novadesk", nullptr, &type, reinterpret_cast<LPBYTE>(value), &valueSize);
    RegCloseKey(hKey);

    if (rc != ERROR_SUCCESS || type != REG_SZ)
    {
        return false;
    }

    const std::wstring expectedExe = GetManageExePath();
    std::wstring configured = value;
    configured.erase(std::remove(configured.begin(), configured.end(), L'"'), configured.end());
    return _wcsicmp(configured.c_str(), expectedExe.c_str()) == 0;
}

static bool SetRunOnStartupEnabled(bool enabled)
{
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }

    bool ok = true;
    if (enabled)
    {
        const std::wstring value = L"\"" + GetManageExePath() + L"\"";
        const LONG rc = RegSetValueExW(hKey, L"Novadesk", 0, REG_SZ,
                                       reinterpret_cast<const BYTE *>(value.c_str()),
                                       static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        ok = (rc == ERROR_SUCCESS);
    }
    else
    {
        const LONG rc = RegDeleteValueW(hKey, L"Novadesk");
        ok = (rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hKey);
    return ok;
}

static std::wstring HttpGetText(const std::wstring &url)
{
    HINTERNET hInternet = InternetOpenW(L"ManageNovadesk/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet)
    {
        return L"";
    }

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl)
    {
        InternetCloseHandle(hInternet);
        return L"";
    }

    std::string bytes;
    char buffer[4096];
    DWORD read = 0;
    while (InternetReadFile(hUrl, buffer, static_cast<DWORD>(sizeof(buffer)), &read) && read > 0)
    {
        bytes.append(buffer, buffer + read);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return BytesToWide(bytes);
}

static std::vector<int> ParseVersion(const std::wstring &value)
{
    std::wstring s = value;
    if (!s.empty() && (s[0] == L'v' || s[0] == L'V'))
    {
        s.erase(s.begin());
    }
    for (wchar_t &ch : s)
    {
        if (!(ch >= L'0' && ch <= L'9') && ch != L'.')
        {
            ch = L'.';
        }
    }

    std::vector<int> parts;
    std::wstringstream ss(s);
    std::wstring token;
    while (std::getline(ss, token, L'.'))
    {
        if (token.empty())
            continue;
        try
        {
            parts.push_back(std::stoi(token));
        }
        catch (...)
        {
            parts.push_back(0);
        }
    }
    return parts;
}

static bool IsVersionNewer(const std::wstring &candidate, const std::wstring &current)
{
    std::vector<int> a = ParseVersion(candidate);
    std::vector<int> b = ParseVersion(current);
    const size_t n = (a.size() > b.size()) ? a.size() : b.size();
    a.resize(n, 0);
    b.resize(n, 0);
    for (size_t i = 0; i < n; ++i)
    {
        if (a[i] > b[i])
            return true;
        if (a[i] < b[i])
            return false;
    }
    return false;
}

static bool GetLatestGithubRelease(std::wstring &tag, std::wstring &assetUrl, std::wstring &assetName)
{
    tag.clear();
    assetUrl.clear();
    assetName.clear();

    const std::wstring jsonText = HttpGetText(L"https://api.github.com/repos/Official-Novadesk/novadesk/releases/latest");
    if (jsonText.empty())
    {
        return false;
    }
    try
    {
        nlohmann::json doc = nlohmann::json::parse(WideToUtf8(jsonText));
        if (doc.contains("tag_name") && doc["tag_name"].is_string())
        {
            tag = Utf8ToWide(doc["tag_name"].get<std::string>());
        }
        if (doc.contains("assets") && doc["assets"].is_array())
        {
            for (const auto &asset : doc["assets"])
            {
                if (!asset.is_object())
                    continue;
                if (!asset.contains("name") || !asset.contains("browser_download_url"))
                    continue;
                if (!asset["name"].is_string() || !asset["browser_download_url"].is_string())
                    continue;

                const std::wstring name = Utf8ToWide(asset["name"].get<std::string>());
                const std::wstring url = Utf8ToWide(asset["browser_download_url"].get<std::string>());
                std::wstring lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
                if (lowerName.find(L".exe") != std::wstring::npos || lowerName.find(L".msi") != std::wstring::npos)
                {
                    assetName = name;
                    assetUrl = url;
                    break;
                }
            }
            if (assetUrl.empty())
            {
                for (const auto &asset : doc["assets"])
                {
                    if (!asset.is_object())
                        continue;
                    if (!asset.contains("name") || !asset.contains("browser_download_url"))
                        continue;
                    if (!asset["name"].is_string() || !asset["browser_download_url"].is_string())
                        continue;
                    assetName = Utf8ToWide(asset["name"].get<std::string>());
                    assetUrl = Utf8ToWide(asset["browser_download_url"].get<std::string>());
                    break;
                }
            }
        }
    }
    catch (...)
    {
        return false;
    }
    return !tag.empty();
}

static bool DownloadFileToPath(const std::wstring &url, const std::wstring &destinationPath)
{
    HINTERNET hInternet = InternetOpenW(L"ManageNovadesk/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet)
        return false;

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl)
    {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream out(std::filesystem::path(destinationPath), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD read = 0;
    while (InternetReadFile(hUrl, buffer, static_cast<DWORD>(sizeof(buffer)), &read) && read > 0)
    {
        out.write(buffer, read);
    }
    out.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return std::filesystem::exists(std::filesystem::path(destinationPath));
}

static void SetSettingsStatusText(const std::wstring &text)
{
    UNREFERENCED_PARAMETER(text);
}

static int ShowManageMessageBox(const std::wstring &message, const std::wstring &title, UINT type)
{
    HWND owner = nullptr;
    if (g_mainWindow && IsWindow(g_mainWindow) && IsWindowVisible(g_mainWindow))
    {
        owner = g_mainWindow;
    }

    const UINT flags = type | MB_SETFOREGROUND | MB_TOPMOST | (owner ? 0 : MB_TASKMODAL);
    return MessageBoxW(owner, message.c_str(), title.c_str(), flags);
}

static void SaveManageSettings()
{
    const std::wstring path = GetManageSettingsPath();
    std::vector<std::wstring> paths;
    paths.reserve(g_savedLoadedScripts.size());
    for (const auto &kv : g_savedLoadedScripts)
    {
        paths.push_back(kv.second);
    }
    std::sort(paths.begin(), paths.end());

    nlohmann::json doc = nlohmann::json::object();
    doc["autoCheckForUpdates"] = g_manageAutoCheckForUpdates;
    doc["loadedScripts"] = nlohmann::json::array();
    for (const auto &p : paths)
    {
        doc["loadedScripts"].push_back(WideToUtf8(p));
    }

    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        LogLine(L"[Manage] Failed to write settings file: " + path);
        return;
    }
    out << doc.dump(2);
}

static void RememberLoadedScriptState(const std::wstring &scriptPath, bool loaded)
{
    const std::wstring norm = NormalizePathLower(scriptPath);
    if (loaded)
    {
        g_savedLoadedScripts[norm] = scriptPath;
    }
    else
    {
        g_savedLoadedScripts.erase(norm);
    }
    SaveManageSettings();
}

static void LoadManageSettings()
{
    g_savedLoadedScripts.clear();
    g_manageAutoCheckForUpdates = false;
    const std::wstring path = GetManageSettingsPath();
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in.is_open())
    {
        return;
    }

    nlohmann::json doc;
    try
    {
        in >> doc;
    }
    catch (...)
    {
        return;
    }

    if (!doc.is_object())
    {
        return;
    }

    if (doc.contains("autoCheckForUpdates") && doc["autoCheckForUpdates"].is_boolean())
    {
        g_manageAutoCheckForUpdates = doc["autoCheckForUpdates"].get<bool>();
    }

    if (!doc.contains("loadedScripts") || !doc["loadedScripts"].is_array())
    {
        return;
    }

    for (const auto &v : doc["loadedScripts"])
    {
        if (!v.is_string())
            continue;
        std::wstring pathValue = Utf8ToWide(v.get<std::string>());
        if (pathValue.empty())
        {
            continue;
        }
        g_savedLoadedScripts[NormalizePathLower(pathValue)] = pathValue;
    }
}

static void ApplySavedLoadedScripts()
{
    for (const auto &kv : g_savedLoadedScripts)
    {
        ExecuteNovadeskCommand(L"--load", kv.second);
    }
}

static std::wstring CreateTempListPath()
{
    wchar_t tempPath[MAX_PATH + 1] = {};
    if (!GetTempPathW(MAX_PATH, tempPath))
        return L"";
    wchar_t filePath[MAX_PATH + 1] = {};
    if (!GetTempFileNameW(tempPath, L"nds", 0, filePath))
        return L"";
    return std::wstring(filePath);
}

static void StartNovadesk()
{
    if (g_novadeskRunning)
        return;

    const std::wstring exe = GetNovadeskExePath();
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&g_novadeskLogRead, &g_novadeskLogWrite, &sa, 0))
    {
        g_novadeskLogRead = nullptr;
        g_novadeskLogWrite = nullptr;
        LogLine(L"[Manage] Failed to create output pipe for Novadesk.");
    }
    else
    {
        SetHandleInformation(g_novadeskLogRead, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    if (g_novadeskLogWrite)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = g_novadeskLogWrite;
        si.hStdError = g_novadeskLogWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"\"" + exe + L"\" " + EnsureSingleInstanceArg(L"");

    if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        g_novadeskProcess = pi;
        g_novadeskRunning = true;
        if (g_novadeskLogWrite)
        {
            CloseHandle(g_novadeskLogWrite);
            g_novadeskLogWrite = nullptr;
            g_novadeskLogThread = CreateThread(nullptr, 0, NovadeskLogReaderThreadProc, nullptr, 0, nullptr);
        }
        LogLine(L"[Manage] Started Novadesk: " + exe);
    }
    else
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Failed to start Novadesk (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
        StopNovadeskLogCapture();
    }
}

static void StopNovadesk()
{
    if (!g_novadeskRunning)
    {
        StopNovadeskLogCapture();
        return;
    }

    TerminateProcess(g_novadeskProcess.hProcess, 0);
    CloseHandle(g_novadeskProcess.hProcess);
    CloseHandle(g_novadeskProcess.hThread);
    g_novadeskProcess = {};
    g_novadeskRunning = false;
    StopNovadeskLogCapture();
    LogLine(L"[Manage] Stopped Novadesk process.");
}

static void AddTrayIcon(HWND hWnd)
{
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = hWnd;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray.uCallbackMessage = kTrayMessage;
    g_tray.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK));
    wcscpy_s(g_tray.szTip, L"Manage Novadesk");

    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

static void RemoveTrayIcon()
{
    if (g_tray.hWnd)
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
    if (g_tray.hIcon)
        DestroyIcon(g_tray.hIcon);
    g_tray = {};
}

static void EnsureTrayMenu()
{
    if (g_trayMenu)
        return;
    g_trayMenu = CreatePopupMenu();
    HMENU homeMenu = CreatePopupMenu();
    AppendMenuW(homeMenu, MF_STRING, kTrayMenuHomeWebsiteId, L"Official Website");
    AppendMenuW(homeMenu, MF_STRING, kTrayMenuHomeDonateId, L"Donate");

    HMENU docsMenu = CreatePopupMenu();
    AppendMenuW(docsMenu, MF_STRING, kTrayMenuDocsMainId, L"Documentation");
    AppendMenuW(docsMenu, MF_STRING, kTrayMenuDocsApiId, L"API Reference");

    HMENU settingsMenu = CreatePopupMenu();
    AppendMenuW(settingsMenu, MF_STRING, kTrayMenuSettingsOpenId, L"Open Settings");
    AppendMenuW(settingsMenu, MF_STRING, kTrayMenuSettingsCheckUpdatesId, L"Check for Updates");

    AppendMenuW(g_trayMenu, MF_STRING, kTrayMenuShowHideId, L"Show");
    AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(homeMenu), L"Home");
    AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(docsMenu), L"Docs");
    AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(settingsMenu), L"Settings");
    AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_trayMenu, MF_STRING, kTrayMenuExitId, L"Exit");
}

static void ShowTrayMenu(HWND hWnd)
{
    EnsureTrayMenu();
    ModifyMenuW(g_trayMenu, kTrayMenuShowHideId, MF_BYCOMMAND | MF_STRING, kTrayMenuShowHideId, IsWindowVisible(hWnd) ? L"Hide" : L"Show");
    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(g_trayMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    PostMessageW(hWnd, kTrayMenuCommand, 0, 0);
}

static std::wstring RunProcessCaptureStdout(const std::wstring &exePath, const std::wstring &args)
{
    std::wstring cmd = L"\"" + exePath + L"\" " + EnsureSingleInstanceArg(args);
    LogLine(L"[Manage] Run capture: " + cmd);

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hRead = nullptr;
    HANDLE hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return L"";
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = cmd;
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        DWORD err = GetLastError();
        LogLine(L"[Manage] CreateProcess failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
        return L"";
    }

    CloseHandle(hWrite);

    std::wstring output;
    std::vector<char> buffer(4096);
    DWORD bytesRead = 0;
    while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
    {
        // Assume UTF-16LE from wcout
        if (bytesRead % 2 != 0)
            bytesRead -= 1;
        output.append(reinterpret_cast<wchar_t *>(buffer.data()), bytesRead / 2);
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    LogLine(L"[Manage] Capture output length: " + std::to_wstring(output.size()));
    return output;
}

static bool RunProcessWait(const std::wstring &exePath, const std::wstring &args)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"\"" + exePath + L"\" " + EnsureSingleInstanceArg(args);
    LogLine(L"[Manage] Run wait: " + cmdLine);
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Run wait failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

static void UpdateButtonState();
static void RefreshLogsView();
static void ApplyTabState();
static void OnToggleLoadSelected();
static void OnClearLogs();
static std::wstring GetListViewText(HWND list, int row, int subItem);
static bool CopyTextToClipboard(HWND owner, const std::wstring &text);
static void ShowLogsContextMenu(HWND hWnd, int x, int y);
static void AutoExpandLogsMessageColumn(const std::wstring &message);

static std::wstring GetListViewText(HWND list, int row, int subItem)
{
    if (!list || row < 0 || subItem < 0)
        return L"";

    int capacity = 256;
    for (int attempt = 0; attempt < 6; ++attempt)
    {
        std::vector<wchar_t> buf(static_cast<size_t>(capacity), L'\0');
        LVITEMW item{};
        item.iSubItem = subItem;
        item.pszText = buf.data();
        item.cchTextMax = capacity;
        const int copied = static_cast<int>(SendMessageW(list, LVM_GETITEMTEXTW, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&item)));
        if (copied < capacity - 1)
        {
            return std::wstring(buf.data(), static_cast<size_t>(copied));
        }
        capacity *= 2;
    }
    return L"";
}

static bool CopyTextToClipboard(HWND owner, const std::wstring &text)
{
    if (text.empty())
        return false;

    if (!OpenClipboard(owner))
        return false;

    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!mem)
    {
        CloseClipboard();
        return false;
    }

    void *dst = GlobalLock(mem);
    if (!dst)
    {
        GlobalFree(mem);
        CloseClipboard();
        return false;
    }

    memcpy(dst, text.c_str(), bytes);
    GlobalUnlock(mem);

    if (!SetClipboardData(CF_UNICODETEXT, mem))
    {
        GlobalFree(mem);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

static void AutoExpandLogsMessageColumn(const std::wstring &message)
{
    if (!g_logsList || message.empty())
        return;

    const int textWidth = ListView_GetStringWidth(g_logsList, message.c_str());
    if (textWidth <= 0)
        return;

    const int desiredWidth = textWidth + 24;
    if (desiredWidth > g_logsMessageColumnWidth)
    {
        g_logsMessageColumnWidth = desiredWidth;
        ListView_SetColumnWidth(g_logsList, 3, g_logsMessageColumnWidth);
    }
}

static void ShowLogsContextMenu(HWND hWnd, int x, int y)
{
    if (!g_logsList)
        return;

    POINT pt{x, y};
    if (pt.x == -1 && pt.y == -1)
    {
        const int selected = ListView_GetNextItem(g_logsList, -1, LVNI_SELECTED);
        if (selected < 0)
            return;
        RECT rc{};
        if (!ListView_GetItemRect(g_logsList, selected, &rc, LVIR_BOUNDS))
            return;
        POINT localPt{rc.left + 6, rc.top + 6};
        ClientToScreen(g_logsList, &localPt);
        pt = localPt;
    }
    else
    {
        POINT clientPt = pt;
        ScreenToClient(g_logsList, &clientPt);
        LVHITTESTINFO hit{};
        hit.pt = clientPt;
        const int hitIndex = ListView_SubItemHitTest(g_logsList, &hit);
        if (hitIndex >= 0)
        {
            ListView_SetItemState(g_logsList, -1, 0, LVIS_SELECTED);
            ListView_SetItemState(g_logsList, hitIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }

    const int row = ListView_GetNextItem(g_logsList, -1, LVNI_SELECTED);
    if (row < 0)
        return;

    const std::wstring time = GetListViewText(g_logsList, row, 0);
    const std::wstring source = GetListViewText(g_logsList, row, 1);
    const std::wstring level = GetListViewText(g_logsList, row, 2);
    const std::wstring message = GetListViewText(g_logsList, row, 3);
    const std::wstring full = L"[" + time + L"] [" + source + L"] [" + level + L"] " + message;

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, kLogMenuCopyMessage, L"Copy Message");
    AppendMenuW(menu, MF_STRING, kLogMenuCopyFullLog, L"Copy Full Log");
    SetForegroundWindow(hWnd);
    const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(menu);
    // Ensure the popup menu is dismissed reliably after TrackPopupMenu.
    PostMessageW(hWnd, WM_NULL, 0, 0);

    if (cmd == kLogMenuCopyMessage)
    {
        CopyTextToClipboard(hWnd, message);
    }
    else if (cmd == kLogMenuCopyFullLog)
    {
        CopyTextToClipboard(hWnd, full);
    }
}

static void AppendLogRow(const std::wstring &time, const std::wstring &source, const std::wstring &level, const std::wstring &message)
{
    if (!g_logsList)
        return;

    const int first = ListView_GetItemCount(g_logsList) > 0 ? 0 : -1;
    if (first >= 0)
    {
        const std::wstring firstTime = GetListViewText(g_logsList, first, 0);
        const std::wstring firstSource = GetListViewText(g_logsList, first, 1);
        const std::wstring firstLevel = GetListViewText(g_logsList, first, 2);
        const std::wstring firstMessage = GetListViewText(g_logsList, first, 3);
        if (time == firstTime && source == firstSource && level == firstLevel && message == firstMessage)
        {
            return;
        }
    }

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.pszText = const_cast<wchar_t *>(time.c_str());
    int row = ListView_InsertItem(g_logsList, &item);
    ListView_SetItemText(g_logsList, row, 1, const_cast<wchar_t *>(source.c_str()));
    ListView_SetItemText(g_logsList, row, 2, const_cast<wchar_t *>(level.c_str()));
    ListView_SetItemText(g_logsList, row, 3, const_cast<wchar_t *>(message.c_str()));
    AutoExpandLogsMessageColumn(message);

    while (ListView_GetItemCount(g_logsList) > kMaxLogRows)
    {
        const int last = ListView_GetItemCount(g_logsList) - 1;
        if (last >= 0)
        {
            ListView_DeleteItem(g_logsList, last);
        }
        else
        {
            break;
        }
    }

    if (ListView_GetItemCount(g_logsList) > 0)
    {
        ListView_EnsureVisible(g_logsList, 0, FALSE);
    }
}

static void ParseAndAppendLogLine(const std::wstring &line)
{
    if (line.empty())
        return;

    std::wstring trimmed = line;
    while (!trimmed.empty() && (trimmed.back() == L'\r' || trimmed.back() == L'\n'))
    {
        trimmed.pop_back();
    }
    if (trimmed.empty())
        return;

    std::wstring time;
    std::wstring source;
    std::wstring level;
    std::wstring message = trimmed;

    size_t p0s = trimmed.find(L'[');
    size_t p0e = (p0s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p0s + 1);
    size_t p1s = (p0e == std::wstring::npos) ? std::wstring::npos : trimmed.find(L'[', p0e + 1);
    size_t p1e = (p1s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p1s + 1);
    size_t p2s = (p1e == std::wstring::npos) ? std::wstring::npos : trimmed.find(L'[', p1e + 1);
    size_t p2e = (p2s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p2s + 1);

    if (p0s != std::wstring::npos && p0e != std::wstring::npos &&
        p1s != std::wstring::npos && p1e != std::wstring::npos &&
        p2s != std::wstring::npos && p2e != std::wstring::npos)
    {
        time = trimmed.substr(p0s + 1, p0e - p0s - 1);
        source = trimmed.substr(p1s + 1, p1e - p1s - 1);
        level = trimmed.substr(p2s + 1, p2e - p2s - 1);
        size_t msgStart = p2e + 1;
        while (msgStart < trimmed.size() && trimmed[msgStart] == L' ')
        {
            ++msgStart;
        }
        message = trimmed.substr(msgStart);
    }

    AppendLogRow(time, source, level, message);
}

static void ProcessLogChunk(const std::wstring &chunk)
{
    if (chunk.empty())
        return;

    g_pendingLogLine += chunk;
    size_t start = 0;
    for (;;)
    {
        size_t nl = g_pendingLogLine.find(L'\n', start);
        if (nl == std::wstring::npos)
            break;
        std::wstring line = g_pendingLogLine.substr(start, nl - start);
        ParseAndAppendLogLine(line);
        start = nl + 1;
    }
    if (start > 0)
    {
        g_pendingLogLine.erase(0, start);
    }
}

static std::unordered_set<std::wstring> GetRunningScripts()
{
    std::unordered_set<std::wstring> out;
    std::wstring exe = GetNovadeskExePath();
    const std::wstring tempList = CreateTempListPath();
    if (tempList.empty())
    {
        LogLine(L"[Manage] list-scripts temp file not created.");
        return out;
    }
    const std::wstring args = L"--list-scripts-file \"" + tempList + L"\"";
    RunProcessWait(exe, args);

    std::wifstream in(tempList.c_str());
    if (in.is_open())
    {
        std::wstring line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            LogLine(L"[Manage] list-scripts: " + line);
            out.insert(NormalizePathLower(line));
        }
        in.close();
    }
    else
    {
        LogLine(L"[Manage] list-scripts failed to open: " + tempList);
    }
    DeleteFileW(tempList.c_str());
    LogLine(L"[Manage] Running scripts count: " + std::to_wstring(out.size()));
    return out;
}

static std::vector<WidgetEntry> LoadWidgets()
{
    std::vector<WidgetEntry> list;
    const std::wstring root = GetWidgetsRoot();
    if (root.empty())
        return list;

    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return list;

    auto running = GetRunningScripts();

    for (const auto &entry : std::filesystem::directory_iterator(root, ec))
    {
        if (ec)
            break;
        if (!entry.is_directory())
            continue;

        std::filesystem::path dir = entry.path();
        std::filesystem::path index = dir / L"index.js";
        if (!std::filesystem::exists(index, ec))
            continue;

        WidgetEntry w{};
        w.name = dir.filename().wstring();
        w.scriptPath = index.wstring();
        w.loaded = running.find(NormalizePathLower(w.scriptPath)) != running.end();
        list.push_back(std::move(w));
    }

    return list;
}

static bool ExecuteNovadeskCommand(const std::wstring &cmd, const std::wstring &path)
{
    const std::wstring exe = GetNovadeskExePath();
    const std::wstring args = cmd + L" \"" + path + L"\"";
    const bool ok = RunProcessWait(exe, args);
    LogLine(ok ? L"[Manage] Exec OK." : L"[Manage] Exec failed.");
    return ok;
}

static void RefreshListView()
{
    g_widgets = LoadWidgets();
    LogLine(L"[Manage] Refresh list. Widgets: " + std::to_wstring(g_widgets.size()));

    ListView_DeleteAllItems(g_list);
    int idx = 0;
    for (const auto &w : g_widgets)
    {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.pszText = const_cast<wchar_t *>(w.name.c_str());
        ListView_InsertItem(g_list, &item);

        ListView_SetItemText(g_list, idx, 1, const_cast<wchar_t *>(w.scriptPath.c_str()));
        std::wstring status = w.loaded ? L"Loaded" : L"Not Loaded";
        ListView_SetItemText(g_list, idx, 2, const_cast<wchar_t *>(status.c_str()));
        LogLine(L"[Manage] Widget: " + w.name + L" | " + w.scriptPath + L" | " + (w.loaded ? L"Loaded" : L"Not Loaded"));
        ++idx;
    }
    UpdateButtonState();
}

static int GetSelectedIndex()
{
    return ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
}

static void UpdateButtonState()
{
    if (g_activeTab != 0)
    {
        EnableWindow(g_btnLoad, FALSE);
        EnableWindow(g_btnRefresh, FALSE);
        EnableWindow(g_btnRefreshAll, FALSE);
        SetWindowTextW(g_btnLoad, L"Load");
        return;
    }

    EnableWindow(g_btnRefreshAll, TRUE);

    const int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        EnableWindow(g_btnLoad, FALSE);
        EnableWindow(g_btnRefresh, FALSE);
        SetWindowTextW(g_btnLoad, L"Load");
        return;
    }

    const bool isLoaded = g_widgets[idx].loaded;
    SetWindowTextW(g_btnLoad, isLoaded ? L"Unload" : L"Load");
    EnableWindow(g_btnLoad, TRUE);
    EnableWindow(g_btnRefresh, isLoaded ? TRUE : FALSE);
}

static void RefreshLogsView()
{
    if (!g_logsList)
        return;
}

static void OnClearLogs()
{
    if (!g_logsList)
        return;

    ListView_DeleteAllItems(g_logsList);
    g_pendingLogLine.clear();
}

static bool ExecuteNovadeskCommandNoPath(const std::wstring &cmd)
{
    const std::wstring exe = GetNovadeskExePath();
    const bool ok = RunProcessWait(exe, cmd);
    LogLine(ok ? L"[Manage] Exec OK." : L"[Manage] Exec failed.");
    return ok;
}

static void RefreshSettingsControls()
{
    if (!g_chkRunOnStartup)
        return;

    SendMessageW(g_chkRunOnStartup, BM_SETCHECK, IsRunOnStartupEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_chkAutoUpdate, BM_SETCHECK, g_manageAutoCheckForUpdates ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_chkEnableLogging, BM_SETCHECK, GetNovadeskSettingBool("disableLogging", false) ? BST_UNCHECKED : BST_CHECKED, 0);
    SendMessageW(g_chkEnableDebugging, BM_SETCHECK, GetNovadeskSettingBool("enableDebugging", false) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_chkSaveLogToFile, BM_SETCHECK, GetNovadeskSettingBool("saveLogToFile", false) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_chkUseHardwareAcceleration, BM_SETCHECK, GetNovadeskSettingBool("useHardwareAcceleration", false) ? BST_CHECKED : BST_UNCHECKED, 0);
}

static void CheckForUpdates(bool silent)
{
    if (g_updateDialogOpen)
    {
        return;
    }

    SetSettingsStatusText(L"Checking for updates...");
    std::wstring latest;
    std::wstring assetUrl;
    std::wstring assetName;
    if (!GetLatestGithubRelease(latest, assetUrl, assetName))
    {
        SetSettingsStatusText(L"Update check failed.");
        if (!silent)
        {
            ShowManageMessageBox(L"Unable to check latest version from GitHub.", L"Novadesk Update", MB_OK | MB_ICONWARNING);
        }
        return;
    }

    if (IsVersionNewer(latest, kCurrentVersion))
    {
        const std::wstring msg = L"New version available: " + latest + L" (current: " + std::wstring(kCurrentVersion) + L")";
        SetSettingsStatusText(msg);
        const bool shouldShowDialog = !silent || (g_manageAutoCheckForUpdates && latest != g_lastNotifiedUpdateVersion);
        if (shouldShowDialog)
        {
            g_updateDialogOpen = true;
            const int rc = ShowManageMessageBox(msg + L"\n\nDownload latest release to temp and run setup now?",
                                                L"Novadesk Update",
                                                MB_YESNO | MB_ICONINFORMATION);
            g_updateDialogOpen = false;
            g_lastNotifiedUpdateVersion = latest;
            if (rc == IDYES)
            {
                if (assetUrl.empty())
                {
                    ShowManageMessageBox(L"No downloadable asset found for the latest release.", L"Novadesk Update", MB_OK | MB_ICONWARNING);
                    return;
                }

                wchar_t tempPath[MAX_PATH + 1] = {};
                if (!GetTempPathW(MAX_PATH, tempPath))
                {
                    ShowManageMessageBox(L"Could not access temp folder.", L"Novadesk Update", MB_OK | MB_ICONWARNING);
                    return;
                }

                std::wstring fileName = assetName.empty() ? L"novadesk_latest_setup.exe" : assetName;
                std::wstring destination = JoinPath(tempPath, fileName);
                SetSettingsStatusText(L"Downloading update...");
                if (!DownloadFileToPath(assetUrl, destination))
                {
                    SetSettingsStatusText(L"Download failed.");
                    ShowManageMessageBox(L"Failed to download latest release.", L"Novadesk Update", MB_OK | MB_ICONWARNING);
                    return;
                }

                SetSettingsStatusText(L"Launching setup...");
                ShellExecuteW(nullptr, L"open", destination.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
    }
    else
    {
        const std::wstring msg = L"You are up to date (" + std::wstring(kCurrentVersion) + L").";
        SetSettingsStatusText(msg);
        if (!silent)
        {
            ShowManageMessageBox(msg, L"Novadesk Update", MB_OK | MB_ICONINFORMATION);
        }
    }
}

struct UpdateCheckParams
{
    bool silent;
};

static DWORD WINAPI CheckForUpdatesThreadProc(LPVOID lpParam)
{
    UpdateCheckParams *params = static_cast<UpdateCheckParams *>(lpParam);
    if (!params) return 0;
    
    bool silent = params->silent;
    delete params;

    CheckForUpdates(silent);

    g_isCheckingForUpdates = false;
    return 0;
}

static void StartAsyncUpdateCheck(bool silent)
{
    if (g_isCheckingForUpdates)
        return;

    g_isCheckingForUpdates = true;
    UpdateCheckParams *params = new UpdateCheckParams{silent};
    HANDLE hThread = CreateThread(nullptr, 0, CheckForUpdatesThreadProc, params, 0, nullptr);
    if (hThread)
    {
        CloseHandle(hThread);
    }
    else
    {
        delete params;
        g_isCheckingForUpdates = false;
    }
}

static void OnSettingsControlClicked(int controlId)
{
    const HWND ctrl = reinterpret_cast<HWND>(GetDlgItem(g_mainWindow, controlId));
    const bool checked = (SendMessageW(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED);
    switch (controlId)
    {
    case kControlIdRunOnStartup:
        if (!SetRunOnStartupEnabled(checked))
        {
            MessageBoxW(g_mainWindow, L"Failed to update startup entry.", L"Manage Novadesk", MB_OK | MB_ICONWARNING);
        }
        break;
    case kControlIdAutoCheckUpdates:
        g_manageAutoCheckForUpdates = checked;
        SaveManageSettings();
        ConfigureAutoUpdateTimer(g_mainWindow);
        if (checked)
        {
            CheckForUpdates(true);
        }
        break;
    case kControlIdEnableLogging:
        SetNovadeskSettingBool("disableLogging", !checked);
        ExecuteNovadeskCommandNoPath(checked ? L"--enable-logging" : L"--disable-logging");
        break;
    case kControlIdEnableDebugging:
        SetNovadeskSettingBool("enableDebugging", checked);
        ExecuteNovadeskCommandNoPath(checked ? L"--enable-debugging" : L"--disable-debugging");
        break;
    case kControlIdSaveLogToFile:
        SetNovadeskSettingBool("saveLogToFile", checked);
        ExecuteNovadeskCommandNoPath(checked ? L"--enable-save-log-to-file" : L"--disable-save-log-to-file");
        break;
    case kControlIdUseHardwareAcceleration:
    {
        const bool previous = GetNovadeskSettingBool("useHardwareAcceleration", false);
        SetNovadeskSettingBool("useHardwareAcceleration", checked);
        if (previous != checked)
        {
            PromptRestartForHardwareAcceleration();
        }
        break;
    }
    default:
        break;
    }

    RefreshSettingsControls();
}

static void ConfigureAutoUpdateTimer(HWND hWnd)
{
    if (!hWnd)
        return;

    KillTimer(hWnd, kAutoUpdateTimerId);
    if (g_manageAutoCheckForUpdates)
    {
        SetTimer(hWnd, kAutoUpdateTimerId, kAutoUpdateIntervalMs, nullptr);
    }
}

static void LayoutTabPages(const RECT &pageRect)
{
    const int pageWidth = pageRect.right - pageRect.left;
    const int pageHeight = pageRect.bottom - pageRect.top;

    if (g_list)
    {
        MoveWindow(g_list, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    }
    if (g_logsList)
    {
        MoveWindow(g_logsList, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    }
    if (g_addonsList)
    {
        MoveWindow(g_addonsList, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    }
    if (g_settingsPanel)
    {
        MoveWindow(g_settingsPanel, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    }
    if (g_aboutPanel)
    {
        MoveWindow(g_aboutPanel, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    }
    const int settingsLeft = pageRect.left + 16;
    const int settingsWidth = pageWidth - 32;
    if (g_settingsTitle)
        MoveWindow(g_settingsTitle, settingsLeft, pageRect.top + 12, settingsWidth, 24, TRUE);
    if (g_chkRunOnStartup)
        MoveWindow(g_chkRunOnStartup, settingsLeft, pageRect.top + 46, settingsWidth, 24, TRUE);
    if (g_btnCheckUpdates)
        MoveWindow(g_btnCheckUpdates, settingsLeft, pageRect.top + 78, 170, 28, TRUE);
    if (g_chkAutoUpdate)
        MoveWindow(g_chkAutoUpdate, settingsLeft + 186, pageRect.top + 80, settingsWidth - 186, 24, TRUE);
    if (g_chkEnableLogging)
        MoveWindow(g_chkEnableLogging, settingsLeft, pageRect.top + 118, settingsWidth, 24, TRUE);
    if (g_chkEnableDebugging)
        MoveWindow(g_chkEnableDebugging, settingsLeft, pageRect.top + 148, settingsWidth, 24, TRUE);
    if (g_chkSaveLogToFile)
        MoveWindow(g_chkSaveLogToFile, settingsLeft, pageRect.top + 178, settingsWidth, 24, TRUE);
    if (g_chkUseHardwareAcceleration)
        MoveWindow(g_chkUseHardwareAcceleration, settingsLeft, pageRect.top + 208, settingsWidth, 24, TRUE);

    const int margin = 14;
    const int logoTop = pageRect.top + margin;
    const int logoColLeft = pageRect.left + margin;
    const int logoColWidth = 150;
    const int logoSize = 92;
    const int logoLeft = logoColLeft + (logoColWidth - logoSize) / 2;
    const int infoLeft = logoColLeft + logoColWidth + 10;
    const int infoWidth = pageRect.right - infoLeft - margin;

    if (g_aboutLogo)
        MoveWindow(g_aboutLogo, logoLeft, logoTop, logoSize, logoSize, TRUE);
    if (g_aboutAppName)
        MoveWindow(g_aboutAppName, infoLeft, logoTop, infoWidth, 34, TRUE);
    if (g_aboutVersion)
        MoveWindow(g_aboutVersion, infoLeft, logoTop + 36, infoWidth, 24, TRUE);
    if (g_aboutCopyright)
        MoveWindow(g_aboutCopyright, infoLeft, logoTop + 62, infoWidth, 24, TRUE);
    if (g_aboutDivider)
        MoveWindow(g_aboutDivider, infoLeft, logoTop + 88, infoWidth, 2, TRUE);
    if (g_aboutHomeLabel)
        MoveWindow(g_aboutHomeLabel, infoLeft, logoTop + 102, 64, 24, TRUE);
    if (g_aboutHomeLink)
        MoveWindow(g_aboutHomeLink, infoLeft + 66, logoTop + 102, infoWidth - 66, 24, TRUE);
    if (g_aboutDocsLabel)
        MoveWindow(g_aboutDocsLabel, infoLeft, logoTop + 128, 64, 24, TRUE);
    if (g_aboutDocsLink)
        MoveWindow(g_aboutDocsLink, infoLeft + 66, logoTop + 128, infoWidth - 66, 24, TRUE);

    const int supportTop = logoTop + 162;
    const int supportHeight = 126;
    const int supportLeft = pageRect.left + margin;
    const int supportWidth = pageWidth - (margin * 2);
    const int supportButtonWidth = 230;
    const int supportButtonHeight = 34;
    const int supportContentLeft = supportLeft + 16;
    const int supportButtonLeft = supportLeft + supportWidth - supportButtonWidth - 16;
    const int supportTextWidth = supportButtonLeft - supportContentLeft - 16;
    if (g_aboutSupportGroup)
        MoveWindow(g_aboutSupportGroup, supportLeft, supportTop, supportWidth, supportHeight, TRUE);
    if (g_aboutSupportTitle)
        MoveWindow(g_aboutSupportTitle, supportContentLeft, supportTop + 20, supportTextWidth, 28, TRUE);
    if (g_aboutSupportText)
        MoveWindow(g_aboutSupportText, supportContentLeft, supportTop + 52, supportTextWidth, 62, TRUE);
    if (g_aboutDonateButton)
        MoveWindow(g_aboutDonateButton, supportButtonLeft, supportTop + 48, supportButtonWidth, supportButtonHeight, TRUE);

    if (g_aboutBottomNote)
        MoveWindow(g_aboutBottomNote, pageRect.left + 150, supportTop + supportHeight + 8, pageWidth - 300, 24, TRUE);
}

static void ApplyTabState()
{
    const bool widgetsTab = (g_activeTab == 0);
    const bool logsTab = (g_activeTab == 1);
    const bool addonsTab = (g_activeTab == 2);
    const bool settingsTab = (g_activeTab == 3);
    const bool aboutTab = (g_activeTab == 4);

    ShowWindow(g_list, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_logsList, logsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_addonsList, addonsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_settingsPanel, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutPanel, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_settingsTitle, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkRunOnStartup, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnCheckUpdates, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkAutoUpdate, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkEnableLogging, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkEnableDebugging, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkSaveLogToFile, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_chkUseHardwareAcceleration, settingsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutLogo, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutAppName, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutVersion, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutCopyright, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutDivider, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutHomeLabel, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutHomeLink, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutDocsLabel, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutDocsLink, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutSupportGroup, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutSupportTitle, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutSupportText, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutDonateButton, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_aboutBottomNote, aboutTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefreshList, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClearLogs, logsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnLoad, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefresh, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefreshAll, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClose, SW_SHOW);

    if (logsTab)
    {
        RefreshLogsView();
    }
    if (addonsTab)
    {
        RefreshAddonsView();
    }
    if (settingsTab)
    {
        RefreshSettingsControls();
    }
    UpdateButtonState();
}

static void OnToggleLoadSelected()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        LogLine(L"[Manage] Toggle load ignored: no selection.");
        return;
    }

    const bool isLoaded = g_widgets[idx].loaded;
    bool ok = false;
    if (isLoaded)
    {
        LogLine(L"[Manage] Unload: " + g_widgets[idx].scriptPath);
        ok = ExecuteNovadeskCommand(L"--unload", g_widgets[idx].scriptPath);
    }
    else
    {
        LogLine(L"[Manage] Load: " + g_widgets[idx].scriptPath);
        ok = ExecuteNovadeskCommand(L"--load", g_widgets[idx].scriptPath);
    }
    if (ok)
    {
        RememberLoadedScriptState(g_widgets[idx].scriptPath, !isLoaded);
    }
    else
    {
        ShowManageMessageBox(L"Failed to send command to Novadesk.", L"Manage Novadesk", MB_OK | MB_ICONWARNING);
    }
    RefreshListView();
    UpdateButtonState();
}

static void OnRefreshSelected()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        LogLine(L"[Manage] Refresh ignored: no selection.");
        return;
    }
    LogLine(L"[Manage] Refresh: " + g_widgets[idx].scriptPath);
    ExecuteNovadeskCommand(L"--refresh", g_widgets[idx].scriptPath);
    RefreshListView();
    UpdateButtonState();
}

static void OnRefreshAll()
{
    LogLine(L"[Manage] Refresh All");
    ExecuteNovadeskCommandNoPath(L"--refresh-all");
    RefreshListView();
    UpdateButtonState();
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_manageCloseMessage != 0 && message == g_manageCloseMessage)
    {
        RequestAppExit(hWnd);
        return 0;
    }

    switch (message)
    {
    case WM_CREATE:
    {
        LogLine(L"[Manage] Window created.");
        g_mainWindow = hWnd;
        g_pendingLogLine.clear();
        SendMessageW(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_windowIconLarge));
        SendMessageW(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_windowIconSmall));
        RECT rc{};
        GetClientRect(hWnd, &rc);

        LoadManageSettings();
        StartNovadesk();
        AddTrayIcon(hWnd);
        InitGdiPlus();

        g_buttonFont = CreateFontW(-12, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_aboutTitleFont = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
        g_aboutSectionFont = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");
        g_aboutBodyFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Tahoma");

        g_tab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                10, 10, rc.right - 20, rc.bottom - 60,
                                hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdTabs)), GetModuleHandleW(nullptr), nullptr);
        TCITEMW tabItem{};
        tabItem.mask = TCIF_TEXT;
        tabItem.pszText = const_cast<wchar_t *>(L"Widgets");
        TabCtrl_InsertItem(g_tab, 0, &tabItem);
        tabItem.pszText = const_cast<wchar_t *>(L"Logs");
        TabCtrl_InsertItem(g_tab, 1, &tabItem);
        tabItem.pszText = const_cast<wchar_t *>(L"Addons");
        TabCtrl_InsertItem(g_tab, 2, &tabItem);
        tabItem.pszText = const_cast<wchar_t *>(L"Settings");
        TabCtrl_InsertItem(g_tab, 3, &tabItem);
        tabItem.pszText = const_cast<wchar_t *>(L"About");
        TabCtrl_InsertItem(g_tab, 4, &tabItem);

        RECT tabRectOnParent{};
        GetWindowRect(g_tab, &tabRectOnParent);
        MapWindowPoints(HWND_DESKTOP, hWnd, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
        RECT pageRect{};
        GetClientRect(g_tab, &pageRect);
        TabCtrl_AdjustRect(g_tab, FALSE, &pageRect);
        OffsetRect(&pageRect, tabRectOnParent.left, tabRectOnParent.top);

        g_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                 pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                 hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_logsList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                     WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_HSCROLL,
                                     pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                     hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_addonsList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                       WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                       pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                       hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_settingsPanel = CreateWindowExW(0, L"STATIC", L"",
                                          WS_CHILD,
                                          pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                          hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutPanel = CreateWindowExW(0, L"STATIC", L"",
                                       WS_CHILD,
                                       pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                       hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_settingsTitle = CreateWindowExW(0, L"STATIC", L"Manage Settings",
                                          WS_CHILD | SS_LEFT,
                                          pageRect.left + 16, pageRect.top + 12, pageRect.right - pageRect.left - 32, 24,
                                          hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_chkRunOnStartup = CreateWindowExW(0, L"BUTTON", L"Run On Startup",
                                            WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                            pageRect.left + 16, pageRect.top + 46, 240, 24,
                                            hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRunOnStartup)), GetModuleHandleW(nullptr), nullptr);
        g_btnCheckUpdates = CreateWindowExW(0, L"BUTTON", L"Check for Update",
                                            WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                            pageRect.left + 16, pageRect.top + 78, 160, 28,
                                            hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdCheckUpdates)), GetModuleHandleW(nullptr), nullptr);
        g_chkAutoUpdate = CreateWindowExW(0, L"BUTTON", L"Automatically Check For the Update",
                                          WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                          pageRect.left + 190, pageRect.top + 80, 300, 24,
                                          hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAutoCheckUpdates)), GetModuleHandleW(nullptr), nullptr);
        g_chkEnableLogging = CreateWindowExW(0, L"BUTTON", L"Enable Logging",
                                             WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                             pageRect.left + 16, pageRect.top + 118, 220, 24,
                                             hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdEnableLogging)), GetModuleHandleW(nullptr), nullptr);
        g_chkEnableDebugging = CreateWindowExW(0, L"BUTTON", L"Enable Debugging",
                                               WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                               pageRect.left + 16, pageRect.top + 148, 220, 24,
                                               hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdEnableDebugging)), GetModuleHandleW(nullptr), nullptr);
        g_chkSaveLogToFile = CreateWindowExW(0, L"BUTTON", L"Save Logs to File",
                                             WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                             pageRect.left + 16, pageRect.top + 178, 260, 24,
                                             hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdSaveLogToFile)), GetModuleHandleW(nullptr), nullptr);
        g_chkUseHardwareAcceleration = CreateWindowExW(0, L"BUTTON", L"Use Hardware Acceleration",
                                                       WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                       pageRect.left + 16, pageRect.top + 208, 300, 24,
                                                       hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdUseHardwareAcceleration)), GetModuleHandleW(nullptr), nullptr);
        g_aboutLogo = CreateWindowExW(0, L"STATIC", L"",
                                      WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
                                      pageRect.left, pageRect.top, 90, 90,
                                      hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutAppName = CreateWindowExW(0, L"STATIC", L"Novadesk",
                                         WS_CHILD | SS_LEFT,
                                         pageRect.left + 120, pageRect.top, pageRect.right - pageRect.left - 140, 32,
                                         hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutVersion = CreateWindowExW(0, L"STATIC", L"Version 0.7.0.0 (Beta)",
                                         WS_CHILD | SS_LEFT,
                                         pageRect.left + 120, pageRect.top + 34, pageRect.right - pageRect.left - 140, 22,
                                         hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutCopyright = CreateWindowExW(0, L"STATIC", L"(c) 2026 OfficialNovadesk. All rights reserved.",
                                           WS_CHILD | SS_LEFT,
                                           pageRect.left + 120, pageRect.top + 58, pageRect.right - pageRect.left - 140, 22,
                                           hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutDivider = CreateWindowExW(0, L"STATIC", L"",
                                         WS_CHILD | SS_ETCHEDHORZ,
                                         pageRect.left + 120, pageRect.top + 84, pageRect.right - pageRect.left - 140, 2,
                                         hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutHomeLabel = CreateWindowExW(0, L"STATIC", L"Home:",
                                           WS_CHILD | SS_LEFT,
                                           pageRect.left + 120, pageRect.top + 94, 60, 22,
                                           hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutHomeLink = CreateWindowExW(0, WC_LINK, L"<a href=\"https://novadesk.pages.dev/\">https://novadesk.pages.dev/</a>",
                                          WS_CHILD | WS_TABSTOP,
                                          pageRect.left + 178, pageRect.top + 94, pageRect.right - pageRect.left - 200, 22,
                                          hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutHome)), GetModuleHandleW(nullptr), nullptr);
        g_aboutDocsLabel = CreateWindowExW(0, L"STATIC", L"Docs:",
                                           WS_CHILD | SS_LEFT,
                                           pageRect.left + 120, pageRect.top + 118, 60, 22,
                                           hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutDocsLink = CreateWindowExW(0, WC_LINK, L"<a href=\"https://novadesk-docs.pages.dev/\">https://novadesk-docs.pages.dev/</a>",
                                          WS_CHILD | WS_TABSTOP,
                                          pageRect.left + 178, pageRect.top + 118, pageRect.right - pageRect.left - 200, 22,
                                          hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutDocs)), GetModuleHandleW(nullptr), nullptr);
        g_aboutSupportGroup = CreateWindowExW(0, L"BUTTON", L"Support Section",
                                              WS_CHILD | BS_GROUPBOX,
                                              pageRect.left + 8, pageRect.top + 146, pageRect.right - pageRect.left - 16, 104,
                                              hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutSupportTitle = CreateWindowExW(0, L"STATIC", L"Support && Contribution",
                                              WS_CHILD | SS_LEFT,
                                              pageRect.left + 22, pageRect.top + 168, pageRect.right - pageRect.left - 210, 26,
                                              hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutSupportText = CreateWindowExW(0, L"STATIC",
                                             L"We appreciate your support. If you find Novadesk helpful,\r\n"
                                             L"consider contributing to its ongoing development.",
                                             WS_CHILD | SS_LEFT,
                                             pageRect.left + 22, pageRect.top + 194, pageRect.right - pageRect.left - 220, 44,
                                             hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_aboutDonateButton = CreateWindowExW(0, L"BUTTON", L"Support Project via Donation",
                                              WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                              pageRect.right - 190, pageRect.top + 188, 176, 32,
                                              hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutDonate)), GetModuleHandleW(nullptr), nullptr);
        g_aboutBottomNote = CreateWindowExW(0, L"STATIC", L"Development depends on user support.",
                                            WS_CHILD | SS_CENTER,
                                            pageRect.left + 120, pageRect.top + 262, pageRect.right - pageRect.left - 240, 24,
                                            hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);

        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.pszText = const_cast<wchar_t *>(L"Widget");
        col.cx = 160;
        ListView_InsertColumn(g_list, 0, &col);

        col.pszText = const_cast<wchar_t *>(L"Script Path");
        col.cx = 360;
        ListView_InsertColumn(g_list, 1, &col);

        col.pszText = const_cast<wchar_t *>(L"Status");
        col.cx = 120;
        ListView_InsertColumn(g_list, 2, &col);

        ListView_SetExtendedListViewStyle(g_logsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW lcol{};
        lcol.mask = LVCF_TEXT | LVCF_WIDTH;
        lcol.pszText = const_cast<wchar_t *>(L"Time");
        lcol.cx = 190;
        ListView_InsertColumn(g_logsList, 0, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Source");
        lcol.cx = 120;
        ListView_InsertColumn(g_logsList, 1, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Level");
        lcol.cx = 80;
        ListView_InsertColumn(g_logsList, 2, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Message");
        lcol.cx = g_logsMessageColumnWidth;
        ListView_InsertColumn(g_logsList, 3, &lcol);

        ListView_SetExtendedListViewStyle(g_addonsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW acol{};
        acol.mask = LVCF_TEXT | LVCF_WIDTH;
        acol.pszText = const_cast<wchar_t *>(L"Addon Name");
        acol.cx = 190;
        ListView_InsertColumn(g_addonsList, 0, &acol);
        acol.pszText = const_cast<wchar_t *>(L"Author");
        acol.cx = 180;
        ListView_InsertColumn(g_addonsList, 1, &acol);
        acol.pszText = const_cast<wchar_t *>(L"Version");
        acol.cx = 120;
        ListView_InsertColumn(g_addonsList, 2, &acol);
        acol.pszText = const_cast<wchar_t *>(L"Copyright");
        acol.cx = 420;
        ListView_InsertColumn(g_addonsList, 3, &acol);
        UpdateAboutLogo();

        g_btnRefreshList = CreateWindowW(L"BUTTON", L"Refresh List", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         10, rc.bottom - 40, 110, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshList)), GetModuleHandleW(nullptr), nullptr);
        g_btnClearLogs = CreateWindowW(L"BUTTON", L"Clear Logs", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                       10, rc.bottom - 40, 110, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdClearLogs)), GetModuleHandleW(nullptr), nullptr);
        g_btnLoad = CreateWindowW(L"BUTTON", L"Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  130, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdLoad)), GetModuleHandleW(nullptr), nullptr);
        g_btnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     220, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefresh)), GetModuleHandleW(nullptr), nullptr);
        g_btnRefreshAll = CreateWindowW(L"BUTTON", L"Refresh All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        310, rc.bottom - 40, 100, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshAll)), GetModuleHandleW(nullptr), nullptr);
        g_btnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   rc.right - 90, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdClose)), GetModuleHandleW(nullptr), nullptr);

        if (g_buttonFont)
        {
            SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefreshList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnClearLogs, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnLoad, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefresh, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefreshAll, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnClose, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_logsList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_addonsList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_settingsPanel, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_settingsTitle, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
            SendMessageW(g_chkRunOnStartup, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnCheckUpdates, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_chkAutoUpdate, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_chkEnableLogging, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_chkEnableDebugging, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_chkSaveLogToFile, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_chkUseHardwareAcceleration, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_aboutAppName, WM_SETFONT, (WPARAM)g_aboutTitleFont, TRUE);
            SendMessageW(g_aboutVersion, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutCopyright, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutHomeLabel, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutHomeLink, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutDocsLabel, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutDocsLink, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutSupportGroup, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutSupportTitle, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
            SendMessageW(g_aboutSupportText, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutDonateButton, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
            SendMessageW(g_aboutBottomNote, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
        }

        LayoutTabPages(pageRect);
        RefreshSettingsControls();
        ConfigureAutoUpdateTimer(hWnd);
        if (g_manageAutoCheckForUpdates)
        {
            StartAsyncUpdateCheck(true);
        }
        RefreshListView();
        RefreshAddonsView();
        ApplyTabState();
        SetTimer(hWnd, kStartupSyncTimerId, kStartupSyncDelayMs, nullptr);
        SetTimer(hWnd, kLogsRefreshTimerId, 700, nullptr);
        UpdateButtonState();
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
        mmi->ptMinTrackSize.x = kWindowWidth;
        mmi->ptMinTrackSize.y = kWindowHeight;
        mmi->ptMaxTrackSize.x = kWindowWidth;
        mmi->ptMaxTrackSize.y = kWindowHeight;
        return 0;
    }
    case WM_SIZE:
    {
        RECT rc{};
        GetClientRect(hWnd, &rc);
        if (g_tab)
        {
            MoveWindow(g_tab, 10, 10, rc.right - 20, rc.bottom - 60, TRUE);
            RECT tabRectOnParent{};
            GetWindowRect(g_tab, &tabRectOnParent);
            MapWindowPoints(HWND_DESKTOP, hWnd, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
            RECT pageRect{};
            GetClientRect(g_tab, &pageRect);
            TabCtrl_AdjustRect(g_tab, FALSE, &pageRect);
            OffsetRect(&pageRect, tabRectOnParent.left, tabRectOnParent.top);
            LayoutTabPages(pageRect);
        }
        if (g_btnRefreshList)
        {
            MoveWindow(g_btnRefreshList, 10, rc.bottom - 40, 110, 28, TRUE);
            MoveWindow(g_btnClearLogs, 10, rc.bottom - 40, 110, 28, TRUE);
            MoveWindow(g_btnLoad, 130, rc.bottom - 40, 80, 28, TRUE);
            MoveWindow(g_btnRefresh, 220, rc.bottom - 40, 80, 28, TRUE);
            MoveWindow(g_btnRefreshAll, 310, rc.bottom - 40, 100, 28, TRUE);
            MoveWindow(g_btnClose, rc.right - 90, rc.bottom - 40, 80, 28, TRUE);
        }
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case kControlIdRefreshList:
            RefreshListView();
            break;
        case kControlIdClearLogs:
            OnClearLogs();
            break;
        case kControlIdLoad:
            OnToggleLoadSelected();
            break;
        case kControlIdRefresh:
            OnRefreshSelected();
            break;
        case kControlIdRefreshAll:
            OnRefreshAll();
            break;
        case kControlIdClose:
            ShowWindow(hWnd, SW_HIDE);
            break;
        case kControlIdRunOnStartup:
        case kControlIdAutoCheckUpdates:
        case kControlIdEnableLogging:
        case kControlIdEnableDebugging:
        case kControlIdSaveLogToFile:
        case kControlIdUseHardwareAcceleration:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                OnSettingsControlClicked(LOWORD(wParam));
            }
            break;
        case kControlIdCheckUpdates:
            StartAsyncUpdateCheck(false);
            break;
        case kControlIdAboutDonate:
            OpenUrl(L"https://www.patreon.com/cw/officialnovadesk");
            break;
        case kTrayMenuShowHideId:
            if (IsWindowVisible(hWnd))
            {
                ShowWindow(hWnd, SW_HIDE);
            }
            else
            {
                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
            }
            break;
        case kTrayMenuHomeWebsiteId:
            OpenUrl(L"https://novadesk.pages.dev/");
            break;
        case kTrayMenuHomeDonateId:
            OpenUrl(L"https://www.patreon.com/cw/officialnovadesk");
            break;
        case kTrayMenuDocsMainId:
            OpenUrl(L"https://novadesk-docs.pages.dev/");
            break;
        case kTrayMenuDocsApiId:
            OpenUrl(L"https://novadesk-docs.pages.dev/api/logging.html");
            break;
        case kTrayMenuSettingsOpenId:
            if (g_tab)
            {
                g_activeTab = 3;
                TabCtrl_SetCurSel(g_tab, g_activeTab);
                ApplyTabState();
            }
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
            break;
        case kTrayMenuSettingsCheckUpdatesId:
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
            g_activeTab = 3;
            if (g_tab)
            {
                TabCtrl_SetCurSel(g_tab, g_activeTab);
                ApplyTabState();
            }
            StartAsyncUpdateCheck(false);
            break;
        case kTrayMenuExitId:
            RequestAppExit(hWnd);
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
    {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr && hdr->hwndFrom == g_list && hdr->code == LVN_ITEMCHANGED)
        {
            UpdateButtonState();
        }
        else if (hdr && hdr->hwndFrom == g_tab && hdr->code == TCN_SELCHANGE)
        {
            g_activeTab = TabCtrl_GetCurSel(g_tab);
            ApplyTabState();
        }
        else if (hdr && (hdr->hwndFrom == g_aboutHomeLink || hdr->hwndFrom == g_aboutDocsLink) &&
                 (hdr->code == NM_CLICK || hdr->code == NM_RETURN))
        {
            auto *link = reinterpret_cast<PNMLINK>(lParam);
            if (link)
            {
                OpenUrl(link->item.szUrl);
            }
        }
        break;
    }
    case WM_CONTEXTMENU:
        if (reinterpret_cast<HWND>(wParam) == g_logsList)
        {
            ShowLogsContextMenu(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == kLogsRefreshTimerId && g_activeTab == 1)
        {
            RefreshLogsView();
        }
        else if (wParam == kAutoUpdateTimerId && g_manageAutoCheckForUpdates)
        {
            CheckForUpdates(true);
        }
        else if (wParam == kStartupSyncTimerId)
        {
            KillTimer(hWnd, kStartupSyncTimerId);
            ApplySavedLoadedScripts();
            RefreshListView();
            UpdateButtonState();
        }
        return 0;
    case kLogAppendMessage:
    {
        auto *chunk = reinterpret_cast<std::wstring *>(lParam);
        if (!chunk)
            return 0;
        ProcessLogChunk(*chunk);
        delete chunk;
        if (g_activeTab == 1)
        {
            RefreshLogsView();
        }
        return 0;
    }
    case WM_CLOSE:
        if (g_forceExitOnClose)
        {
            DestroyWindow(hWnd);
        }
        else
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        return 0;
    case WM_APP + 1:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
        {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        else if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hWnd);
        }
        return 0;
    case WM_DESTROY:
        g_mainWindow = nullptr;
        KillTimer(hWnd, kLogsRefreshTimerId);
        KillTimer(hWnd, kAutoUpdateTimerId);
        KillTimer(hWnd, kStartupSyncTimerId);
        if (g_windowIconLarge)
        {
            DestroyIcon(g_windowIconLarge);
            g_windowIconLarge = nullptr;
        }
        if (g_windowIconSmall)
        {
            DestroyIcon(g_windowIconSmall);
            g_windowIconSmall = nullptr;
        }
        if (g_buttonFont)
        {
            DeleteObject(g_buttonFont);
            g_buttonFont = nullptr;
        }
        if (g_aboutTitleFont)
        {
            DeleteObject(g_aboutTitleFont);
            g_aboutTitleFont = nullptr;
        }
        if (g_aboutSectionFont)
        {
            DeleteObject(g_aboutSectionFont);
            g_aboutSectionFont = nullptr;
        }
        if (g_aboutBodyFont)
        {
            DeleteObject(g_aboutBodyFont);
            g_aboutBodyFont = nullptr;
        }
        if (g_aboutLogoBitmap)
        {
            DeleteObject(g_aboutLogoBitmap);
            g_aboutLogoBitmap = nullptr;
        }
        ShutdownGdiPlus();
        if (g_trayMenu)
        {
            DestroyMenu(g_trayMenu);
            g_trayMenu = nullptr;
        }
        RemoveTrayIcon();
        StopNovadesk();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE,
                     _In_ LPSTR,
                     _In_ int nCmdShow)
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&icc);

    const wchar_t *className = kManageWindowClassName;
    g_manageCloseMessage = RegisterWindowMessageW(kManageCloseMessageName);
    HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, L"Global\\NovadeskManageWindowSingleton");
    if (instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND existing = FindWindowW(className, nullptr);
        if (existing)
        {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        CloseHandle(instanceMutex);
        return 0;
    }

    LoadWindowIcons(hInstance);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_windowIconLarge;
    wc.hIconSm = g_windowIconSmall;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT wr{0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRectEx(&wr, style, FALSE, 0);
    const int windowWidth = wr.right - wr.left;
    const int windowHeight = wr.bottom - wr.top;

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int x = workArea.left + ((workArea.right - workArea.left) - windowWidth) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - windowHeight) / 2;

    HWND hWnd = CreateWindowExW(0, className, L"Manage Novadesk",
                                style,
                                x, y,
                                windowWidth, windowHeight,
                                nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (instanceMutex)
    {
        CloseHandle(instanceMutex);
    }
    return (int)msg.wParam;
}
