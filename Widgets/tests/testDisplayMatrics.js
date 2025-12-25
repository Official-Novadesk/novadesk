/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!logToFile;
!enableDebugging;

var metrics = novadesk.system.getDisplayMetrics();

var sysWidget = new widgetWindow({
  id: "sysWidget",
  width: 450,
  height: 600,
  backgroundcolor: "rgba(30, 30, 40, 0.9)",
});

sysWidget.addText({
  id: "title",
  text: "Display Metrics",
  x: 20, y: 20,
  fontsize: 20,
  color: "rgb(255, 255, 255)",
  fontweight: "bold"
});

var yOffset = 60;

function addMetricText(id, label, value, xOffset) {
  xOffset = xOffset || 0;
  sysWidget.addText({
    id: id,
    text: label + (value !== "" ? ": " + value : ""),
    x: 20 + xOffset, y: yOffset,
    fontsize: 14,
    color: "rgb(200, 200, 200)"
  });
  yOffset += 25;
}

// Show Primary Monitor info
if (metrics.primary) {
  addMetricText("primary_label", "Primary Monitor", "", 0);
  addMetricText("primary_wa", "  Work Area", metrics.primary.workArea.width + "x" + metrics.primary.workArea.height + " @ " + metrics.primary.workArea.x + "," + metrics.primary.workArea.y, 10);
  addMetricText("primary_sa", "  Screen Area", metrics.primary.screenArea.width + "x" + metrics.primary.screenArea.height + " @ " + metrics.primary.screenArea.x + "," + metrics.primary.screenArea.y, 10);
  yOffset += 10;
}

// Show Virtual Screen info
addMetricText("vs_label", "Virtual Screen", "", 0);
addMetricText("vs_dim", "  Dimensions", metrics.virtualScreen.width + "x" + metrics.virtualScreen.height + " @ " + metrics.virtualScreen.x + "," + metrics.virtualScreen.y, 10);
yOffset += 10;

// Show All Monitors
addMetricText("monitors_label", "All Monitors (" + metrics.monitors.length + ")", "", 0);
metrics.monitors.forEach(function (m) {
  addMetricText("m" + m.id + "_label", "  Monitor " + m.id, "", 10);
  addMetricText("m" + m.id + "_wa", "    Work Area", m.workArea.width + "x" + m.workArea.height, 20);
  addMetricText("m" + m.id + "_sa", "    Screen Area", m.screenArea.width + "x" + m.screenArea.height, 20);
});

novadesk.log("System Metrics displayed and in JSON: " + JSON.stringify(metrics));
