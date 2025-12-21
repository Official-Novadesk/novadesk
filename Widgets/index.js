/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

function createContentDemoWindow() {
    var demoWidget = new widgetWindow({
        id: "demoWidget",
        width: 800,
        height: 800,
        backgroundcolor: "rgba(10,10,10,200)",
        zpos: "normal",
        draggable: true,
        keeponscreen: true
    });

    demoWidget.addText({
        id: "demoText",
        text: "Novadesk",
        x: 0, y: 0,
        width: 300,
        height: 300,
        fontsize: 32,
        align: "centercenter",
        color: "rgba(255,255,255,255)",
        onleftmouseup: "novadesk.log('Text clicked!')"
    });

    novadesk.log("Content demo widget created");
}

function onAppReady() {
    createContentDemoWindow();
}
