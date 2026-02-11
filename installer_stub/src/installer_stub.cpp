#include <windows.h>
#include <winreg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <atlbase.h>
#include <atlcomcli.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <tlhelp32.h>
#include "..\..\Library\json\json.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;
using ATL::CComPtr;

static const std::string INSTALLER_MAGIC = "NWSFX1";

#pragma pack(push, 1)
struct InstallerFooter {
    char magic[8];
    uint64_t payloadSize;
    uint64_t manifestSize;
};
#pragma pack(pop)

static_assert(sizeof(InstallerFooter) == 24, "InstallerFooter size mismatch");

std::wstring ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring ExpandEnv(const std::wstring& value) {
    if (value.empty()) return L"";
    DWORD needed = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (needed == 0) return value;
    std::wstring buffer(needed, L'\0');
    DWORD written = ExpandEnvironmentStringsW(value.c_str(), buffer.data(), needed);
    if (written == 0) return value;
    buffer.resize(written - 1);
    return buffer;
}

bool ReadInstallerFooter(const fs::path& exePath, InstallerFooter& footer, uint64_t& footerOffset) {
    std::ifstream in(exePath, std::ios::binary | std::ios::ate);
    if (!in) return false;
    std::streamoff size = in.tellg();
    if (size < (std::streamoff)sizeof(InstallerFooter)) return false;
    footerOffset = static_cast<uint64_t>(size) - sizeof(InstallerFooter);
    in.seekg(static_cast<std::streamoff>(footerOffset));
    in.read(reinterpret_cast<char*>(&footer), sizeof(footer));
    if (!in) return false;
    return std::memcmp(footer.magic, INSTALLER_MAGIC.c_str(), INSTALLER_MAGIC.size()) == 0;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

bool RelaunchAsAdmin(const std::wstring& arguments) {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exePathBuf;
    sei.lpParameters = arguments.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        return false;
    }
    if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }
    return true;
}

bool CreateShortcut(const std::wstring& targetPath, const std::wstring& shortcutPath, const std::wstring& workingDir, const std::wstring& description) {
    CComPtr<IShellLinkW> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (FAILED(hr)) return false;

    shellLink->SetPath(targetPath.c_str());
    shellLink->SetWorkingDirectory(workingDir.c_str());
    shellLink->SetDescription(description.c_str());
    shellLink->SetIconLocation(targetPath.c_str(), 0);

    CComPtr<IPersistFile> persistFile;
    hr = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
    if (FAILED(hr)) return false;

    return SUCCEEDED(persistFile->Save(shortcutPath.c_str(), TRUE));
}

bool ExtractEmbeddedStub(const fs::path& outPath) {
    HRSRC hRes = FindResourceW(nullptr, L"INSTALLER_STUB", RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hData = LoadResource(nullptr, hRes);
    if (!hData) return false;
    DWORD size = SizeofResource(nullptr, hRes);
    void* data = LockResource(hData);
    if (!data || size == 0) return false;

    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return true;
}

bool IsProcessRunning(const std::wstring& exeName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exeName.c_str()) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return false;
}

