/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

var clockWidget;
var clockInterval;

function createClockWidget() {
    clockWidget = new widgetWindow({
        id: "clockWidget",
        x: 10, y: 10,
        backgroundcolor: "rgba(0,0,0,150)",
        zpos: "ondesktop",
        draggable: true
    });

    clockWidget.addText({
        id: "timeDisplay",
        text: "Loading time...",
        x: 10, y: 10,
        fontsize: 20,
        color: "white"
    });

    clockWidget.addText({
        id: "timerStatus",
        text: "Interval: Active",
        x: 10, y: 40,
        fontsize: 10,
        color: "#aaa"
    });

    function pad(n) {
        return n < 10 ? '0' + n : n;
    }

    // Update every second
    clockInterval = novadesk.setInterval(function () {
        var now = new Date();
        var timeStr = pad(now.getHours()) + ":" +
            pad(now.getMinutes()) + ":" +
            pad(now.getSeconds());

        clockWidget.updateText("timeDisplay", timeStr);
        novadesk.log("Clock widget updated via setInterval");
    }, 1000);

    // Test setTimeout
    novadesk.setTimeout(function () {
        novadesk.log("One-shot timeout fired from clock widget!");
    }, 5000);

    // Test setImmediate
    novadesk.setImmediate(function () {
        novadesk.log("Clock widget initialized via setImmediate");
    });
}
