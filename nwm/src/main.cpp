#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <windows.h>
#include <winreg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <atlbase.h>
#include <atlcomcli.h>
#include "rescle.h"
#include "../../Library/json/json.hpp"

#pragma comment(lib, "version.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;
using ATL::CComPtr;

// Constants
const std::string NOVADESK_EXE = "Novadesk.exe";
const std::string WIDGETS_DIR = "Widgets";
const std::string INSTALLER_MAGIC = "NWSFX1";

struct SetupOptions {
    bool createDesktopShortcut = true;
    bool createStartupShortcut = false;
    bool runOnStartup = false;
    std::string installDir = "%ProgramFiles%\\Novadesk";
    std::string startMenuFolder = "Novadesk";
    std::string setupName = "setup";
    std::string setupIcon;
    bool enableUninstall = true;
    bool launchAfterInstall = false;
};

#pragma pack(push, 1)
struct InstallerFooter {
    char magic[8];
    uint64_t payloadSize;
    uint64_t manifestSize;
};
#pragma pack(pop)

static_assert(sizeof(InstallerFooter) == 24, "InstallerFooter size mismatch");

// Helper to get executable directory
fs::path GetExeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return fs::path(path).parent_path();
}

// Helper to execute a command and wait
bool ExecuteCommand(const std::string& command) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode == 0;
    }
    return false;
}

void PrintUsage() {
    std::cout << "Novadesk Widget Maker (nwm)" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  nwm init <widget-name>    - Create a new widget in current directory" << std::endl;
    std::cout << "  nwm run                   - Run widget in current directory" << std::endl;
    std::cout << "  nwm build                 - Build widget in current directory" << std::endl;
    std::cout << "  nwm -v, --version         - Show version information" << std::endl;
    std::cout << "  nwm -h, --help            - Show this help message" << std::endl;
}

// Helper for string conversion
std::string ToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string GetProductVersion() {
    wchar_t szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);

    DWORD dwHandle = 0;
    DWORD dwSize = GetFileVersionInfoSizeW(szExePath, &dwHandle);
    if (dwSize == 0) return "Unknown";

    std::vector<BYTE> data(dwSize);
    if (!GetFileVersionInfoW(szExePath, dwHandle, dwSize, data.data())) return "Unknown";

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate;

    if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
        return "Unknown";

    wchar_t szSubBlock[100];
    swprintf_s(szSubBlock, L"\\StringFileInfo\\%04x%04x\\ProductVersion",
               lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

    LPVOID lpBuffer;
    UINT dwLen;
    if (VerQueryValueW(data.data(), szSubBlock, &lpBuffer, &dwLen)) {
        std::wstring wversion((wchar_t*)lpBuffer);
        return ToString(wversion);
    }

    return "Unknown";
}

// Helper for string conversion
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

struct InstallerFileEntry {
    std::string relPath;
    uint64_t offset = 0;
    uint64_t size = 0;
};

