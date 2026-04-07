#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <functional>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "..\..\third_party\json\json.hpp"
#include "unzip.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

constexpr wchar_t kWindowClassName[] = L"NovadeskNdpkgInstaller";
constexpr wchar_t kWindowTitle[] = L"ndpkg Installer";
constexpr wchar_t kManageWindowClassName[] = L"NovadeskManagerWindow";
constexpr wchar_t kManageCloseMessageName[] = L"Novadesk.Manage.RequestClose";
constexpr char kNdpkgMagic[] = "NDPKG1";
constexpr COLORREF kWindowBgColor = RGB(240, 240, 240);

enum ControlId : int {
    IDC_WIDGET_TITLE = 1003,
    IDC_NAME = 1004,
    IDC_VERSION = 1005,
    IDC_AUTHOR = 1006,
    IDC_ADDONS = 1007,
    IDC_PREVIEW = 1008,
    IDC_INSTALL = 1009,
    IDC_CANCEL = 1010,
    IDC_PROGRESS = 1011
};

#pragma pack(push, 1)
struct NdpkgFooter {
    char magic[8];
    uint32_t version;
    uint32_t reserved;
};
#pragma pack(pop)

struct PackageInfo {
    std::string name;
    std::string version;
    std::string author;
    std::vector<std::string> addons;
    fs::path previewImagePath;
};

enum class AddonInstallStatus {
    Add,
    Replace,
    NewerVersionFound
};

struct AddonRow {
    std::string fileName;
    fs::path packagePath;
    AddonInstallStatus status = AddonInstallStatus::Add;
    bool checked = true;
};

struct AppState {
    HWND hwnd = nullptr;
    HWND hTitle = nullptr;
    HWND hName = nullptr;
    HWND hVersion = nullptr;
    HWND hAuthor = nullptr;
    HWND hAddons = nullptr;
    HWND hPreview = nullptr;
    HWND hInstall = nullptr;
    HWND hCancel = nullptr;
    HWND hProgress = nullptr;
    HFONT hTitleFont = nullptr;
    HFONT hUiFont = nullptr;
    HBITMAP hPreviewBitmap = nullptr;
    ULONG_PTR gdiplusToken = 0;

    bool packageLoaded = false;
    bool installing = false;
    fs::path selectedPackagePath;
    fs::path workingDir;
    fs::path extractDir;
    PackageInfo info;
    std::vector<AddonRow> addonRows;
};

std::wstring Utf8ToWide(const std::string& input) {
    if (input.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    std::wstring out(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, out.data(), size);
    return out;
}

std::wstring ToWide(const fs::path& p) {
    return p.wstring();
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return value;
}

std::wstring ToLowerWide(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(towlower(c));
    });
    return value;
}

void SetEditText(HWND hEdit, const std::wstring& value) {
    SetWindowTextW(hEdit, value.c_str());
}

