// BarElement hit-test demo: clicks should register only on drawn pixels.

var startX = 30;
var startY = 30;
var spacingY = 120;

function makeBar(id, x, y, w, h, value, orientation, label) {
    var isOn = false;
    win.addBar({
        id: id,
        x: x, y: y,
        width: w, height: h,
        value: value,
        orientation: orientation,
        backgroundColor: "#222222",
        barColor: "#00c853",
        barCornerRadius: 10,
        onLeftMouseDown: function () {
            isOn = !isOn;
            win.setElementProperties(id, {
                barColor: isOn ? "#ff6f00" : "#00c853"
            });
        }
    });

    win.addText({
        id: id + "-label",
        text: label,
        x: x + w + 20,
        y: y + 20,
        fontColor: "#ffffff",
        fontSize: 16
    });
}

// 1. Horizontal bar (50%)
makeBar("bar-hit-h", startX, startY, 300, 30, 0.5, "horizontal", "HitTest: horizontal 50%");

// 2. Vertical bar (35%)
makeBar("bar-hit-v", startX, startY + spacingY, 40, 200, 0.35, "vertical", "HitTest: vertical 35%");

// 3. Rounded bar with gradient
win.addBar({
    id: "bar-hit-grad",
    x: startX, y: startY + spacingY * 2,
    width: 320, height: 34,
    value: 0.6,
    orientation: "horizontal",
    backgroundColor: "#1c1c1c",
    barColor: "linearGradient(0, #00e5ff, #1de9b6)",
    barCornerRadius: 16,
    onLeftMouseDown: function () {
        win.setElementProperties("bar-hit-grad", {
            value: 0.9
        });
    }
});

win.addText({
    id: "bar-hit-grad-label",
    text: "HitTest: gradient bar (click to set 90%)",
    x: startX + 340,
    y: startY + spacingY * 2 + 8,
    fontColor: "#ffffff",
    fontSize: 16
});

console.log("BarElement hit-test demo loaded");