bool InstallFromSelf() {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

    InstallerFooter footer{};
    uint64_t footerOffset = 0;
    if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
        return false;
    }

    FreeConsole();

    if (!IsRunningAsAdmin()) {
        if (RelaunchAsAdmin(L"--install")) {
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

    std::string appName = manifest.value("appName", "Novadesk");
    std::string appExeRel = manifest.value("appExeRel", appName + ".exe");
    auto setup = manifest.value("setup", nlohmann::json::object());

    SetupOptions options;
    options.createDesktopShortcut = setup.value("createDesktopShortcut", true);
    options.createStartupShortcut = setup.value("createStartupShortcut", false);
    options.runOnStartup = setup.value("runOnStartup", false);
    options.installDir = setup.value("installDir", options.installDir);
    options.startMenuFolder = setup.value("startMenuFolder", options.startMenuFolder);
    options.enableUninstall = setup.value("enableUninstall", true);

    std::wstring installDirW = ExpandEnv(ToWString(options.installDir));
    if (installDirW.empty()) {
        installDirW = ExpandEnv(ToWString("%ProgramFiles%\\Novadesk"));
    }

    fs::path installDir = fs::path(installDirW);
    fs::create_directories(installDir);

    auto files = manifest.value("files", nlohmann::json::array());
    for (const auto& entry : files) {
        InstallerFileEntry fileEntry;
        fileEntry.relPath = entry.value("path", "");
        fileEntry.offset = entry.value("offset", 0ull);
        fileEntry.size = entry.value("size", 0ull);
        if (fileEntry.relPath.empty() || fileEntry.size == 0) continue;

        fs::path outPath = installDir / fs::path(fileEntry.relPath);
        fs::create_directories(outPath.parent_path());

        std::ofstream out(outPath, std::ios::binary);
        if (!out) continue;

        in.seekg(static_cast<std::streamoff>(payloadStart + fileEntry.offset));
        uint64_t remaining = fileEntry.size;
        std::vector<char> buffer(1024 * 1024);
        while (remaining > 0) {
            uint64_t toRead = std::min<uint64_t>(remaining, buffer.size());
            in.read(buffer.data(), static_cast<std::streamsize>(toRead));
            std::streamsize got = in.gcount();
            if (got <= 0) break;
            out.write(buffer.data(), got);
            remaining -= static_cast<uint64_t>(got);
        }
    }

    fs::path appExePath = installDir / fs::path(appExeRel);
    std::wstring appExeW = appExePath.wstring();
    std::wstring workingDirW = installDir.wstring();
    std::wstring appNameW = ToWString(appName);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        if (!options.startMenuFolder.empty()) {
            PWSTR programsPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programsPath))) {
                fs::path startMenuDir = fs::path(programsPath) / fs::path(options.startMenuFolder);
                fs::create_directories(startMenuDir);
                fs::path shortcutPath = startMenuDir / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(programsPath);
            }
        }

        if (options.createDesktopShortcut) {
            PWSTR desktopPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath))) {
                fs::path shortcutPath = fs::path(desktopPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(desktopPath);
            }
        }

        if (options.createStartupShortcut || options.runOnStartup) {
            PWSTR startupPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startupPath))) {
                fs::path shortcutPath = fs::path(startupPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(startupPath);
            }
        }

        if (options.enableUninstall) {
            fs::path uninstallPath = installDir / "Uninstall.exe";
            try {
                fs::copy_file(exePath, uninstallPath, fs::copy_options::overwrite_existing);
            } catch (...) {
            }

            HKEY hKey = nullptr;
            std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                std::wstring displayName = appNameW;
                std::wstring uninstallCmd = L"\"" + uninstallPath.wstring() + L"\" --uninstall";
                RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayName.c_str()), static_cast<DWORD>((displayName.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, reinterpret_cast<const BYTE*>(uninstallCmd.c_str()), static_cast<DWORD>((uninstallCmd.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, reinterpret_cast<const BYTE*>(installDir.wstring().c_str()), static_cast<DWORD>((installDir.wstring().size() + 1) * sizeof(wchar_t)));
                RegCloseKey(hKey);
            }
        }

        CoUninitialize();
    }

    return true;
}

