// Combine shapes demo

try {
    if (!win) {
        throw new Error("combine.js: 'win' is not defined.");
    }


// Base shapes
win.addShape({
    id: "baseCircle",
    type: "ellipse",
    x: 40, y: 40,
    width: 140, height: 140,
    fillColor: "#4a90e2",
    strokeColor: "#1f3f66",
    strokeWidth: 3
});

win.addShape({
    id: "cutRect",
    type: "rectangle",
    x: 90, y: 20,
    width: 120, height: 120,
    fillColor: "#8bc34a",
    strokeColor: "#3b6a1d",
    strokeWidth: 3,
    rotate: 10
});

// Combined shape (xor)
win.addShape({
    id: "comboXor",
    type: "combine",
    base: "baseCircle",
    consume: true,
    ops: [
        { id: "cutRect", mode: "xor", consume: true }
    ],
    x: 0, y: 0,
    fillColor: "#ffcf33",
    strokeColor: "#333333",
    strokeWidth: 2
});

// Additional shapes for union/intersect
win.addShape({
    id: "leftCircle",
    type: "ellipse",
    x: 260, y: 60,
    width: 110, height: 110,
    fillColor: "#ef5350",
    strokeColor: "#7f1d1d",
    strokeWidth: 2
});

win.addShape({
    id: "rightCircle",
    type: "ellipse",
    x: 320, y: 60,
    width: 110, height: 110,
    fillColor: "#66bb6a",
    strokeColor: "#1b5e20",
    strokeWidth: 2
});

// Union (keep originals)
win.addShape({
    id: "comboUnion",
    type: "combine",
    base: "leftCircle",
    ops: [
        { id: "rightCircle", mode: "union", consume: false }
    ],
    fillColor: "#8e24aa",
    strokeColor: "#2e003e",
    strokeWidth: 2
});

// Visibility test (click box to toggle show + log size)
win.addText({
    id: "visLabel",
    text: "Click the box to toggle visibility",
    x: 40, y: 220,
    fontColor: "#111",
    fontSize: 14
});

win.addText({
    id: "visText",
    text: "I hide/show",
    x: 40, y: 240,
    fontColor: "#1f3f66",
    fontSize: 16
});

win.addImage({
    id: "visImage",
    x: 160, y: 220,
    width: 48, height: 48,
    path: "assets/pic.png",
    preserveAspectRatio: "preserve"
});

var visHidden = false;
win.addShape({
    id: "visToggle",
    type: "rectangle",
    x: 240, y: 215,
    width: 160, height: 60,
    radius: 10,
    fillColor: "#eeeeee",
    strokeColor: "#333333",
    strokeWidth: 2,
    onLeftMouseUp: function () {
        var beforeW = win.getElementProperty("visText", "width");
        var beforeH = win.getElementProperty("visText", "height");
        var beforeW2 = win.getElementProperty("visImage", "width");
        var beforeH2 = win.getElementProperty("visImage", "height");

        console.log("Before toggle - visText:", beforeW, beforeH, "visImage:", beforeW2, beforeH2);

        visHidden = !visHidden;
        win.setElementProperties("visText", { show: !visHidden });
        win.setElementProperties("visImage", { show: !visHidden });

        var afterW = win.getElementProperty("visText", "width");
        var afterH = win.getElementProperty("visText", "height");
        var afterW2 = win.getElementProperty("visImage", "width");
        var afterH2 = win.getElementProperty("visImage", "height");

        console.log("After toggle - visText:", afterW, afterH, "visImage:", afterW2, afterH2, "show:", !visHidden);
    }
});
} catch (e) {
    console.log("Combine demo error:", e && (e.stack || e.message || e));
}