bool IsAppInstalled(const std::wstring& appName) {
    std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appName;
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

namespace {
    HWND g_hwnd = nullptr;
    HWND g_title = nullptr;
    HWND g_status = nullptr;
    HWND g_progress = nullptr;
    HWND g_repairButton = nullptr;
    HWND g_uninstallButton = nullptr;
    HWND g_closeButton = nullptr;
    HFONT g_titleFont = nullptr;
    HFONT g_textFont = nullptr;
    std::atomic<uint64_t> g_done{0};
    std::atomic<uint64_t> g_total{1};
    std::atomic<bool> g_finished{false};
    std::atomic<bool> g_uninstall{false};
    std::atomic<bool> g_forceRepair{false};
    std::atomic<bool> g_showActions{false};
    std::atomic<bool> g_allowClose{false};
    std::atomic<int> g_action{0}; // 0 none, 1 repair, 2 uninstall
    std::mutex g_statusMutex;
    std::mutex g_actionMutex;
    std::condition_variable g_actionCv;
    std::wstring g_statusText = L"Preparing...";
    std::wstring g_appName = L"";
    std::wstring g_appNameUi = L"Installer";
    std::wstring g_launchExe;
    std::wstring g_launchDir;
    bool g_launchAfterInstall = false;
    void UpdateStatus(const std::wstring& text) {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        g_statusText = text;
    }

    void SetCloseEnabled(HWND hwnd, bool enabled) {
        HMENU hMenu = GetSystemMenu(hwnd, FALSE);
        if (!hMenu) return;
        EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
        DrawMenuBar(hwnd);
    }

    void RefreshUi() {
        if (!g_hwnd) return;
        uint64_t total = g_total.load();
        uint64_t done = g_done.load();
        if (total == 0) total = 1;
        int percent = static_cast<int>((done * 100) / total);
        SendMessageW(g_progress, PBM_SETPOS, percent, 0);
        std::lock_guard<std::mutex> lock(g_statusMutex);
        SetWindowTextW(g_status, g_statusText.c_str());
    }
}

std::wstring LoadAppNameFromManifest() {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

    InstallerFooter footer{};
    uint64_t footerOffset = 0;
    if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
        return L"";
    }

    uint64_t exeSize = footerOffset + sizeof(InstallerFooter);
    uint64_t payloadSize = footer.payloadSize;
    uint64_t manifestSize = footer.manifestSize;
    if (manifestSize == 0 || payloadSize == 0) {
        return L"";
    }

    uint64_t payloadStart = exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
    uint64_t manifestOffset = payloadStart + payloadSize;

    std::ifstream in(exePath, std::ios::binary);
    if (!in) return L"";
    in.seekg(static_cast<std::streamoff>(manifestOffset));
    std::string manifestJson(manifestSize, '\0');
    in.read(manifestJson.data(), static_cast<std::streamsize>(manifestSize));
    if (!in) return L"";

    nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
    if (manifest.is_discarded()) return L"";
    std::string appName = manifest.value("appName", "");
    return ToWString(appName);
}