bool BuildInstallerSfx(const fs::path& distDir,
                       const fs::path& widgetPath,
                       const fs::path& stubExe,
                       const std::string& widgetRealName,
                       const std::string& version,
                       const std::string& author,
                       const std::string& description,
                       const SetupOptions& setupOptions) {
    std::string setupName = setupOptions.setupName.empty() ? "setup" : setupOptions.setupName;
    if (setupName.size() < 4 || setupName.substr(setupName.size() - 4) != ".exe") {
        setupName += ".exe";
    }
    fs::path installerOut = distDir / setupName;
    if (fs::exists(installerOut)) {
        fs::remove(installerOut);
    }

    // Clean up any stray stub artifacts in dist (we embed stub into setup.exe)
    fs::path distStub = distDir / "installer_stub.exe";
    if (fs::exists(distStub)) {
        fs::remove(distStub);
    }
    fs::path distTmpStubExe = distDir / "_installer_stub.tmp.exe";
    if (fs::exists(distTmpStubExe)) {
        fs::remove(distTmpStubExe);
    }
    fs::path distTmpStub = distDir / "_installer_stub.tmp";
    if (fs::exists(distTmpStub)) {
        fs::remove(distTmpStub);
    }

    // Create a temp copy of the stub so we never mutate the original build artifact
    fs::path tempStub = fs::temp_directory_path() / ("_installer_stub_" + setupName);
    if (tempStub.extension() != ".exe") {
        tempStub += ".exe";
    }
    struct TempStubCleanup {
        fs::path path;
        ~TempStubCleanup() {
            if (path.empty()) return;
            try {
                if (fs::exists(path)) {
                    fs::remove(path);
                }
            } catch (...) {
            }
        }
    } tempStubCleanup{tempStub};

    try {
        fs::copy_file(stubExe, tempStub, fs::copy_options::overwrite_existing);
    } catch (...) {
        std::cerr << "Error: Failed to copy installer stub to temp at " << tempStub.string() << std::endl;
        return false;
    }

    // Apply app icon to the temp stub before embedding
    if (!setupOptions.setupIcon.empty()) {
        fs::path iconPath = widgetPath / setupOptions.setupIcon;
        if (fs::exists(iconPath)) {
            rescle::ResourceUpdater stubUpdater;
            if (stubUpdater.Load(tempStub.c_str())) {
                stubUpdater.SetIcon(iconPath.c_str());
                stubUpdater.Commit();
            }
        }
    }

    {
        std::ifstream stubIn(tempStub, std::ios::binary);
        if (!stubIn) {
            std::cerr << "Error: Failed to read installer stub from " << stubExe.string() << std::endl;
            return false;
        }
        std::ofstream out(installerOut, std::ios::binary);
        if (!out) {
            std::cerr << "Error: Failed to create installer at " << installerOut.string() << std::endl;
            return false;
        }
        out << stubIn.rdbuf();
    }

    if (!setupOptions.setupIcon.empty()) {
        fs::path iconPath = widgetPath / setupOptions.setupIcon;
        if (fs::exists(iconPath)) {
            rescle::ResourceUpdater updater;
            if (updater.Load(installerOut.c_str())) {
                updater.SetIcon(iconPath.c_str());
                if (!updater.Commit()) {
                    std::cerr << "Error: Failed to set installer icon." << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: Failed to load installer for icon update." << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error: setupIcon not found at " << iconPath.string() << std::endl;
            return false;
        }
    }

    // Embed installer_stub.exe as RT_RCDATA so Uninstall.exe can be created from it
    try {
        std::ifstream stubIn(tempStub, std::ios::binary);
        if (!stubIn) {
            std::cerr << "Error: Failed to read installer stub for embedding." << std::endl;
            return false;
        }
        std::vector<char> stubBytes((std::istreambuf_iterator<char>(stubIn)), std::istreambuf_iterator<char>());
        if (stubBytes.empty()) {
            std::cerr << "Error: installer stub is empty." << std::endl;
            return false;
        }

        HANDLE hUpdate = BeginUpdateResourceW(installerOut.wstring().c_str(), FALSE);
        if (!hUpdate) {
            std::cerr << "Error: BeginUpdateResource failed." << std::endl;
            return false;
        }

        if (!UpdateResourceW(hUpdate, RT_RCDATA, L"INSTALLER_STUB", MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                             stubBytes.data(), static_cast<DWORD>(stubBytes.size()))) {
            std::cerr << "Error: UpdateResource failed." << std::endl;
            EndUpdateResourceW(hUpdate, TRUE);
            return false;
        }

        if (!EndUpdateResourceW(hUpdate, FALSE)) {
            std::cerr << "Error: EndUpdateResource failed." << std::endl;
            return false;
        }

    } catch (...) {
        std::cerr << "Error: Failed to embed installer stub resource." << std::endl;
        return false;
    }

    struct FileMeta {
        fs::path relPath;
        uint64_t size = 0;
        uint64_t offset = 0;
    };

    std::vector<FileMeta> files;
    uint64_t payloadSize = 0;
    for (const auto& entry : fs::recursive_directory_iterator(distDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path() == installerOut) continue;

        const std::string filename = entry.path().filename().string();
        if (filename == "installer_stub.exe" || filename.rfind("_installer_stub.tmp", 0) == 0) {
            continue;
        }

        FileMeta meta;
        meta.relPath = fs::relative(entry.path(), distDir);
        meta.size = static_cast<uint64_t>(fs::file_size(entry.path()));
        meta.offset = payloadSize;
        payloadSize += meta.size;
        files.push_back(meta);
    }

    nlohmann::json manifest;
    manifest["appName"] = widgetRealName;
    manifest["version"] = version;
    manifest["author"] = author;
    manifest["description"] = description;
    manifest["appExeRel"] = widgetRealName + ".exe";
    manifest["setup"] = {
        {"createDesktopShortcut", setupOptions.createDesktopShortcut},
        {"createStartupShortcut", setupOptions.createStartupShortcut},
        {"runOnStartup", setupOptions.runOnStartup},
        {"installDir", setupOptions.installDir},
        {"startMenuFolder", setupOptions.startMenuFolder},
        {"setupName", setupOptions.setupName},
        {"setupIcon", setupOptions.setupIcon},
        {"enableUninstall", setupOptions.enableUninstall},
        {"launchAfterInstall", setupOptions.launchAfterInstall}
    };

    nlohmann::json fileArray = nlohmann::json::array();
    for (const auto& file : files) {
        nlohmann::json entry;
        entry["path"] = file.relPath.generic_string();
        entry["offset"] = file.offset;
        entry["size"] = file.size;
        fileArray.push_back(entry);
    }
    manifest["files"] = fileArray;

    std::string manifestJson = manifest.dump();
    uint64_t manifestSize = static_cast<uint64_t>(manifestJson.size());

    InstallerFooter footer{};
    std::memset(&footer, 0, sizeof(footer));
    std::memcpy(footer.magic, INSTALLER_MAGIC.c_str(), INSTALLER_MAGIC.size());
    footer.payloadSize = payloadSize;
    footer.manifestSize = manifestSize;

    std::ofstream out(installerOut, std::ios::binary | std::ios::app);
    if (!out) {
        std::cerr << "Error: Failed to append installer payload at " << installerOut.string() << std::endl;
        return false;
    }

    std::vector<char> buffer(1024 * 1024);
    for (const auto& file : files) {
        fs::path filePath = distDir / file.relPath;
        std::ifstream in(filePath, std::ios::binary);
        if (!in) {
            std::cerr << "Warning: Skipping file " << filePath.string() << std::endl;
            continue;
        }
        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            std::streamsize got = in.gcount();
            if (got > 0) out.write(buffer.data(), got);
        }
    }

    out.write(manifestJson.data(), static_cast<std::streamsize>(manifestJson.size()));
    out.write(reinterpret_cast<const char*>(&footer), sizeof(footer));

    std::cout << "Installer created at " << installerOut << std::endl;
    return true;
}

// Helper to parse version string "1.0.0.0" into 4 shorts
bool ParseVersion(const std::string& version, unsigned short& v1, unsigned short& v2, unsigned short& v3, unsigned short& v4) {
    v1 = v2 = v3 = v4 = 0;
    std::vector<int> parts;
    std::string current;
    for (char c : version) {
        if (c == '.') {
            if (!current.empty()) parts.push_back(std::stoi(current));
            current.clear();
        } else if (isdigit(c)) {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(std::stoi(current));

    if (parts.size() >= 1) v1 = parts[0];
    if (parts.size() >= 2) v2 = parts[1];
    if (parts.size() >= 3) v3 = parts[2];
    if (parts.size() >= 4) v4 = parts[3];
    return parts.size() > 0;
}

bool InitWidget(const std::string& name) {
    if (name.empty()) {
        std::cerr << "Error: Widget name required for init" << std::endl;
        return false;
    }
    fs::path baseDir = fs::current_path() / name;
    if (fs::exists(baseDir)) {
        std::cerr << "Error: Widget directory already exists: " << baseDir << std::endl;
        return false;
    }

    try {
        fs::path exeDir = GetExeDir();
        fs::path templateDir = exeDir / "widget";

        // Check if template exists in exe dir or source dir (for dev)
        if (!fs::exists(templateDir)) {
            templateDir = exeDir.parent_path().parent_path().parent_path() / "nwm" / "widget";
        }

        if (fs::exists(templateDir)) {
            fs::copy(templateDir, baseDir, fs::copy_options::recursive);

            // Replace name in meta.json
            fs::path metaPath = baseDir / "meta.json";
            if (fs::exists(metaPath)) {
                std::ifstream inFile(metaPath);
                std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
                inFile.close();

                // Improved replacement logic: find "name" key and replace value between quotes
                size_t nameKeyPos = content.find("\"name\"");
                if (nameKeyPos != std::string::npos) {
                    size_t colonPos = content.find(":", nameKeyPos);
                    if (colonPos != std::string::npos) {
                        size_t firstQuote = content.find("\"", colonPos);
                        size_t secondQuote = content.find("\"", firstQuote + 1);
                        if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                            content.replace(firstQuote + 1, secondQuote - firstQuote - 1, name);
                        }
                    }
                }

                // Replace any remaining {NAME} placeholders
                const std::string placeholderUpper = "{NAME}";
                const std::string placeholderTitle = "{Name}";
                size_t pos = 0;
                while ((pos = content.find(placeholderUpper, pos)) != std::string::npos) {
                    content.replace(pos, placeholderUpper.size(), name);
                    pos += name.size();
                }
                pos = 0;
                while ((pos = content.find(placeholderTitle, pos)) != std::string::npos) {
                    content.replace(pos, placeholderTitle.size(), name);
                    pos += name.size();
                }

                std::ofstream outFile(metaPath);
                outFile << content;
            }

            std::cout << "Widget initialized from template." << std::endl;
        } else {
            std::cerr << "Error: Widget template folder not found at " << templateDir << std::endl;
            return false;
        }

        std::cout << "Widget '" << name << "' initialized successfully at " << baseDir << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool RunWidget() {
    fs::path widgetPath = fs::current_path();
    if (!fs::exists(widgetPath / "index.js")) {
        std::cerr << "Error: Could not find index.js in current directory." << std::endl;
        return false;
    }

    fs::path exeDir = GetExeDir();
    fs::path novadeskExe = exeDir.parent_path() / NOVADESK_EXE;
    if (!fs::exists(novadeskExe)) {
        // Fallback for dev environment
        novadeskExe = exeDir.parent_path().parent_path() / NOVADESK_EXE;
    }

    if (!fs::exists(novadeskExe)) {
        std::cerr << "Error: Novadesk.exe not found." << std::endl;
        return false;
    }

    fs::path scriptPath = widgetPath / "index.js";
    std::string command = "\"" + novadeskExe.string() + "\" \"" + scriptPath.string() + "\"";
    std::cout << "Running: " << command << std::endl;

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cout << "Waiting for process to exit..." << std::endl;
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    } else {
        std::cerr << "Error: Failed to launch Novadesk.exe" << std::endl;
        return false;
    }
}

bool BuildWidget() {
    fs::path widgetPath = fs::current_path();
    fs::path metaPath = widgetPath / "meta.json";
    if (!fs::exists(metaPath)) {
        std::cerr << "Error: meta.json not found in current directory." << std::endl;
        return false;
    }

    std::ifstream metaFile(metaPath);
    std::stringstream metaBuffer;
    metaBuffer << metaFile.rdbuf();

    nlohmann::json meta = nlohmann::json::parse(metaBuffer.str(), nullptr, false);
    if (meta.is_discarded()) {
        std::cerr << "Error: meta.json is not valid JSON." << std::endl;
        return false;
    }

    std::string widgetRealName = meta.value("name", "");
    std::string version = meta.value("version", "");
    std::string icon = meta.value("icon", "");
    std::string author = meta.value("author", "");
    std::string description = meta.value("description", "");

    auto setupJson = meta.value("setup", nlohmann::json::object());
    SetupOptions setupOptions;
    setupOptions.createDesktopShortcut = setupJson.value("createDesktopShortcut", setupOptions.createDesktopShortcut);
    setupOptions.createStartupShortcut = setupJson.value("createStartupShortcut", setupOptions.createStartupShortcut);
    setupOptions.runOnStartup = setupJson.value("runOnStartup", setupOptions.runOnStartup);
    setupOptions.installDir = setupJson.value("installDir", setupOptions.installDir);
    setupOptions.startMenuFolder = setupJson.value("startMenuFolder", setupOptions.startMenuFolder);
    setupOptions.setupName = setupJson.value("setupName", setupOptions.setupName);
    setupOptions.setupIcon = setupJson.value("setupIcon", setupOptions.setupIcon);
    setupOptions.enableUninstall = setupJson.value("enableUninstall", setupOptions.enableUninstall);
    setupOptions.launchAfterInstall = setupJson.value("launchAfterInstall", setupOptions.launchAfterInstall);

    bool missing = false;
    if (widgetRealName.empty()) { std::cerr << "Error: 'name' is missing in meta.json" << std::endl; missing = true; }
    if (version.empty()) { std::cerr << "Error: 'version' is missing in meta.json" << std::endl; missing = true; }
    if (icon.empty()) { std::cerr << "Error: 'icon' is missing in meta.json" << std::endl; missing = true; }
    if (author.empty()) { std::cerr << "Error: 'author' is missing in meta.json" << std::endl; missing = true; }
    if (description.empty()) { std::cerr << "Error: 'description' is missing in meta.json" << std::endl; missing = true; }
    if (setupOptions.installDir.empty()) { std::cerr << "Error: 'setup.installDir' is missing in meta.json" << std::endl; missing = true; }
    if (setupOptions.startMenuFolder.empty()) { std::cerr << "Error: 'setup.startMenuFolder' is missing in meta.json" << std::endl; missing = true; }
    if (setupOptions.setupName.empty()) { std::cerr << "Error: 'setup.setupName' is missing in meta.json" << std::endl; missing = true; }
    if (setupOptions.setupIcon.empty()) { std::cerr << "Error: 'setup.setupIcon' is missing in meta.json" << std::endl; missing = true; }
    
    if (missing) return false;

    try {
        fs::path distDir = widgetPath / "dist";
        if (fs::exists(distDir)) fs::remove_all(distDir);
        fs::create_directories(distDir);

        fs::path exeDir = GetExeDir();
        fs::path srcExe = exeDir.parent_path() / NOVADESK_EXE;
        if (!fs::exists(srcExe)) {
            srcExe = exeDir.parent_path().parent_path() / NOVADESK_EXE;
        }

        fs::path destExe = distDir / (widgetRealName + ".exe");
        fs::copy_file(srcExe, destExe, fs::copy_options::overwrite_existing);

        fs::path widgetsSubDir = distDir / "Widgets";
        fs::create_directories(widgetsSubDir);

        for (const auto& entry : fs::directory_iterator(widgetPath)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            if (filename == "dist") continue;

            if (fs::is_directory(path)) {
                fs::copy(path, widgetsSubDir / filename, fs::copy_options::recursive);
            } else if (filename != (widgetRealName + ".exe")) {
                fs::copy_file(path, widgetsSubDir / filename, fs::copy_options::overwrite_existing);
            }
        }

        std::cout << "Applying metadata via internal rescle..." << std::endl;
        rescle::ResourceUpdater updater;
        if (updater.Load(destExe.c_str())) {
            updater.SetVersionString(RU_VS_PRODUCT_NAME, ToWString(widgetRealName).c_str());
            updater.SetVersionString(RU_VS_COMPANY_NAME, ToWString(author).c_str());
            updater.SetVersionString(RU_VS_FILE_DESCRIPTION, ToWString(description).c_str());
            updater.SetVersionString(RU_VS_FILE_VERSION, ToWString(version).c_str());
            updater.SetVersionString(RU_VS_PRODUCT_VERSION, ToWString(version).c_str());

            unsigned short v1, v2, v3, v4;
            if (ParseVersion(version, v1, v2, v3, v4)) {
                updater.SetFileVersion(v1, v2, v3, v4);
                updater.SetProductVersion(v1, v2, v3, v4);
            }

            if (!icon.empty()) {
                fs::path iconPath = widgetPath / icon;
                if (fs::exists(iconPath)) {
                    updater.SetIcon(iconPath.c_str());
                }
            }

            if (!updater.Commit()) {
                std::cerr << "Error: Failed to commit metadata updates via rescle." << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error: Failed to load executable for metadata update." << std::endl;
            return false;
        }

        std::cout << "Successfully built widget in " << distDir << std::endl;

        fs::path stubExe = exeDir / "installer_stub.exe";
        if (!fs::exists(stubExe)) {
            fs::path fallbackStub = exeDir.parent_path() / "installer_stub.exe";
            if (fs::exists(fallbackStub)) {
                stubExe = fallbackStub;
            } else {
                std::cerr << "Warning: installer_stub.exe not found. Falling back to nwm.exe." << std::endl;
                stubExe = exeDir / "nwm.exe";
            }
        }

        if (!BuildInstallerSfx(distDir, widgetPath, stubExe, widgetRealName, version, author, description, setupOptions)) {
            std::cerr << "Error: Failed to build installer." << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Build Error: " << e.what() << std::endl;
        return false;
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        if (InstallFromSelf()) {
            return 0;
        }
        PrintUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "--install") {
        return InstallFromSelf() ? 0 : 1;
    } else if (command == "init") {
        std::string widgetName = (argc > 2) ? argv[2] : "";
        return InitWidget(widgetName) ? 0 : 1;
    } else if (command == "run") {
        return RunWidget() ? 0 : 1;
    } else if (command == "build") {
        return BuildWidget() ? 0 : 1;
    } else if (command == "-v" || command == "--version") {
        std::cout << "nwm version " << GetProductVersion() << std::endl;
        return 0;
    } else if (command == "-h" || command == "--help") {
        PrintUsage();
        return 0;
    } else {
        PrintUsage();
        return 1;
    }
}
