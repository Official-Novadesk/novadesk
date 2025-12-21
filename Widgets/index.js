/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// Use novadesk.include to load another script globally
novadesk.include("testWidget.js");
novadesk.include("timerDemo.js");

function onAppReady() {
    novadesk.log("App ready, initializing widgets...");
    createContentDemoWindow();
    createClockWidget();
}
