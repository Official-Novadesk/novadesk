/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <dwrite_1.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <map>

namespace FontManager
{
    // Initializes the font manager system
    bool Initialize();
    
    // Cleans up all cached font collections
    void Cleanup();

    /*
    ** Gets or creates a font collection from the specified directory.
    ** The path can be absolute or relative to the executable.
    */
    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetFontCollection(const std::wstring& directoryPath);
}
