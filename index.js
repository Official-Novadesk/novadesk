/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

var mainWindow;

function createMainWindow() {
    mainWindow = new widgetWindow({
        id: "mainWidget",
        width: 200,
        height: 200,
        backgroundColor: "rgba(10,10,10,200)",
        zPos: "ondesktop",
        draggable: true,
        snapEdges: true,
        keepOnScreen: true
    });
    novadesk.log("Main widget created");
}

function createContentDemoWindow() {
    var demoWidget = new widgetWindow({
        id: "demoWidget",
        width: 300,
        height: 400,
        backgroundColor: "rgba(30,30,40,240)",
        zPos: "normal",
        draggable: true,
        keepOnScreen: true
    });

    // Add Title
    demoWidget.addText({
        id: "title",
        x: 0,
        y: 20,
        width: 300,
        height: 40,
        text: "Weather Widget",
        fontFamily: "Segoe UI",
        fontSize: 24,
        color: "rgba(255,255,255,255)",
        fontWeight: "bold",
        align: "center"
    });

    demoWidget.addText({
        id: "demoText",
        text: "Hover & Click Me!",
        x: 20, y: 200,
        width: 260, height: 30,
        fontSize: 24,
        color: "rgba(255,255,255,255)",
        onMouseOver: "novadesk.log('Mouse entered text!')",
        onMouseLeave: "novadesk.log('Mouse left text!')",
        onLeftMouseUp: "novadesk.log('Text clicked!')"
    });

    demoWidget.addImage({
        id: "demoIcon",
        path: "icon.png", // Assuming icon.png exists or use a placebo
        x: 100, y: 100,
        width: 64, height: 64,
        onRightMouseUp: "novadesk.log('Icon Right Clicked!')",
        onScrollUp: "novadesk.log('Scrolled Up over Icon!')",
        onScrollDown: "novadesk.log('Scrolled Down over Icon!')"
    });

    // Add Temperature
    demoWidget.addText({
        id: "temp",
        x: 0,
        y: 80,
        width: 300,
        height: 80,
        text: "72°",
        fontFamily: "Segoe UI",
        fontSize: 64,
        color: "rgba(255,255,255,255)",
        align: "center"
    });

    // Add Condition Text
    demoWidget.addText({
        id: "condition",
        x: 0,
        y: 160,
        width: 300,
        height: 30,
        text: "Partly Cloudy",
        fontFamily: "Segoe UI",
        fontSize: 16,
        color: "rgba(200,200,200,255)",
        fontStyle: "italic",
        align: "center"
    });

    novadesk.log("Content demo widget created");
}

function novadeskAppReady() {
    createMainWindow();
    createContentDemoWindow();
}
