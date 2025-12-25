/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!logToFile;

testWidget = new widgetWindow({});

testWidget.addText({
  id: "testText",
  text: "Hello World",
  fontsize: 20,
  x: 10,
  y: 10,
  fontcolor: "rgb(255,255,255)",
  onleftmouseup:
    "novadesk.log('Widget Properties:', JSON.stringify(WidgetProperties));",
});


WidgetProperties = testWidget.getProperties();

novadesk.log("Widget Properties:", JSON.stringify(WidgetProperties));

novadesk.onReady(function () {
  novadesk.log("On Ready FUnction Called");
});
