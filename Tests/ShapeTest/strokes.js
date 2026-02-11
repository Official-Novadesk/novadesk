// Stroke feature coverage tests for Shape element.

var startX = 20;
var startY = 20;
var spacingY = 110;
var labelX = 250;

// 1. Basic stroke on rectangle
win.addShape({
    id: "stroke-rect-basic",
    type: "rectangle",
    x: startX, y: startY,
    width: 180, height: 70,
    fillColor: "#f2f2f2",
    strokeColor: "#222",
    strokeWidth: 4
});

win.addText({
    id: "label-stroke-rect-basic",
    text: "Rect: solid stroke",
    x: labelX, y: startY + 20,
    fontColor: "#111",
    fontSize: 16
});

// 2. Gradient stroke on rectangle
win.addShape({
    id: "stroke-rect-gradient",
    type: "rectangle",
    x: startX, y: startY + spacingY,
    width: 180, height: 70,
    radius: 12,
    fillColor: "#ffffff",
    strokeColor: "linearGradient(0, #ff0000, #00ff00, #0000ff)",
    strokeWidth: 6
});

win.addText({
    id: "label-stroke-rect-gradient",
    text: "Rect: gradient stroke",
    x: labelX, y: startY + spacingY + 20,
    fontColor: "#111",
    fontSize: 16
});

// 3. Line caps: Round
win.addShape({
    id: "stroke-line-round",
    type: "line",
    startX: startX, startY: startY + spacingY * 2 + 20,
    endX: startX + 180, endY: startY + spacingY * 2 + 20,
    strokeColor: "#1b6",
    strokeWidth: 14,
    strokeStartCap: "Round",
    strokeEndCap: "Round"
});

win.addText({
    id: "label-stroke-line-round",
    text: "Line caps: Round",
    x: labelX, y: startY + spacingY * 2,
    fontColor: "#111",
    fontSize: 16
});

// 4. Line caps: Square
win.addShape({
    id: "stroke-line-square",
    type: "line",
    startX: startX, startY: startY + spacingY * 3 + 20,
    endX: startX + 180, endY: startY + spacingY * 3 + 20,
    strokeColor: "#e91e63",
    strokeWidth: 14,
    strokeStartCap: "Square",
    strokeEndCap: "Square"
});

win.addText({
    id: "label-stroke-line-square",
    text: "Line caps: Square",
    x: labelX, y: startY + spacingY * 3,
    fontColor: "#111",
    fontSize: 16
});

// 5. Line caps: Triangle
win.addShape({
    id: "stroke-line-triangle",
    type: "line",
    startX: startX, startY: startY + spacingY * 4 + 20,
    endX: startX + 180, endY: startY + spacingY * 4 + 20,
    strokeColor: "#ff9800",
    strokeWidth: 14,
    strokeStartCap: "Triangle",
    strokeEndCap: "Triangle"
});

win.addText({
    id: "label-stroke-line-triangle",
    text: "Line caps: Triangle",
    x: labelX, y: startY + spacingY * 4,
    fontColor: "#111",
    fontSize: 16
});

// 6. Line joins: Miter (sharp), Bevel, Round, MiterOrBevel
var zigzag = "M20 0 L70 40 L120 0 L170 40 L220 0";
var joinY = startY + spacingY * 5 + 10;

win.addShape({
    id: "stroke-join-miter",
    type: "path",
    x: startX, y: joinY,
    pathData: zigzag,
    strokeColor: "#333",
    strokeWidth: 10,
    strokeLineJoin: "Miter"
});

win.addText({
    id: "label-stroke-join-miter",
    text: "Join: Miter",
    x: labelX, y: joinY + 5,
    fontColor: "#111",
    fontSize: 16
});

win.addShape({
    id: "stroke-join-bevel",
    type: "path",
    x: startX, y: joinY + 40,
    pathData: zigzag,
    strokeColor: "#333",
    strokeWidth: 10,
    strokeLineJoin: "Bevel"
});

win.addText({
    id: "label-stroke-join-bevel",
    text: "Join: Bevel",
    x: labelX, y: joinY + 45,
    fontColor: "#111",
    fontSize: 16
});

win.addShape({
    id: "stroke-join-round",
    type: "path",
    x: startX, y: joinY + 80,
    pathData: zigzag,
    strokeColor: "#333",
    strokeWidth: 10,
    strokeLineJoin: "Round"
});

win.addText({
    id: "label-stroke-join-round",
    text: "Join: Round",
    x: labelX, y: joinY + 85,
    fontColor: "#111",
    fontSize: 16
});

win.addShape({
    id: "stroke-join-miter-or-bevel",
    type: "path",
    x: startX, y: joinY + 120,
    pathData: zigzag,
    strokeColor: "#333",
    strokeWidth: 10,
    strokeLineJoin: "MiterOrBevel"
});

win.addText({
    id: "label-stroke-join-miter-or-bevel",
    text: "Join: MiterOrBevel",
    x: labelX, y: joinY + 125,
    fontColor: "#111",
    fontSize: 16
});

// 7. Dashed stroke with dash cap + offset
var dashY = joinY + 180;
win.addShape({
    id: "stroke-dash-round-cap",
    type: "line",
    startX: startX, startY: dashY,
    endX: startX + 220, endY: dashY,
    strokeColor: "#1565c0",
    strokeWidth: 10,
    strokeDashes: [12, 6],
    strokeDashOffset: 0,
    strokeDashCap: "Round"
});

win.addText({
    id: "label-stroke-dash-round",
    text: "Dash: [12,6], cap Round",
    x: labelX, y: dashY - 10,
    fontColor: "#111",
    fontSize: 16
});

win.addShape({
    id: "stroke-dash-offset",
    type: "line",
    startX: startX, startY: dashY + 40,
    endX: startX + 220, endY: dashY + 40,
    strokeColor: "#1565c0",
    strokeWidth: 10,
    strokeDashes: [12, 6],
    strokeDashOffset: 9,
    strokeDashCap: "Flat"
});

win.addText({
    id: "label-stroke-dash-offset",
    text: "Dash offset: 9",
    x: labelX, y: dashY + 30,
    fontColor: "#111",
    fontSize: 16
});

console.log("ShapeTest strokes loaded");
