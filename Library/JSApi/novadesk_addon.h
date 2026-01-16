/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "duktape/duktape.h"

// Define the initialization function signature
// This function is required in every Novadesk addon DLL.
// It is called when the addon is loaded via system.loadAddon().
typedef void (*NovadeskAddonInitFn)(duk_context* ctx);

// Define the cleanup function signature (optional)
// If exported, it is called when the script is reloaded or Novadesk exits.
typedef void (*NovadeskAddonUnloadFn)();

#ifdef __cplusplus
}
#endif
