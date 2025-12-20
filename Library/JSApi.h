/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once
#include "duktape/duktape.h"

namespace JSApi {
    /*
    ** Initialize the JavaScript API and register all global functions.
    ** Registers the 'novadesk' object with log, error, and debug methods.
    ** Registers the 'widgetWindow' constructor for creating widget windows.
    */
    void InitializeJavaScriptAPI(duk_context* ctx);

    /*
    ** Load and execute the main JavaScript file (index.js).
    ** Searches for index.js in the same directory as the executable.
    ** Calls novadeskAppReady() function if defined in the script.
    ** Returns true on success, false on failure.
    */
    bool LoadAndExecuteScript(duk_context* ctx);

    /*
    ** Reload all JavaScript scripts.
    ** Destroys all existing widgets and reloads index.js.
    ** Useful for development and testing without restarting the application.
    */
    void ReloadScripts(duk_context* ctx);

    /*
    ** JavaScript API: novadesk.log(...)
    ** Logs informational messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_log(duk_context* ctx);

    /*
    ** JavaScript API: novadesk.error(...)
    ** Logs error messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_error(duk_context* ctx);

    /*
    ** JavaScript API: novadesk.debug(...)
    ** Logs debug messages to the debug output.
    ** Accepts variable number of arguments that are converted to strings.
    */
    duk_ret_t js_debug(duk_context* ctx);

    /*
    ** JavaScript API: new widgetWindow(options)
    ** Creates a new widget window with the specified options.
    ** Options include: width, height, backgroundColor, zPos, draggable,
    ** clickThrough, keepOnScreen, and snapEdges.
    */
    duk_ret_t js_create_widget_window(duk_context* ctx);

    // Widget content methods
    duk_ret_t js_widget_add_image(duk_context* ctx);
    duk_ret_t js_widget_add_text(duk_context* ctx);
    duk_ret_t js_widget_update_image(duk_context* ctx);
    duk_ret_t js_widget_update_text(duk_context* ctx);
    duk_ret_t js_widget_remove_content(duk_context* ctx);
    duk_ret_t js_widget_clear_content(duk_context* ctx);
}
