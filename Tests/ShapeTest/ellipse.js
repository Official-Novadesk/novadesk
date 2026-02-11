// Ellipse feature coverage tests for Shape element.

var startX = 20;
var startY = 20;
var spacingY = 120;
var labelX = 260;

// 1. Basic ellipse: fill + stroke
win.addShape({
    id: "ellipse-basic",
    type: "ellipse",
    x: startX, y: startY,
    width: 180, height: 90,
    fillColor: "#f0f0f0",
    strokeColor: "#222",
    strokeWidth: 4
});

win.addText({
    id: "label-ellipse-basic",
    text: "Ellipse: solid stroke",
    x: labelX, y: startY + 30,
    fontColor: "#111",
    fontSize: 16
});

// 2. Gradient fill + solid stroke
win.addShape({
    id: "ellipse-grad-fill",
    type: "ellipse",
    x: startX, y: startY + spacingY,
    width: 180, height: 90,
    fillColor: "linearGradient(45, #00f, #0ff)",
    strokeColor: "#111",
    strokeWidth: 3
});

win.addText({
    id: "label-ellipse-grad-fill",
    text: "Ellipse: gradient fill",
    x: labelX, y: startY + spacingY + 30,
    fontColor: "#111",
    fontSize: 16
});

// 3. Gradient stroke
win.addShape({
    id: "ellipse-grad-stroke",
    type: "ellipse",
    x: startX, y: startY + spacingY * 2,
    width: 180, height: 90,
    fillColor: "#ffffff",
    strokeColor: "radialGradient(circle, #ff0000, #00ff00)",
    strokeWidth: 8
});

win.addText({
    id: "label-ellipse-grad-stroke",
    text: "Ellipse: gradient stroke",
    x: labelX, y: startY + spacingY * 2 + 30,
    fontColor: "#111",
    fontSize: 16
});

// 4. Dashed stroke
win.addShape({
    id: "ellipse-dashed",
    type: "ellipse",
    x: startX, y: startY + spacingY * 3,
    width: 180, height: 90,
    fillColor: "#fafafa",
    strokeColor: "#1565c0",
    strokeWidth: 6,
    strokeDashes: [10, 6],
    strokeDashCap: "Round"
});

win.addText({
    id: "label-ellipse-dashed",
    text: "Ellipse: dashed stroke",
    x: labelX, y: startY + spacingY * 3 + 30,
    fontColor: "#111",
    fontSize: 16
});

// 5. Dash offset + thicker stroke
win.addShape({
    id: "ellipse-dash-offset",
    type: "ellipse",
    x: startX, y: startY + spacingY * 4,
    width: 180, height: 90,
    fillColor: "#ffffff",
    strokeColor: "#ff6f00",
    strokeWidth: 10,
    strokeDashes: [14, 8],
    strokeDashOffset: 8,
    strokeDashCap: "Flat"
});

win.addText({
    id: "label-ellipse-dash-offset",
    text: "Ellipse: dash offset",
    x: labelX, y: startY + spacingY * 4 + 30,
    fontColor: "#111",
    fontSize: 16
});

// 6. Rotated ellipse
win.addShape({
    id: "ellipse-rotated",
    type: "ellipse",
    x: startX, y: startY + spacingY * 5,
    width: 180, height: 90,
    fillColor: "#e8f5e9",
    strokeColor: "#2e7d32",
    strokeWidth: 5,
    rotate: 30
});

win.addText({
    id: "label-ellipse-rotated",
    text: "Ellipse: rotated",
    x: labelX, y: startY + spacingY * 5 + 30,
    fontColor: "#111",
    fontSize: 16
});

console.log("ShapeTest ellipse loaded");