uint64_t GetFileVersionQuad(const fs::path& filePath) {
    DWORD handle = 0;
    const std::wstring fileWide = filePath.wstring();
    DWORD size = GetFileVersionInfoSizeW(fileWide.c_str(), &handle);
    if (size == 0) return 0;

    std::vector<BYTE> versionData(size);
    if (!GetFileVersionInfoW(fileWide.c_str(), handle, size, versionData.data())) return 0;

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT fileInfoLen = 0;
    if (!VerQueryValueW(versionData.data(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen) || !fileInfo) {
        return 0;
    }

    const uint64_t ms = static_cast<uint64_t>(fileInfo->dwFileVersionMS);
    const uint64_t ls = static_cast<uint64_t>(fileInfo->dwFileVersionLS);
    return (ms << 32) | ls;
}

const wchar_t* AddonStatusText(AddonInstallStatus status) {
    switch (status) {
    case AddonInstallStatus::Add:
        return L"Add";
    case AddonInstallStatus::Replace:
        return L"Replace";
    case AddonInstallStatus::NewerVersionFound:
        return L"Newer Version Found";
    }
    return L"Add";
}

bool EnsureDirectory(const fs::path& path, std::wstring& errorOut) {
    std::error_code ec;
    if (fs::exists(path, ec)) return true;
    if (fs::create_directories(path, ec)) return true;
    errorOut = L"Failed to create directory: " + ToWide(path);
    return false;
}

bool CopyPayloadToTempZip(const fs::path& ndpkgPath, const fs::path& zipOutPath, std::wstring& errorOut) {
    std::ifstream in(ndpkgPath, std::ios::binary);
    if (!in) {
        errorOut = L"Failed to open package file.";
        return false;
    }

    in.seekg(0, std::ios::end);
    const auto fileSize = in.tellg();
    if (fileSize < static_cast<std::streamoff>(sizeof(NdpkgFooter))) {
        errorOut = L"Invalid package: file is too small.";
        return false;
    }

    in.seekg(fileSize - static_cast<std::streamoff>(sizeof(NdpkgFooter)));
    NdpkgFooter footer{};
    in.read(reinterpret_cast<char*>(&footer), sizeof(footer));
    if (!in) {
        errorOut = L"Invalid package: failed reading footer.";
        return false;
    }

    if (memcmp(footer.magic, kNdpkgMagic, strlen(kNdpkgMagic)) != 0 || footer.version != 1) {
        errorOut = L"Invalid package: missing or invalid NDPKG footer.";
        return false;
    }

    const std::streamoff payloadSize = fileSize - static_cast<std::streamoff>(sizeof(NdpkgFooter));
    in.clear();
    in.seekg(0, std::ios::beg);

    std::ofstream out(zipOutPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        errorOut = L"Failed to create temporary payload file.";
        return false;
    }

    constexpr size_t kBufferSize = 64 * 1024;
    std::vector<char> buffer(kBufferSize);
    std::streamoff remaining = payloadSize;
    while (remaining > 0) {
        const auto chunk = static_cast<std::streamsize>(std::min<std::streamoff>(remaining, static_cast<std::streamoff>(buffer.size())));
        in.read(buffer.data(), chunk);
        if (!in) {
            errorOut = L"Failed to read package payload.";
            return false;
        }
        out.write(buffer.data(), chunk);
        if (!out) {
            errorOut = L"Failed to write package payload.";
            return false;
        }
        remaining -= chunk;
    }

    return true;
}

bool ExtractZip(const fs::path& zipPath, const fs::path& outputDir, std::wstring& errorOut) {
    unzFile uf = unzOpen64(zipPath.string().c_str());
    if (!uf) {
        errorOut = L"Failed to open ZIP payload.";
        return false;
    }

    auto closeUnz = [&]() { unzClose(uf); };

    int rc = unzGoToFirstFile(uf);
    if (rc == UNZ_END_OF_LIST_OF_FILE) {
        closeUnz();
        return true;
    }
    if (rc != UNZ_OK) {
        closeUnz();
        errorOut = L"Failed to read ZIP entries.";
        return false;
    }

    do {
        unz_file_info64 fileInfo{};
        char rawName[1024]{};
        rc = unzGetCurrentFileInfo64(uf, &fileInfo, rawName, sizeof(rawName), nullptr, 0, nullptr, 0);
        if (rc != UNZ_OK) {
            closeUnz();
            errorOut = L"Failed to read ZIP file info.";
            return false;
        }

        std::string entryName(rawName);
        std::replace(entryName.begin(), entryName.end(), '\\', '/');
        fs::path targetPath = outputDir / fs::path(entryName);

        const bool isDir = !entryName.empty() && entryName.back() == '/';
        if (isDir) {
            std::error_code ec;
            fs::create_directories(targetPath, ec);
        } else {
            std::error_code ec;
            fs::create_directories(targetPath.parent_path(), ec);

            rc = unzOpenCurrentFile(uf);
            if (rc != UNZ_OK) {
                closeUnz();
                errorOut = L"Failed to open ZIP entry.";
                return false;
            }

            std::ofstream out(targetPath, std::ios::binary | std::ios::trunc);
            if (!out) {
                unzCloseCurrentFile(uf);
                closeUnz();
                errorOut = L"Failed to create extracted file.";
                return false;
            }

            std::vector<char> buffer(64 * 1024);
            while (true) {
                const int bytes = unzReadCurrentFile(uf, buffer.data(), static_cast<unsigned int>(buffer.size()));
                if (bytes < 0) {
                    unzCloseCurrentFile(uf);
                    closeUnz();
                    errorOut = L"Failed while extracting ZIP entry.";
                    return false;
                }
                if (bytes == 0) break;
                out.write(buffer.data(), bytes);
                if (!out) {
                    unzCloseCurrentFile(uf);
                    closeUnz();
                    errorOut = L"Failed while writing extracted file.";
                    return false;
                }
            }

            unzCloseCurrentFile(uf);
        }

        rc = unzGoToNextFile(uf);
    } while (rc == UNZ_OK);

    closeUnz();
    if (rc != UNZ_END_OF_LIST_OF_FILE) {
        errorOut = L"Failed while iterating ZIP entries.";
        return false;
    }
    return true;
}

std::vector<std::string> ParseAddons(const json& j) {
    std::vector<std::string> addons;
    if (!j.contains("addons") || !j["addons"].is_array()) return addons;
    for (const auto& item : j["addons"]) {
        if (item.is_string()) addons.push_back(item.get<std::string>());
    }
    return addons;
}

fs::path FindPreviewImage(const fs::path& extractDir) {
    const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tif", ".tiff" };
    for (const auto& ext : exts) {
        fs::path preferred = extractDir / ("preview" + ext);
        if (fs::exists(preferred) && fs::is_regular_file(preferred)) {
            return preferred;
        }
    }
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(extractDir, ec)) {
        if (ec || !entry.is_regular_file()) continue;
        const auto fileName = ToLower(entry.path().filename().string());
        if (fileName == "ndpkg.json") continue;
        const auto ext = ToLower(entry.path().extension().string());
        if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
            return entry.path();
        }
    }
    return {};
}

