/* ClipString Test Widget */

var clipWidget;

function createClipTest() {
    clipWidget = new widgetWindow({
        id: "clipTest",
        x: 100, y: 100,
        width: 500, height: 400,
        backgroundcolor: "rgba(30,30,30,220)",
        draggable: true
    });

    var longText = "This is a very long string that should be clipped or truncated with an ellipsis if the ClipString property is working correctly.";

    // 1. No Clipping (Default)
    clipWidget.addText({
        id: "noClip",
        text: "NONE: " + longText,
        x: 10, y: 10, width: 480, height: 40,
        fontsize: 12, color: "white"
    });

    // 2. ClipString=1 (Character Clipping)
    clipWidget.addText({
        id: "clipOn",
        text: "CLIP_ON: " + longText,
        x: 10, y: 60, width: 200, height: 40,
        fontsize: 12, color: "cyan",
        clipstring: 1
    });

    // 3. ClipString=2 (Ellipsis)
    clipWidget.addText({
        id: "clipEllipsis",
        text: "ELLIPSIS: " + longText,
        x: 10, y: 110, width: 200, height: 40,
        fontsize: 12, color: "yellow",
        clipstring: 2
    });

    // 4. ClipStringW (Auto-width capped at 150)
    clipWidget.addText({
        id: "clipW",
        text: "CLIP_W (capped 150): " + longText,
        x: 10, y: 160,
        fontsize: 12, color: "lime",
        clipstring: 2, clipstringw: 150
    });

    novadesk.log("ClipString test widget created.");
}

novadesk.onReady(function () {
    createClipTest();
});