bool InstallFromSelf(bool skipInstalledCheck) {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

    InstallerFooter footer{};
    uint64_t footerOffset = 0;
    if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
        return false;
    }

    if (!IsRunningAsAdmin()) {
        if (RelaunchAsAdmin(L"--install-ui")) {
            return true;
        }
        return true;
    }

    uint64_t exeSize = footerOffset + sizeof(InstallerFooter);
    uint64_t payloadSize = footer.payloadSize;
    uint64_t manifestSize = footer.manifestSize;
    if (manifestSize == 0 || payloadSize == 0) {
        return true;
    }

    uint64_t payloadStart = exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
    uint64_t manifestOffset = payloadStart + payloadSize;

    std::ifstream in(exePath, std::ios::binary);
    if (!in) {
        return true;
    }

    in.seekg(static_cast<std::streamoff>(manifestOffset));
    std::string manifestJson(manifestSize, '\0');
    in.read(manifestJson.data(), static_cast<std::streamsize>(manifestSize));
    if (!in) {
        return true;
    }

    nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
    if (manifest.is_discarded()) {
        return true;
    }

    std::string appName = manifest.value("appName", "");
    std::string version = manifest.value("version", "");
    std::string author = manifest.value("author", "");
    std::string description = manifest.value("description", "");
    std::string appExeRel = manifest.value("appExeRel", "");
    auto setup = manifest.value("setup", nlohmann::json::object());
    std::string installDirStr = setup.value("installDir", "");
    std::string startMenuFolder = setup.value("startMenuFolder", "");
    std::string setupName = setup.value("setupName", "");
    std::string setupIcon = setup.value("setupIcon", "");
    bool launchAfterInstall = setup.value("launchAfterInstall", false);
    if (appName.empty() || version.empty() || author.empty() || description.empty() || appExeRel.empty() ||
        installDirStr.empty() || startMenuFolder.empty() || setupName.empty() || setupIcon.empty()) {
        UpdateStatus(L"Missing required metadata.");
        return false;
    }

    bool createDesktopShortcut = setup.value("createDesktopShortcut", true);
    bool createStartupShortcut = setup.value("createStartupShortcut", false);
    bool runOnStartup = setup.value("runOnStartup", false);
    bool enableUninstall = setup.value("enableUninstall", true);

    std::wstring appNameW = ToWString(appName);
    if (!skipInstalledCheck && IsAppInstalled(appNameW)) {
        UpdateStatus(L"Already installed.");
        return false;
    }

    std::wstring installDirW = ExpandEnv(ToWString(installDirStr));

    fs::path installDirPath = fs::path(installDirW);
    fs::create_directories(installDirPath);

    // Persist manifest for uninstall
    try {
        std::ofstream mf(installDirPath / "install_manifest.json", std::ios::binary);
        if (mf) {
            mf.write(manifestJson.data(), static_cast<std::streamsize>(manifestJson.size()));
        }
    } catch (...) {
    }

    auto files = manifest.value("files", nlohmann::json::array());
    uint64_t totalBytes = 0;
    for (const auto& entry : files) {
        totalBytes += entry.value("size", 0ull);
    }

    UpdateStatus(L"Installing files...");

    uint64_t doneBytes = 0;
    for (const auto& entry : files) {
        std::string relPath = entry.value("path", "");
        uint64_t offset = entry.value("offset", 0ull);
        uint64_t size = entry.value("size", 0ull);
        if (relPath.empty() || size == 0) continue;

        fs::path outPath = installDirPath / fs::path(relPath);
        fs::create_directories(outPath.parent_path());

        std::ofstream out(outPath, std::ios::binary);
        if (!out) continue;

        in.seekg(static_cast<std::streamoff>(payloadStart + offset));
        uint64_t remaining = size;
        std::vector<char> buffer(1024 * 1024);
        while (remaining > 0) {
            uint64_t toRead = std::min<uint64_t>(remaining, buffer.size());
            in.read(buffer.data(), static_cast<std::streamsize>(toRead));
            std::streamsize got = in.gcount();
            if (got <= 0) break;
            out.write(buffer.data(), got);
            remaining -= static_cast<uint64_t>(got);
            doneBytes += static_cast<uint64_t>(got);
            g_done.store(doneBytes);
            g_total.store(totalBytes == 0 ? 1 : totalBytes);
        }
    }

    fs::path appExePath = installDirPath / fs::path(appExeRel);
    std::wstring appExeW = appExePath.wstring();
    std::wstring workingDirW = installDirPath.wstring();
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        UpdateStatus(L"Creating shortcuts...");
        if (!startMenuFolder.empty()) {
            PWSTR programsPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programsPath))) {
                fs::path startMenuDir = fs::path(programsPath) / fs::path(startMenuFolder);
                fs::create_directories(startMenuDir);
                fs::path shortcutPath = startMenuDir / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(programsPath);
            }
        }

        if (createDesktopShortcut) {
            PWSTR desktopPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath))) {
                fs::path shortcutPath = fs::path(desktopPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(desktopPath);
            }
        }

        if (createStartupShortcut || runOnStartup) {
            PWSTR startupPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startupPath))) {
                fs::path shortcutPath = fs::path(startupPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(startupPath);
            }
        }

        if (enableUninstall) {
            UpdateStatus(L"Registering uninstall...");
            fs::path uninstallPath = installDirPath / "Uninstall.exe";
            try {
                if (!ExtractEmbeddedStub(uninstallPath)) {
                    fs::copy_file(exePath, uninstallPath, fs::copy_options::overwrite_existing);
                }
            } catch (...) {
            }

            HKEY hKey = nullptr;
            std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                std::wstring displayName = appNameW;
                std::wstring publisher = ToWString(author);
                std::wstring displayVersion = ToWString(version);
                std::wstring displayDesc = ToWString(description);
                std::wstring uninstallCmd = L"\"" + uninstallPath.wstring() + L"\" --uninstall";
                std::wstring displayIcon = uninstallPath.wstring();
                RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayName.c_str()), static_cast<DWORD>((displayName.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"Publisher", 0, REG_SZ, reinterpret_cast<const BYTE*>(publisher.c_str()), static_cast<DWORD>((publisher.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayVersion.c_str()), static_cast<DWORD>((displayVersion.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"Description", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayDesc.c_str()), static_cast<DWORD>((displayDesc.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, reinterpret_cast<const BYTE*>(uninstallCmd.c_str()), static_cast<DWORD>((uninstallCmd.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, reinterpret_cast<const BYTE*>(installDirPath.wstring().c_str()), static_cast<DWORD>((installDirPath.wstring().size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayIcon.c_str()), static_cast<DWORD>((displayIcon.size() + 1) * sizeof(wchar_t)));
                RegCloseKey(hKey);
            }
        }

        CoUninitialize();
    }

    g_launchAfterInstall = launchAfterInstall;
    g_launchExe = appExeW;
    g_launchDir = workingDirW;

    UpdateStatus(L"Done");
    return true;
}

bool UninstallSelf() {
    if (!IsRunningAsAdmin()) {
        if (RelaunchAsAdmin(L"--uninstall")) {
            return true;
        }
        return true;
    }

    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

    fs::path installDirPath = exePath.parent_path();

    // Try to resolve install location (and uninstall string) from registry using app name if available
    std::wstring appNameHint = g_appName;
    std::wstring uninstallCmdFromReg;
    if (!appNameHint.empty()) {
        HKEY hInstallKey = nullptr;
        std::wstring installKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameHint;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, installKeyPath.c_str(), 0, KEY_READ, &hInstallKey) == ERROR_SUCCESS) {
            wchar_t installPathBuf[MAX_PATH];
            DWORD size = sizeof(installPathBuf);
            if (RegQueryValueExW(hInstallKey, L"InstallLocation", nullptr, nullptr, reinterpret_cast<LPBYTE>(installPathBuf), &size) == ERROR_SUCCESS) {
                if (installPathBuf[0] != L'\0') {
                    installDirPath = fs::path(installPathBuf);
                }
            }
            wchar_t uninstallBuf[2048];
            DWORD uninstallSize = sizeof(uninstallBuf);
            if (RegQueryValueExW(hInstallKey, L"UninstallString", nullptr, nullptr, reinterpret_cast<LPBYTE>(uninstallBuf), &uninstallSize) == ERROR_SUCCESS) {
                if (uninstallBuf[0] != L'\0') {
                    uninstallCmdFromReg = uninstallBuf;
                }
            }
            RegCloseKey(hInstallKey);
        }
    }

    std::string manifestJson;
    try {
        std::ifstream mf(installDirPath / "install_manifest.json", std::ios::binary);
        if (mf) {
            manifestJson.assign(std::istreambuf_iterator<char>(mf), std::istreambuf_iterator<char>());
        }
    } catch (...) {
    }
    if (manifestJson.empty()) {
        if (!uninstallCmdFromReg.empty()) {
            UpdateStatus(L"Launching uninstaller...");
            std::wstring cmd = L"/C \"\"";
            cmd += uninstallCmdFromReg;
            cmd += L"\"\"";
            SHELLEXECUTEINFOW sei = {};
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_NO_CONSOLE;
            sei.lpVerb = L"open";
            sei.lpFile = L"cmd.exe";
            sei.lpParameters = cmd.c_str();
            sei.nShow = SW_HIDE;
            ShellExecuteExW(&sei);
            return true;
        }
        return true;
    }

    nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
    if (manifest.is_discarded()) {
        return true;
    }

    std::string appName = manifest.value("appName", "");
    std::string version = manifest.value("version", "");
    std::string appExeRel = manifest.value("appExeRel", "");
    auto setup = manifest.value("setup", nlohmann::json::object());
    std::string installDir = setup.value("installDir", "");
    std::string startMenuFolder = setup.value("startMenuFolder", "");
    std::string setupName = setup.value("setupName", "");
    std::string setupIcon = setup.value("setupIcon", "");
    if (appName.empty() || version.empty() || appExeRel.empty() ||
        installDir.empty() || startMenuFolder.empty() || setupName.empty() || setupIcon.empty()) {
        UpdateStatus(L"Missing required metadata.");
        return false;
    }
    bool runOnStartup = setup.value("runOnStartup", false);
    bool createStartupShortcut = setup.value("createStartupShortcut", false);

    // If registry has a canonical install location, prefer it
    HKEY hInstallKey = nullptr;
    std::wstring installKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ToWString(appName);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, installKeyPath.c_str(), 0, KEY_READ, &hInstallKey) == ERROR_SUCCESS) {
        wchar_t installPathBuf[MAX_PATH];
        DWORD size = sizeof(installPathBuf);
        if (RegQueryValueExW(hInstallKey, L"InstallLocation", nullptr, nullptr, reinterpret_cast<LPBYTE>(installPathBuf), &size) == ERROR_SUCCESS) {
            if (installPathBuf[0] != L'\0') {
                installDirPath = fs::path(installPathBuf);
            }
        }
        RegCloseKey(hInstallKey);
    }

    std::wstring appExeNameW = fs::path(appExeRel).filename().wstring();
    if (IsProcessRunning(appExeNameW)) {
        UpdateStatus(L"Close the app before uninstalling.");
        return false;
    }

    // installDirPath already resolved above

    if (installDirPath.empty()) {
        return true;
    }

    UpdateStatus(L"Removing shortcuts...");
    std::wstring appNameW = ToWString(appName);

    PWSTR programsPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programsPath))) {
        fs::path startMenuDir = fs::path(programsPath) / ToWString(startMenuFolder);
        fs::remove_all(startMenuDir);
        CoTaskMemFree(programsPath);
    }

    PWSTR desktopPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath))) {
        fs::remove(fs::path(desktopPath) / (appNameW + L".lnk"));
        CoTaskMemFree(desktopPath);
    }

    if (createStartupShortcut || runOnStartup) {
        PWSTR startupPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startupPath))) {
            fs::remove(fs::path(startupPath) / (appNameW + L".lnk"));
            CoTaskMemFree(startupPath);
        }
    }

    UpdateStatus(L"Removing files...");
    bool removalQueued = false;
    try {
        std::error_code ec;
        fs::remove_all(installDirPath / "Widgets", ec);
        ec.clear();
        fs::remove_all(installDirPath, ec);
        if (ec || fs::exists(installDirPath)) {
            removalQueued = true;
        }
    } catch (...) {
        removalQueued = true;
    }

    if (removalQueued && !installDirPath.empty()) {
        UpdateStatus(L"Finishing removal...");
        std::wstring deleteCmd = L"/C rmdir /s /q \"" + installDirPath.wstring() + L"\"";
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NO_CONSOLE;
        sei.lpVerb = L"open";
        sei.lpFile = L"cmd.exe";
        sei.lpParameters = deleteCmd.c_str();
        sei.nShow = SW_HIDE;
        ShellExecuteExW(&sei);
    }

    // Remove Roaming AppData folder if present
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath))) {
        try {
            fs::remove_all(fs::path(appDataPath) / appNameW);
        } catch (...) {
        }
    }

    // Self-delete only if we are running from Uninstall.exe (not setup.exe)
    if (_wcsicmp(exePath.filename().c_str(), L"Uninstall.exe") == 0) {
        std::wstring deleteCmd = L"/C ping 127.0.0.1 -n 3 > nul & del /f /q \"" + exePath.wstring() + L"\" & rmdir /s /q \"" + installDirPath.wstring() + L"\"";
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NO_CONSOLE;
        sei.lpVerb = L"open";
        sei.lpFile = L"cmd.exe";
        sei.lpParameters = deleteCmd.c_str();
        sei.nShow = SW_HIDE;
        ShellExecuteExW(&sei);
    }

    HKEY hUninstall = nullptr;
    std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_WRITE, &hUninstall) == ERROR_SUCCESS) {
        RegDeleteTreeW(hUninstall, appNameW.c_str());
        RegCloseKey(hUninstall);
    }

    UpdateStatus(L"Done");
    return true;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TIMER:
            RefreshUi();
            if (g_showActions.load()) {
                ShowWindow(g_repairButton, SW_SHOW);
                ShowWindow(g_uninstallButton, SW_SHOW);
                EnableWindow(g_repairButton, TRUE);
                EnableWindow(g_uninstallButton, TRUE);
                if (!g_finished.load()) {
                    g_allowClose.store(true);
                    SetCloseEnabled(hwnd, true);
                }
            }
            if (g_finished.load()) {
                g_allowClose.store(true);
                SetCloseEnabled(hwnd, true);
                ShowWindow(g_closeButton, SW_SHOW);
                EnableWindow(g_repairButton, FALSE);
                EnableWindow(g_uninstallButton, FALSE);
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1001) {
                g_action.store(1);
                g_showActions.store(false);
                g_allowClose.store(false);
                SetCloseEnabled(hwnd, false);
                EnableWindow(g_repairButton, FALSE);
                EnableWindow(g_uninstallButton, FALSE);
                g_actionCv.notify_one();
                return 0;
            }
            if (LOWORD(wParam) == 1002) {
                g_action.store(2);
                g_showActions.store(false);
                g_allowClose.store(false);
                SetCloseEnabled(hwnd, false);
                EnableWindow(g_repairButton, FALSE);
                EnableWindow(g_uninstallButton, FALSE);
                g_actionCv.notify_one();
                return 0;
            }
            if (LOWORD(wParam) == 1003) {
                if (g_finished.load() && !g_uninstall.load() && g_launchAfterInstall && !g_launchExe.empty()) {
                    ShellExecuteW(nullptr, L"open", g_launchExe.c_str(), nullptr, g_launchDir.c_str(), SW_SHOWNORMAL);
                }
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            if (g_allowClose.load()) {
                DestroyWindow(hwnd);
                return 0;
            }
            // Ignore close while installing
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--uninstall") == 0) {
            g_uninstall.store(true);
            break;
        }
        if (wcscmp(argv[i], L"--repair") == 0) {
            g_forceRepair.store(true);
            break;
        }
    }
    if (argv) LocalFree(argv);

    if (!IsRunningAsAdmin()) {
        if (g_uninstall.load()) {
            RelaunchAsAdmin(L"--uninstall");
        } else if (g_forceRepair.load()) {
            RelaunchAsAdmin(L"--repair");
        } else {
            RelaunchAsAdmin(L"");
        }
        return 0;
    }

    g_appName = LoadAppNameFromManifest();
    g_appNameUi = g_appName.empty() ? L"Installer" : g_appName;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    const wchar_t* className = L"NovadeskInstallerWindow";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    RegisterClassExW(&wc);

    int width = 520;
    int height = 220;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    std::wstring windowTitle = g_appNameUi + (g_uninstall.load() ? L" Uninstaller" : L" Installer");
    g_hwnd = CreateWindowExW(
        0, className, windowTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwnd) {
        return InstallFromSelf(false) ? 0 : 1;
    }

    HICON hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (hIconBig) {
        SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    }
    if (hIconSmall) {
        SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    } else if (hIconBig) {
        SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconBig);
    }

    std::wstring headerText = g_uninstall.load() ? (L"Uninstalling " + g_appNameUi) : (L"Installing " + g_appNameUi);
    g_title = CreateWindowW(L"STATIC", headerText.c_str(),
                            WS_CHILD | WS_VISIBLE,
                            24, 20, 460, 28,
                            g_hwnd, nullptr, hInstance, nullptr);
    g_titleFont = CreateFontW(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_textFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(g_title, WM_SETFONT, (WPARAM)g_titleFont, TRUE);

    g_status = CreateWindowW(L"STATIC", L"Preparing...",
                             WS_CHILD | WS_VISIBLE,
                             24, 60, 460, 20,
                             g_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_textFont, TRUE);

    g_progress = CreateWindowW(PROGRESS_CLASSW, nullptr,
                               WS_CHILD | WS_VISIBLE,
                               24, 92, 460, 22,
                               g_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(g_progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    g_repairButton = CreateWindowW(L"BUTTON", L"Repair",
                                   WS_CHILD | WS_VISIBLE,
                                   24, 130, 100, 28,
                                   g_hwnd, (HMENU)1001, hInstance, nullptr);
    g_uninstallButton = CreateWindowW(L"BUTTON", L"Uninstall",
                                      WS_CHILD | WS_VISIBLE,
                                      134, 130, 100, 28,
                                      g_hwnd, (HMENU)1002, hInstance, nullptr);
    g_closeButton = CreateWindowW(L"BUTTON", L"Close",
                                  WS_CHILD | WS_VISIBLE,
                                  384, 130, 100, 28,
                                  g_hwnd, (HMENU)1003, hInstance, nullptr);
    SendMessageW(g_repairButton, WM_SETFONT, (WPARAM)g_textFont, TRUE);
    SendMessageW(g_uninstallButton, WM_SETFONT, (WPARAM)g_textFont, TRUE);
    SendMessageW(g_closeButton, WM_SETFONT, (WPARAM)g_textFont, TRUE);
    ShowWindow(g_repairButton, SW_HIDE);
    ShowWindow(g_uninstallButton, SW_HIDE);
    ShowWindow(g_closeButton, SW_HIDE);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    SetCloseEnabled(g_hwnd, false);

    std::thread worker([&]() {
        if (g_uninstall.load()) {
            UpdateStatus(L"Removing files...");
            g_done.store(1);
            g_total.store(1);
            UninstallSelf();
            g_done.store(1);
            g_total.store(1);
            g_finished.store(true);
            return;
        }

        if (g_forceRepair.load()) {
            UpdateStatus(L"Repairing...");
            InstallFromSelf(true);
            g_finished.store(true);
            return;
        }

        // If already installed, show actions and wait for user choice
        if (!g_appName.empty() && IsAppInstalled(g_appName)) {
            UpdateStatus(L"Already installed. Choose Repair or Uninstall.");
            g_showActions.store(true);
            std::unique_lock<std::mutex> lock(g_actionMutex);
            g_actionCv.wait(lock, [] { return g_action.load() != 0; });
            if (g_action.load() == 2) {
                g_uninstall.store(true);
                if (g_title) {
                    std::wstring headerText = L"Uninstalling " + g_appNameUi;
                    SetWindowTextW(g_title, headerText.c_str());
                }
                UpdateStatus(L"Removing files...");
                g_done.store(1);
                g_total.store(1);
                UninstallSelf();
                g_done.store(1);
                g_total.store(1);
                g_finished.store(true);
                return;
            } else {
                UpdateStatus(L"Repairing...");
                InstallFromSelf(true);
                g_finished.store(true);
                return;
            }
        }

        InstallFromSelf(false);
        g_finished.store(true);
    });
    worker.detach();

    SetTimer(g_hwnd, 1, 100, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_titleFont) DeleteObject(g_titleFont);
    if (g_textFont) DeleteObject(g_textFont);
    return 0;
}
