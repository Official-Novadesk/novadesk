#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>

namespace novadesk::shared::system
{
    struct DisplayRect
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    struct DisplayMonitorInfo
    {
        int id = 0;
        bool active = false;
        std::wstring deviceName;
        std::wstring monitorName;
        DisplayRect work;
        DisplayRect screen;
    };

    struct DisplayMetrics
    {
        int virtualTop = 0;
        int virtualLeft = 0;
        int virtualHeight = 0;
        int virtualWidth = 0;
        int primaryIndex = 0;
        std::vector<DisplayMonitorInfo> monitors;
    };

    struct PowerStatus
    {
        int acline = 0;
        int status = 0;
        int status2 = 0;
        double lifetime = 0.0;
        int percent = 0;
        double mhz = 0.0;
        double hz = 0.0;
    };

    struct CpuStats
    {
        double usage = 0.0;
    };

    struct MemoryStats
    {
        double total = 0.0;
        double available = 0.0;
        double used = 0.0;
        int percent = 0;
    };

    struct NetworkStats
    {
        double netIn = 0.0;
        double netOut = 0.0;
        double totalIn = 0.0;
        double totalOut = 0.0;
    };


    struct DiskStats
    {
        double total = 0.0;
        double available = 0.0;
        double used = 0.0;
        int percent = 0;
    };

    struct DiskIoStats
    {
        double readSpeed = 0.0;  // bytes/sec
        double writeSpeed = 0.0; // bytes/sec
    };

    struct RecycleBinStats
    {
        double count = 0.0;
        double size = 0.0;
    };

    enum class RegistryValueType
    {
        None = 0,
        String,
        Number
    };

    struct RegistryValue
    {
        RegistryValueType type = RegistryValueType::None;
        std::wstring stringValue;
        double numberValue = 0.0;
    };

    bool ClipboardSetText(const std::wstring &text);
    bool ClipboardGetText(std::wstring &outText);

    bool SetWallpaper(const std::wstring &imagePath, const std::wstring &style = L"fill");
    bool GetCurrentWallpaperPath(std::wstring &outPath);

    bool GetPowerStatus(PowerStatus &outStatus);
    bool GetCpuStats(CpuStats &outStats);
    bool GetMemoryStats(MemoryStats &outStats);
    bool GetNetworkStats(NetworkStats &outStats);
    bool GetDiskStats(const std::wstring &path, DiskStats &outStats);
    bool GetDiskIoStats(DiskIoStats &outStats);
    bool GetRecycleBinStats(RecycleBinStats &outStats);
    bool OpenRecycleBin();
    bool EmptyRecycleBin(bool silent);
    std::string FormatCurrentTime(const std::string &format, const std::string &localeName);
    double CurrentUnixTimestamp();
    std::string FormatTimestamp(double timestamp, const std::string &format, const std::string &localeName);
    bool ParseTimestamp(const std::string &text, const std::string &format, const std::string &localeName, double &outTimestamp);
    bool IsDaylightSavingTimeNow();
    DisplayMetrics GetDisplayMetrics();

    bool AudioSetVolume(int volumePercent);
    int AudioGetVolume();
    bool AudioPlaySound(const std::wstring &path, bool loop);
    void AudioStopSound();
    bool JsonReadTextFile(const std::wstring &path, std::string &outText);
    bool JsonWriteTextFile(const std::wstring &path, const std::string &text);
    bool JsonMergePatchFile(const std::wstring &path, const std::string &patchText);
    std::wstring GetEnv(const std::wstring &name);
    std::vector<std::pair<std::wstring, std::wstring>> GetAllEnv();
    bool Execute(const std::wstring &target, const std::wstring &parameters = L"", const std::wstring &workingDir = L"", int show = SW_SHOWNORMAL);
    bool WebFetch(const std::wstring &url, std::string &outData);

    bool RegistryReadData(const std::wstring &fullPath, const std::wstring &valueName, RegistryValue &outValue);
    bool RegistryWriteString(const std::wstring &fullPath, const std::wstring &valueName, const std::wstring &value);
    bool RegistryWriteNumber(const std::wstring &fullPath, const std::wstring &valueName, double value);

    struct UptimeStats
    {
        double seconds = 0.0;
        int days = 0;
        int hours = 0;
        int minutes = 0;
        int secs = 0;
    };
    bool GetSystemUptime(UptimeStats &outStats);
    std::string FormatUptime(const UptimeStats &stats, const std::string &format);

    struct MessageBoxOptions
    {
        std::wstring title;
        std::wstring message;
        std::wstring type;    // "info" | "warning" | "error" | "question"
        std::wstring buttons; // "ok" | "ok-cancel" | "yes-no" | "yes-no-cancel" | "retry-cancel" | "abort-retry-ignore"
        HWND         parent = nullptr; // Optional owner window
    };

    // Returns the button label string: "ok", "cancel", "yes", "no", "retry", "abort", "ignore"
    std::string ShowMessageBox(const MessageBoxOptions &opts);

} // namespace novadesk::shared::system
