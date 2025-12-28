/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!logToFile;
// !enableDebugging;
var metrics = system.getDisplayMetrics();

var sysWidget = new widgetWindow({
    id: "sysWidget",
    width: 450,
    height: 600,
    backgroundcolor: "rgba(30, 30, 40, 0.9)",
    zpos: "ondesktop",
    draggable: true,
});

sysWidget.addText({
    id: "title",
    text: "Display Metrics",
    x: 20, y: 20,
    fontsize: 20,
    color: "rgb(255, 255, 255)",
    fontweight: "bold",
    onleftmouseup: "sysWidget.refresh();"
});