bool LoadPackageMetadata(const fs::path& extractDir, PackageInfo& outInfo, std::wstring& errorOut) {
    const fs::path metaPath = extractDir / "ndpkg.json";
    if (!fs::exists(metaPath)) {
        errorOut = L"Package metadata file ndpkg.json is missing.";
        return false;
    }

    std::ifstream in(metaPath, std::ios::binary);
    if (!in) {
        errorOut = L"Failed to open ndpkg.json.";
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (...) {
        errorOut = L"ndpkg.json is not valid JSON.";
        return false;
    }

    if (!j.contains("name") || !j["name"].is_string() ||
        !j.contains("version") || !j["version"].is_string() ||
        !j.contains("author") || !j["author"].is_string()) {
        errorOut = L"ndpkg.json is missing required fields.";
        return false;
    }

    outInfo.name = j["name"].get<std::string>();
    outInfo.version = j["version"].get<std::string>();
    outInfo.author = j["author"].get<std::string>();
    outInfo.addons = ParseAddons(j);
    outInfo.previewImagePath = FindPreviewImage(extractDir);
    return true;
}

void DeleteDirectoryIfExists(const fs::path& path) {
    std::error_code ec;
    if (fs::exists(path, ec)) fs::remove_all(path, ec);
}

HBITMAP LoadScaledPreviewBitmap(const fs::path& imagePath, HWND hPreview) {
    if (imagePath.empty() || !fs::exists(imagePath)) return nullptr;

    RECT rc{};
    GetClientRect(hPreview, &rc);
    const int targetW = std::max(1L, rc.right - rc.left);
    const int targetH = std::max(1L, rc.bottom - rc.top);

    Gdiplus::Bitmap source(imagePath.wstring().c_str());
    if (source.GetLastStatus() != Gdiplus::Ok) return nullptr;

    const UINT srcW = source.GetWidth();
    const UINT srcH = source.GetHeight();
    if (srcW == 0 || srcH == 0) return nullptr;

    const float sx = static_cast<float>(targetW) / static_cast<float>(srcW);
    const float sy = static_cast<float>(targetH) / static_cast<float>(srcH);
    const float scale = std::max(sx, sy);
    const int drawW = static_cast<int>(srcW * scale);
    const int drawH = static_cast<int>(srcH * scale);
    const int drawX = (targetW - drawW) / 2;
    const int drawY = (targetH - drawH) / 2;

    Gdiplus::Bitmap canvas(targetW, targetH, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&canvas);
    g.Clear(Gdiplus::Color(255, 240, 240, 240));
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.DrawImage(&source, Gdiplus::Rect(drawX, drawY, drawW, drawH));

    HBITMAP hBmp = nullptr;
    if (canvas.GetHBITMAP(Gdiplus::Color(0, 0, 0), &hBmp) != Gdiplus::Ok) {
        return nullptr;
    }
    return hBmp;
}

std::wstring GetDocumentsPathSuffix(const wchar_t* subdir) {
    PWSTR docs = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
        result = std::wstring(docs) + L"\\Novadesk\\" + subdir;
        CoTaskMemFree(docs);
    }
    return result;
}

