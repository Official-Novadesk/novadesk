/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs
{
    void SetModuleSystemDebug(bool debug);
    void SetUiScriptImportRestricted(bool restricted);
    char *ModuleNormalizeName(JSContext *ctx, const char *baseName, const char *name, void *opaque);
    JSModuleDef *ModuleLoader(JSContext *ctx, const char *moduleName, void *opaque);
} // namespace novadesk::scripting::quickjs
