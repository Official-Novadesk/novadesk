/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

var sysWidget;
var cpuMonitor;
var memMonitor;

function createSystemWidget() {
    sysWidget = new widgetWindow({
        id: "sysWidget",
        x: 10, y: 120, // Below the clock
        backgroundcolor: "rgba(0,0,0,150)",
        zpos: "ondesktop",
        draggable: true
    });

    sysWidget.addText({
        id: "cpuText",
        text: "CPU: Calculating...",
        x: 10, y: 10,
        fontsize: 14,
        color: "rgb(255,255,255)"
    });

    sysWidget.addText({
        id: "memText",
        text: "RAM: Calculating...",
        x: 10, y: 35,
        fontsize: 14,
        color: "rgb(255,255,255)"
    });

    // Initialize monitors
    cpuMonitor = new novadesk.system.CPU();
    memMonitor = new novadesk.system.Memory();

    // Update every 2 seconds
    novadesk.setInterval(function () {
        var cpuUsage = cpuMonitor.usage();
        var memStats = memMonitor.stats();
        var ws = novadesk.system.getWorkspaceVariables();

        sysWidget.updateText("cpuText", "CPU: " + cpuUsage.toFixed(1) + "% (" + ws.WORKAREAWIDTH + "px wide)");

        var usedGB = (memStats.used / (1024 * 1024 * 1024)).toFixed(1);
        var totalGB = (memStats.total / (1024 * 1024 * 1024)).toFixed(0);
        sysWidget.updateText("memText", "RAM: " + usedGB + "GB / " + totalGB + "GB (" + memStats.percent + "%)");
    }, 2000);
}
