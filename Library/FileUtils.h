/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

namespace FileUtils {
    /*
    ** Read the entire content of a file into a string.
    ** Supports UTF-16 file paths (std::wstring).
    ** Returns an empty string if the file cannot be opened.
    */
    std::string ReadFileContent(const std::wstring& path);
}