bool CopyDirectoryRecursive(const fs::path& src,
                           const fs::path& dst,
                           std::wstring& errorOut,
                           const std::function<void()>& onFileCopied = nullptr) {
    std::error_code ec;
    if (!fs::exists(src, ec) || !fs::is_directory(src, ec)) {
        errorOut = L"Missing source directory: " + ToWide(src);
        return false;
    }

    if (!fs::exists(dst, ec) && !fs::create_directories(dst, ec)) {
        errorOut = L"Failed to create destination: " + ToWide(dst);
        return false;
    }

    for (const auto& entry : fs::recursive_directory_iterator(src, ec)) {
        if (ec) {
            errorOut = L"Failed while reading source directory.";
            return false;
        }
        const fs::path rel = fs::relative(entry.path(), src, ec);
        if (ec) {
            errorOut = L"Failed to resolve relative path.";
            return false;
        }

        const fs::path target = dst / rel;
        if (entry.is_directory()) {
            fs::create_directories(target, ec);
            if (ec) {
                errorOut = L"Failed to create target directory.";
                return false;
            }
        } else if (entry.is_regular_file()) {
            fs::create_directories(target.parent_path(), ec);
            if (ec) {
                errorOut = L"Failed to create parent directory for file copy.";
                return false;
            }
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                errorOut = L"Failed to copy file: " + ToWide(entry.path());
                return false;
            }
            if (onFileCopied) onFileCopied();
        }
    }

    return true;
}

bool ResolveInstallTargets(fs::path& widgetsOut, fs::path& addonsOut) {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    fs::path novadeskExe = exeDir / "Novadesk.exe";
    if (!fs::exists(novadeskExe)) {
        fs::path parentCandidate = exeDir.parent_path() / "Novadesk.exe";
        if (fs::exists(parentCandidate)) {
            novadeskExe = parentCandidate;
        } else {
            novadeskExe.clear();
        }
    }

    const bool portable = !novadeskExe.empty() && fs::exists(novadeskExe.parent_path() / "settings.json");
    if (portable) {
        widgetsOut = novadeskExe.parent_path() / "Widgets";
        addonsOut = novadeskExe.parent_path() / "Addons";
        return true;
    }

    const std::wstring docsWidgets = GetDocumentsPathSuffix(L"Widgets");
    const std::wstring docsAddons = GetDocumentsPathSuffix(L"Addons");
    if (docsWidgets.empty() || docsAddons.empty()) return false;
    widgetsOut = fs::path(docsWidgets);
    addonsOut = fs::path(docsAddons);
    return true;
}

std::vector<fs::path> GetPackageAddonFiles(const AppState* state) {
    std::vector<fs::path> out;
    if (!state) return out;

    const fs::path addonsDir = state->extractDir / "Addons";
    if (!fs::exists(addonsDir) || !fs::is_directory(addonsDir)) return out;

    std::unordered_map<std::string, fs::path> byName;
    std::unordered_map<std::string, fs::path> byStem;
    for (const auto& entry : fs::directory_iterator(addonsDir)) {
        if (!entry.is_regular_file()) continue;
        if (ToLower(entry.path().extension().string()) != ".dll") continue;
        byName[ToLower(entry.path().filename().string())] = entry.path();
        byStem[ToLower(entry.path().stem().string())] = entry.path();
    }

    std::unordered_set<std::string> used;
    for (const auto& listedAddon : state->info.addons) {
        std::string key = ToLower(listedAddon);
        std::string stemKey = key;
        if (key.size() > 4 && key.substr(key.size() - 4) == ".dll") {
            stemKey = key.substr(0, key.size() - 4);
        }
        fs::path resolved;
        auto itName = byName.find(key);
        if (itName != byName.end()) {
            resolved = itName->second;
        } else {
            auto itStem = byStem.find(stemKey);
            if (itStem != byStem.end()) resolved = itStem->second;
        }
        if (!resolved.empty()) {
            const std::string canonical = ToLower(resolved.filename().string());
            if (used.insert(canonical).second) out.push_back(resolved);
        }
    }

    for (const auto& [name, path] : byName) {
        if (used.insert(name).second) out.push_back(path);
    }

    return out;
}

void BuildAddonRows(AppState* state) {
    if (!state) return;
    state->addonRows.clear();

    fs::path widgetsTarget;
    fs::path installedAddonsTarget;
    const bool hasTargets = ResolveInstallTargets(widgetsTarget, installedAddonsTarget);
    const auto packageAddons = GetPackageAddonFiles(state);

    for (const auto& addonPath : packageAddons) {
        AddonRow row;
        row.fileName = addonPath.filename().string();
        row.packagePath = addonPath;
        row.status = AddonInstallStatus::Add;
        row.checked = true;

        if (hasTargets) {
            const fs::path installedFile = installedAddonsTarget / addonPath.filename();
            if (fs::exists(installedFile) && fs::is_regular_file(installedFile)) {
                const uint64_t packageVer = GetFileVersionQuad(addonPath);
                const uint64_t installedVer = GetFileVersionQuad(installedFile);
                if (packageVer != 0 && installedVer != 0 && installedVer > packageVer) {
                    row.status = AddonInstallStatus::NewerVersionFound;
                    row.checked = false;
                } else {
                    row.status = AddonInstallStatus::Replace;
                    row.checked = true;
                }
            }
        }

        state->addonRows.push_back(std::move(row));
    }
}

