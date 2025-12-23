/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FileUtils.h"
#include <fstream>
#include <sstream>

namespace FileUtils {
    std::string ReadFileContent(const std::wstring& path) {
        std::ifstream t(path);
        if (!t.is_open()) return "";
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }
}
