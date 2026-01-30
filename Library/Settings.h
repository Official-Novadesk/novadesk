/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SETTINGS_H__
#define __NOVADESK_SETTINGS_H__

#include <string>
#include <map>
#include "Widget.h"
#include "json/json.hpp"

using json = nlohmann::json;

class Settings
{
public:
    static void Initialize();
    static void SaveWidget(const std::wstring& id, const WidgetOptions& options);
    static bool LoadWidget(const std::wstring& id, WidgetOptions& outOptions);
    static void ApplyGlobalSettings();
    static void Save();
    static std::wstring GetSettingsPath();
    static std::wstring GetLogPath();

    static void SetGlobalBool(const std::string& key, bool value);
    static bool GetGlobalBool(const std::string& key, bool defaultValue);

private:
    static void Load();
    static json s_Data;
    static bool s_Dirty;
};

#endif