void PopulateAddonsListView(AppState* state) {
    if (!state || !state->hAddons) return;
    ListView_DeleteAllItems(state->hAddons);

    for (size_t i = 0; i < state->addonRows.size(); ++i) {
        const AddonRow& row = state->addonRows[i];
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        std::wstring name = Utf8ToWide(row.fileName);
        item.pszText = name.empty() ? const_cast<LPWSTR>(L"") : name.data();
        const int itemIndex = ListView_InsertItem(state->hAddons, &item);
        if (itemIndex >= 0) {
            const wchar_t* statusText = AddonStatusText(row.status);
            ListView_SetItemText(state->hAddons, itemIndex, 1, const_cast<LPWSTR>(statusText));
            ListView_SetCheckState(state->hAddons, itemIndex, row.checked ? TRUE : FALSE);
        }
    }
}

void SetCloseButtonEnabled(HWND hwnd, bool enabled) {
    HMENU sysMenu = GetSystemMenu(hwnd, FALSE);
    if (!sysMenu) return;
    EnableMenuItem(sysMenu, SC_CLOSE, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
    DrawMenuBar(hwnd);
}

void SetInstallProgress(AppState* state, int percent) {
    if (!state || !state->hProgress) return;
    const int clamped = std::max(0, std::min(100, percent));
    SendMessageW(state->hProgress, PBM_SETPOS, static_cast<WPARAM>(clamped), 0);
    UpdateWindow(state->hProgress);
}

void SetInstallingUiState(AppState* state, bool installing) {
    if (!state) return;
    state->installing = installing;
    if (state->hInstall) ShowWindow(state->hInstall, installing ? SW_HIDE : SW_SHOW);
    if (state->hCancel) ShowWindow(state->hCancel, installing ? SW_HIDE : SW_SHOW);
    if (state->hProgress) ShowWindow(state->hProgress, installing ? SW_SHOW : SW_HIDE);
    SetCloseButtonEnabled(state->hwnd, !installing);
}

bool RequestManageQuit() {
    HWND manageWindow = FindWindowW(kManageWindowClassName, nullptr);
    if (!manageWindow) return true;

    const UINT closeMessage = RegisterWindowMessageW(kManageCloseMessageName);
    if (closeMessage == 0) return false;

    DWORD_PTR result = 0;
    SendMessageTimeoutW(
        manageWindow,
        closeMessage,
        0,
        0,
        SMTO_ABORTIFHUNG,
        1500,
        &result);

    for (int i = 0; i < 80; ++i) {
        if (!FindWindowW(kManageWindowClassName, nullptr)) {
            return true;
        }
        Sleep(100);
    }
    return !FindWindowW(kManageWindowClassName, nullptr);
}

bool StartManageWindow() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path manageExe = exeDir / "manage_novadesk.exe";

    if (!fs::exists(manageExe)) {
        fs::path parentCandidate = exeDir.parent_path() / "manage_novadesk.exe";
        if (fs::exists(parentCandidate)) {
            manageExe = parentCandidate;
            exeDir = manageExe.parent_path();
        } else {
            return false;
        }
    }

    std::wstring commandLine = L"\"" + manageExe.wstring() + L"\"";
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL started = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        exeDir.wstring().c_str(),
        &si,
        &pi);
    if (!started) return false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

void ResetUiFromState(AppState* state) {
    SetEditText(state->hName, L"");
    SetEditText(state->hVersion, L"");
    SetEditText(state->hAuthor, L"");
    state->addonRows.clear();
    if (state->hAddons) {
        ListView_DeleteAllItems(state->hAddons);
    }
    SetWindowTextW(state->hTitle, L"Select a package to view details");
    EnableWindow(state->hInstall, FALSE);

    if (state->hPreviewBitmap) {
        DeleteObject(state->hPreviewBitmap);
        state->hPreviewBitmap = nullptr;
    }
    SendMessageW(state->hPreview, STM_SETIMAGE, IMAGE_BITMAP, 0);
}

