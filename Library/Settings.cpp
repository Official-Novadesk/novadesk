/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Settings.h"
#include "Utils.h"
#include "Logging.h"
#include "ColorUtil.h"
#include "PathUtils.h"
#include "Novadesk.h"
#include <fstream>
#include <iomanip>

json Settings::s_Data;
bool Settings::s_Dirty = false;

void Settings::Initialize()
{
    Load();
}

std::wstring Settings::GetSettingsPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath = path;
    size_t lastBackslash = exePath.find_last_of(L"\\");
    return exePath.substr(0, lastBackslash + 1) + L"settings.json";
}

void Settings::Load()
{
    std::wstring path = GetSettingsPath();
    std::ifstream i(path);
    if (i.is_open())
    {
        // check for empty file
        if (i.peek() == std::ifstream::traits_type::eof()) {
            Logging::Log(LogLevel::Info, L"Settings file is empty, initializing defaults.");
            s_Data = json::object();
            s_Data["widgets"] = json::object();
            return;
        }

        try {
            i >> s_Data;
            Logging::Log(LogLevel::Info, L"Settings loaded from %s", path.c_str());
        } catch (json::parse_error& e) {
            Logging::Log(LogLevel::Error, L"Failed to parse settings.json: %S - Resetting to defaults", e.what());
            s_Data = json::object();
            s_Data["widgets"] = json::object();
        }
 
        ApplyGlobalSettings();
    }
    else
    {
        s_Data = json::object();
        s_Data["widgets"] = json::object();
        ApplyGlobalSettings(); // Apply defaults
    }
}

void Settings::ApplyGlobalSettings()
{
    // Default values if keys don't exist
    Logging::SetLogLevel(Settings::GetGlobalBool("enableDebugging", false) ? LogLevel::Debug : LogLevel::Info);
    
    bool disableLogging = Settings::GetGlobalBool("disableLogging", false);
    Logging::SetConsoleLogging(!disableLogging);
    
    if (disableLogging) {
        Logging::SetFileLogging(L"");
    } else {
        if (Settings::GetGlobalBool("saveLogToFile", false)) {
            std::wstring logPath = PathUtils::GetExeDir() + L"logs.log";
            Logging::SetFileLogging(logPath, false);
        } else {
            Logging::SetFileLogging(L"");
        }
    }

    if (Settings::GetGlobalBool("hideTrayIcon", false)) {
        ::HideTrayIconDynamic();
    } else {
        ::ShowTrayIconDynamic();
    }
}

void Settings::Save()
{
    if (!s_Dirty) return;
    
    std::wstring path = GetSettingsPath();
    std::ofstream o(path);
    if (o.is_open())
    {
        o << std::setw(4) << s_Data << std::endl;
        s_Dirty = false;
    }
}

void Settings::SaveWidget(const std::wstring& id, const WidgetOptions& options)
{
    if (id.empty()) return;
    
    std::string idStr = Utils::ToString(id);
    
    json widgetData;
    widgetData["x"] = options.x;
    widgetData["y"] = options.y;
    widgetData["windowopacity"] = options.windowOpacity;
    
    std::string zPosStr = "normal";
    switch(options.zPos) {
        case ZPOSITION_ONDESKTOP: zPosStr = "ondesktop"; break;
        case ZPOSITION_ONTOP: zPosStr = "ontop"; break;
        case ZPOSITION_ONBOTTOM: zPosStr = "onbottom"; break;
        case ZPOSITION_ONTOPMOST: zPosStr = "ontopmost"; break;
    }
    widgetData["zpos"] = zPosStr;
    
    widgetData["draggable"] = options.draggable;
    widgetData["clickthrough"] = options.clickThrough;
    widgetData["keeponscreen"] = options.keepOnScreen;
    widgetData["snapedges"] = options.snapEdges;
    
    // Only save if data has actually changed
    if (s_Data.contains("widgets") && s_Data["widgets"].contains(idStr))
    {
        if (s_Data["widgets"][idStr] == widgetData)
        {
            return; // No changes, skip saving
        }
    }
    
    s_Data["widgets"][idStr] = widgetData;
    s_Dirty = true;
    
    Save();
}

bool Settings::LoadWidget(const std::wstring& id, WidgetOptions& outOptions)
{
    if (id.empty()) return false;
    
    std::string idStr = Utils::ToString(id);
    
    if (!s_Data.contains("widgets") || !s_Data["widgets"].contains(idStr))
    {
        return false;
    }
    
    try {
        json& w = s_Data["widgets"][idStr];
        
        if (w.contains("x")) outOptions.x = w["x"];
        if (w.contains("y")) outOptions.y = w["y"];
        if (w.contains("windowopacity")) outOptions.windowOpacity = w["windowopacity"];
        
        if (w.contains("zpos")) {
            std::string z = w["zpos"];
            if (z == "ondesktop") outOptions.zPos = ZPOSITION_ONDESKTOP;
            else if (z == "ontop") outOptions.zPos = ZPOSITION_ONTOP;
            else if (z == "onbottom") outOptions.zPos = ZPOSITION_ONBOTTOM;
            else if (z == "ontopmost") outOptions.zPos = ZPOSITION_ONTOPMOST;
            else outOptions.zPos = ZPOSITION_NORMAL;
        }
        
        if (w.contains("draggable")) outOptions.draggable = w["draggable"];
        if (w.contains("clickthrough")) outOptions.clickThrough = w["clickthrough"];
        if (w.contains("keeponscreen")) outOptions.keepOnScreen = w["keeponscreen"];
        if (w.contains("snapedges")) outOptions.snapEdges = w["snapedges"];
        
        return true;
    } catch (std::exception& e) {
        Logging::Log(LogLevel::Error, L"Error loading widget %s: %S", id.c_str(), e.what());
        return false;
    }
}

void Settings::SetGlobalBool(const std::string& key, bool value)
{
    if (s_Data[key] != value) {
        s_Data[key] = value;
        s_Dirty = true;
        Save();
    }
}

bool Settings::GetGlobalBool(const std::string& key, bool defaultValue)
{
    if (s_Data.contains(key)) {
        return s_Data[key].get<bool>();
    }
    return defaultValue;
}
