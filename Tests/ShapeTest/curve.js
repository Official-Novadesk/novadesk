// Curve shape tests (quadratic + cubic)

var startX = 20;
var startY = 20;
var spacingY = 140;
var labelX = 280;

// 1. Quadratic curve
win.addShape({
    id: "curve-quad",
    type: "curve",
    startX: startX, startY: startY + 80,
    controlX: startX + 120, controlY: startY - 40,
    endX: startX + 240, endY: startY + 80,
    curveType: "quadratic",
    strokeColor: "#1565c0",
    strokeWidth: 6,
    onLeftMouseUp: function () {
        console.log("Path clicked! Reversing direction and toggling caps.");
    }
});

win.addText({
    id: "label-curve-quad",
    text: "Curve: Quadratic",
    x: labelX, y: startY + 50,
    fontColor: "#111",
    fontSize: 16
});

// 2. Cubic curve
win.addShape({
    id: "curve-cubic",
    type: "curve",
    startX: startX, startY: startY + spacingY + 80,
    controlX: startX + 60, controlY: startY + spacingY - 20,
    control2X: startX + 180, control2Y: startY + spacingY + 180,
    endX: startX + 240, endY: startY + spacingY + 80,
    curveType: "cubic",
    strokeColor: "#d32f2f",
    strokeWidth: 6
});

win.addText({
    id: "label-curve-cubic",
    text: "Curve: Cubic",
    x: labelX, y: startY + spacingY + 50,
    fontColor: "#111",
    fontSize: 16
});

// 3. Cubic curve with dashed stroke + caps
win.addShape({
    id: "curve-cubic-dash",
    type: "curve",
    startX: startX, startY: startY + spacingY * 2 + 80,
    controlX: startX + 40, controlY: startY + spacingY * 2 - 20,
    control2X: startX + 200, control2Y: startY + spacingY * 2 + 180,
    endX: startX + 260, endY: startY + spacingY * 2 + 80,
    curveType: "cubic",
    strokeColor: "#2e7d32",
    strokeWidth: 8,
    strokeDashes: [12, 6],
    strokeDashCap: "Round"
});

win.addText({
    id: "label-curve-cubic-dash",
    text: "Curve: Cubic dashed",
    x: labelX, y: startY + spacingY * 2 + 50,
    fontColor: "#111",
    fontSize: 16
});

console.log("ShapeTest curve loaded");