void UpdateUiWithPackage(AppState* state) {
    SetWindowTextW(state->hTitle, Utf8ToWide(state->info.name).c_str());
    SetEditText(state->hName, Utf8ToWide(state->info.name));
    SetEditText(state->hVersion, Utf8ToWide(state->info.version));
    SetEditText(state->hAuthor, Utf8ToWide(state->info.author));

    BuildAddonRows(state);
    PopulateAddonsListView(state);

    if (state->hPreviewBitmap) {
        DeleteObject(state->hPreviewBitmap);
        state->hPreviewBitmap = nullptr;
    }

    state->hPreviewBitmap = LoadScaledPreviewBitmap(state->info.previewImagePath, state->hPreview);
    if (state->hPreviewBitmap) {
        SendMessageW(state->hPreview, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(state->hPreviewBitmap));
    } else {
        SendMessageW(state->hPreview, STM_SETIMAGE, IMAGE_BITMAP, 0);
    }

    EnableWindow(state->hInstall, TRUE);
}

bool LoadPackageFromPath(AppState* state, const fs::path& packagePath, std::wstring& errorOut) {
    state->packageLoaded = false;
    state->selectedPackagePath = packagePath;

    DeleteDirectoryIfExists(state->workingDir);

    const DWORD pid = GetCurrentProcessId();
    const DWORD tick = GetTickCount();
    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);
    state->workingDir = fs::path(tempDir) / (L"ndpkg_installer_" + std::to_wstring(pid) + L"_" + std::to_wstring(tick));
    state->extractDir = state->workingDir / "extract";
    const fs::path payloadZip = state->workingDir / "payload.zip";

    if (!EnsureDirectory(state->workingDir, errorOut)) return false;
    if (!EnsureDirectory(state->extractDir, errorOut)) return false;
    if (!CopyPayloadToTempZip(packagePath, payloadZip, errorOut)) return false;
    if (!ExtractZip(payloadZip, state->extractDir, errorOut)) return false;

    PackageInfo pkg{};
    if (!LoadPackageMetadata(state->extractDir, pkg, errorOut)) return false;
    if (!fs::exists(state->extractDir / "Widgets")) {
        errorOut = L"Invalid package: Widgets folder is missing.";
        return false;
    }

    state->info = std::move(pkg);
    state->packageLoaded = true;
    return true;
}

bool PromptForPackagePath(HWND owner, fs::path& outPath) {
    wchar_t filePath[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Novadesk Package (*.ndpkg)\0*.ndpkg\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"ndpkg";

    if (!GetOpenFileNameW(&ofn)) {
        return false;
    }
    outPath = fs::path(filePath);
    return true;
}

bool IsNdpkgPath(const fs::path& path) {
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return ext == L".ndpkg";
}

bool TryGetCliPackagePath(fs::path& outPath) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;

    bool found = false;
    for (int i = 1; i < argc; ++i) {
        fs::path candidate = fs::path(argv[i]);
        if (candidate.empty()) continue;
        if (!IsNdpkgPath(candidate)) continue;
        outPath = candidate;
        found = true;
        break;
    }

    LocalFree(argv);
    return found;
}

