// Arc feature coverage tests for Shape element.

var startX = 20;
var startY = 20;
var spacingY = 180;
var labelX = 260;

// 1. Basic arc: 0 -> 180, clockwise
win.addShape({
    id: "arc-basic",
    type: "arc",
    x: startX, y: startY,
    width: 180, height: 180,
    radius: 80,
    startAngle: 0,
    endAngle: 180,
    clockwise: true,
    strokeColor: "#222",
    strokeWidth: 6,
    fillColor: "#e0e0e0"
});

win.addText({
    id: "label-arc-basic",
    text: "Arc: 0-180 cw",
    x: labelX, y: startY + 70,
    fontColor: "#111",
    fontSize: 16
});

// 2. Counter-clockwise arc
win.addShape({
    id: "arc-ccw",
    type: "arc",
    x: startX, y: startY + spacingY,
    width: 180, height: 180,
    radius: 80,
    startAngle: 0,
    endAngle: 220,
    clockwise: false,
    strokeColor: "#1565c0",
    strokeWidth: 6,
    fillColor: "#ffffff"
});

win.addText({
    id: "label-arc-ccw",
    text: "Arc: 0-220 ccw",
    x: labelX, y: startY + spacingY + 70,
    fontColor: "#111",
    fontSize: 16
});

// 3. Gradient stroke arc
win.addShape({
    id: "arc-grad-stroke",
    type: "arc",
    x: startX, y: startY + spacingY * 2,
    width: 180, height: 180,
    radius: 80,
    startAngle: 45,
    endAngle: 300,
    clockwise: true,
    strokeColor: "linearGradient(0, #ff0000, #00ff00, #0000ff)",
    strokeWidth: 8,
    fillColor: "#f7f7f7"
});

win.addText({
    id: "label-arc-grad-stroke",
    text: "Arc: gradient stroke",
    x: labelX, y: startY + spacingY * 2 + 70,
    fontColor: "#111",
    fontSize: 16
});

// 4. Dashed arc
win.addShape({
    id: "arc-dashed",
    type: "arc",
    x: startX, y: startY + spacingY * 3,
    width: 180, height: 180,
    radius: 80,
    startAngle: 0,
    endAngle: 300,
    clockwise: true,
    strokeColor: "#2e7d32",
    strokeWidth: 8,
    strokeDashes: [10, 6],
    strokeDashCap: "Round",
    fillColor: "#ffffff"
});

win.addText({
    id: "label-arc-dashed",
    text: "Arc: dashed stroke",
    x: labelX, y: startY + spacingY * 3 + 70,
    fontColor: "#111",
    fontSize: 16
});

// 5. Thick arc with dash offset
win.addShape({
    id: "arc-dash-offset",
    type: "arc",
    x: startX, y: startY + spacingY * 4,
    width: 180, height: 180,
    radius: 80,
    startAngle: 30,
    endAngle: 330,
    clockwise: true,
    strokeColor: "#ff6f00",
    strokeWidth: 12,
    strokeDashes: [14, 8],
    strokeDashOffset: 8,
    strokeDashCap: "Flat",
    fillColor: "#ffffff",
        onLeftMouseUp: function() {
        console.log("Path clicked! Reversing direction and toggling caps.");
    }
});

win.addText({
    id: "label-arc-dash-offset",
    text: "Arc: dash offset",
    x: labelX, y: startY + spacingY * 4 + 70,
    fontColor: "#111",
    fontSize: 16
});

console.log("ShapeTest arc loaded");
