/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!logToFile;
!enableDebugging;

var gradWidget = new widgetWindow({
    id: "gradTest",
    width: 600,
    height: 400,
    backgroundcolor: "rgba(30, 30, 35, 1.0)",
});

function addGradientBox(id, x, y, angle, label) {
    gradWidget.addText({
        id: id + "_label",
        text: label + " (" + angle + "deg)",
        x: x, y: y - 25,
        fontsize: 14,
        color: "rgb(255, 255, 255)"
    });

    // Create an image element to use as a background box with gradient
    // Elements support solidcolor and solidcolor2
    gradWidget.addText({
        id: id,
        text: "",
        x: x, y: y,
        width: 150, height: 100,
        solidcolor: "rgb(255, 0, 0)",      // Red
        solidcolor2: "rgb(0, 0, 255)",    // Blue
        gradientangle: angle
    });
}

// Test various angles
addGradientBox("box0", 50, 50, 0, "Horizontal");
addGradientBox("box90", 250, 50, 90, "Vertical");
addGradientBox("box45", 450, 50, 45, "Diagonal");

addGradientBox("box135", 50, 250, 135, "Diagonal 2");
addGradientBox("box180", 250, 250, 180, "Horizontal Reverse");
addGradientBox("box270", 450, 250, 270, "Vertical Reverse");

novadesk.log("Gradient test widget created. Check if colors are correctly distributed.");