void HandleInstall(AppState* state) {
    if (!state->packageLoaded) return;
    if (state->installing) return;

    SetInstallingUiState(state, true);
    SetInstallProgress(state, 0);

    fs::path widgetsTarget;
    fs::path addonsTarget;
    if (!RequestManageQuit()) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, L"Failed to close running Manage Novadesk.", L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }
    SetInstallProgress(state, 20);

    if (!ResolveInstallTargets(widgetsTarget, addonsTarget)) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, L"Failed to resolve Novadesk install paths.", L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }

    std::wstring error;
    if (!EnsureDirectory(widgetsTarget, error)) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, error.c_str(), L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }
    SetInstallProgress(state, 30);
    if (!EnsureDirectory(addonsTarget, error)) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, error.c_str(), L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }
    SetInstallProgress(state, 35);

    const fs::path widgetsSource = state->extractDir / "Widgets";
    size_t widgetFileCount = 0;
    std::error_code countEc;
    for (const auto& entry : fs::recursive_directory_iterator(widgetsSource, countEc)) {
        if (countEc) break;
        if (entry.is_regular_file()) ++widgetFileCount;
    }
    size_t copiedWidgetFiles = 0;
    if (!CopyDirectoryRecursive(widgetsSource, widgetsTarget, error, [&]() {
        ++copiedWidgetFiles;
        const int widgetProgress = 35 + static_cast<int>((widgetFileCount == 0 ? 1.0 : (static_cast<double>(copiedWidgetFiles) / static_cast<double>(widgetFileCount))) * 45.0);
        SetInstallProgress(state, widgetProgress);
    })) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, error.c_str(), L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }
    SetInstallProgress(state, 80);

    std::vector<fs::path> addonFiles;
    for (size_t i = 0; i < state->addonRows.size(); ++i) {
        if (!state->hAddons || ListView_GetCheckState(state->hAddons, static_cast<int>(i))) {
            addonFiles.push_back(state->addonRows[i].packagePath);
        }
    }

    const size_t addonCount = addonFiles.size();
    size_t copiedAddons = 0;
    for (const auto& addonPath : addonFiles) {
        std::error_code ec;
        fs::copy_file(addonPath, addonsTarget / addonPath.filename(), fs::copy_options::overwrite_existing, ec);
        if (ec) {
            SetInstallingUiState(state, false);
            std::wstring fileErr = L"Failed to copy addon: " + ToWide(addonPath.filename());
            MessageBoxW(state->hwnd, fileErr.c_str(), L"Install Error", MB_ICONERROR | MB_OK);
            return;
        }
        ++copiedAddons;
        const int addonProgress = 80 + static_cast<int>((addonCount == 0 ? 1.0 : (static_cast<double>(copiedAddons) / static_cast<double>(addonCount))) * 15.0);
        SetInstallProgress(state, addonProgress);
    }
    SetInstallProgress(state, 95);

    if (!StartManageWindow()) {
        SetInstallingUiState(state, false);
        MessageBoxW(state->hwnd, L"Installation completed, but failed to restart Manage Novadesk.", L"Install Error", MB_ICONERROR | MB_OK);
        return;
    }

    SetInstallProgress(state, 100);
    DestroyWindow(state->hwnd);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* appState = reinterpret_cast<AppState*>(cs->lpCreateParams);
        appState->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(appState));
        state = appState;

        RECT rc{};
        GetClientRect(hwnd, &rc);

        state->hTitle = CreateWindowW(L"STATIC", L"Select a package to view details", WS_CHILD | WS_VISIBLE, 16, 10, 656, 34, hwnd, reinterpret_cast<HMENU>(IDC_WIDGET_TITLE), nullptr, nullptr);
        state->hTitleFont = CreateFontW(-26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        state->hUiFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessageW(state->hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(state->hTitleFont), TRUE);

        HWND hGroupDetails = CreateWindowW(L"BUTTON", L"Widget Details", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 16, 58, 300, 346, hwnd, nullptr, nullptr, nullptr);
        HWND hLblName = CreateWindowW(L"STATIC", L"Name", WS_CHILD | WS_VISIBLE, 30, 88, 100, 18, hwnd, nullptr, nullptr, nullptr);
        state->hName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY, 30, 108, 272, 24, hwnd, reinterpret_cast<HMENU>(IDC_NAME), nullptr, nullptr);
        HWND hLblVersion = CreateWindowW(L"STATIC", L"Version", WS_CHILD | WS_VISIBLE, 30, 142, 100, 18, hwnd, nullptr, nullptr, nullptr);
        state->hVersion = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY, 30, 162, 272, 24, hwnd, reinterpret_cast<HMENU>(IDC_VERSION), nullptr, nullptr);
        HWND hLblAuthor = CreateWindowW(L"STATIC", L"Author", WS_CHILD | WS_VISIBLE, 30, 196, 100, 18, hwnd, nullptr, nullptr, nullptr);
        state->hAuthor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY, 30, 216, 272, 24, hwnd, reinterpret_cast<HMENU>(IDC_AUTHOR), nullptr, nullptr);
        HWND hLblAddons = CreateWindowW(L"STATIC", L"Included Addons", WS_CHILD | WS_VISIBLE, 30, 250, 140, 18, hwnd, nullptr, nullptr, nullptr);
        state->hAddons = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
            30,
            270,
            272,
            124,
            hwnd,
            reinterpret_cast<HMENU>(IDC_ADDONS),
            nullptr,
            nullptr);
        ListView_SetExtendedListViewStyle(state->hAddons, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.pszText = const_cast<LPWSTR>(L"Addon");
        col.cx = 140;
        col.iSubItem = 0;
        ListView_InsertColumn(state->hAddons, 0, &col);
        col.pszText = const_cast<LPWSTR>(L"Status");
        col.cx = 120;
        col.iSubItem = 1;
        ListView_InsertColumn(state->hAddons, 1, &col);

        HWND hGroupVisuals = CreateWindowW(L"BUTTON", L"Widget Visuals", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 324, 58, 356, 346, hwnd, nullptr, nullptr, nullptr);
        state->hPreview = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE, 340, 86, 324, 266, hwnd, reinterpret_cast<HMENU>(IDC_PREVIEW), nullptr, nullptr);

        state->hInstall = CreateWindowW(L"BUTTON", L"Install", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 468, 364, 95, 30, hwnd, reinterpret_cast<HMENU>(IDC_INSTALL), nullptr, nullptr);
        state->hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 571, 364, 95, 30, hwnd, reinterpret_cast<HMENU>(IDC_CANCEL), nullptr, nullptr);
        state->hProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD, 468, 370, 198, 20, hwnd, reinterpret_cast<HMENU>(IDC_PROGRESS), nullptr, nullptr);
        SendMessageW(state->hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(state->hProgress, PBM_SETPOS, 0, 0);
        ShowWindow(state->hProgress, SW_HIDE);

        SendMessageW(hGroupDetails, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(hLblName, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hName, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(hLblVersion, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hVersion, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(hLblAuthor, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hAuthor, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(hLblAddons, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hAddons, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(hGroupVisuals, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hInstall, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);
        SendMessageW(state->hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(state->hUiFont), TRUE);

        EnableWindow(state->hInstall, FALSE);
        if (state->packageLoaded) {
            UpdateUiWithPackage(state);
        }
        return 0;
    }
    case WM_COMMAND:
        if (!state) return 0;
        switch (LOWORD(wParam)) {
        case IDC_INSTALL:
            HandleInstall(state);
            return 0;
        case IDC_CANCEL:
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        if (state && state->installing) {
            return 0;
        }
        break;
    case WM_DESTROY:
        if (state) {
            if (state->hPreviewBitmap) {
                DeleteObject(state->hPreviewBitmap);
                state->hPreviewBitmap = nullptr;
            }
            if (state->hTitleFont) {
                DeleteObject(state->hTitleFont);
                state->hTitleFont = nullptr;
            }
            if (state->hUiFont) {
                DeleteObject(state->hUiFont);
                state->hUiFont = nullptr;
            }
            DeleteDirectoryIfExists(state->workingDir);
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    AppState state;
    if (Gdiplus::GdiplusStartup(&state.gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok) {
        MessageBoxW(nullptr, L"Failed to initialize GDI+.", L"NDPKG Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = kWindowClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(kWindowBgColor);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_NDPKG_INSTALLER));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_NDPKG_INSTALLER));

    if (!RegisterClassExW(&wc)) {
        Gdiplus::GdiplusShutdown(state.gdiplusToken);
        MessageBoxW(nullptr, L"Failed to register window class.", L"NDPKG Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    fs::path cliPackage;
    if (TryGetCliPackagePath(cliPackage)) {
        std::wstring loadError;
        if (!LoadPackageFromPath(&state, cliPackage, loadError)) {
            Gdiplus::GdiplusShutdown(state.gdiplusToken);
            MessageBoxW(nullptr, loadError.c_str(), L"Failed to Load Package", MB_ICONERROR | MB_OK);
            return 1;
        }
    } else {
        while (true) {
            fs::path selectedPackage;
            if (!PromptForPackagePath(nullptr, selectedPackage)) {
                Gdiplus::GdiplusShutdown(state.gdiplusToken);
                return 0;
            }

            std::wstring loadError;
            if (LoadPackageFromPath(&state, selectedPackage, loadError)) {
                break;
            }

            MessageBoxW(nullptr, loadError.c_str(), L"Failed to Load Package", MB_ICONERROR | MB_OK);
        }
    }

    constexpr int kWindowWidth = 708;
    constexpr int kWindowHeight = 452;
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int windowX = workArea.left + ((workArea.right - workArea.left) - kWindowWidth) / 2;
    const int windowY = workArea.top + ((workArea.bottom - workArea.top) - kWindowHeight) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX,
        windowY,
        kWindowWidth,
        kWindowHeight,
        nullptr,
        nullptr,
        hInstance,
        &state
    );

    if (!hwnd) {
        Gdiplus::GdiplusShutdown(state.gdiplusToken);
        MessageBoxW(nullptr, L"Failed to create main window.", L"NDPKG Installer", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(state.gdiplusToken);
    return static_cast<int>(msg.wParam);
}
