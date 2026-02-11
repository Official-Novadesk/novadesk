// Path feature coverage tests for Shape element.

var startX = 20;
var startY = 20;
var spacingY = 120;
var labelX = 260;

// 1. Basic closed path with solid stroke
win.addShape({
    id: "path-star-solid",
    type: "path",
    x: startX, y: startY,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z",
    fillColor: "#f3e5f5",
    strokeColor: "#222",
    strokeWidth: 4,
    strokeLineJoin: "Miter"
});

win.addText({
    id: "label-path-star-solid",
    text: "Path: star, solid",
    x: labelX, y: startY + 35,
    fontColor: "#111",
    fontSize: 16
});

// 2. Rounded joins + thicker stroke
win.addShape({
    id: "path-star-round",
    type: "path",
    x: startX, y: startY + spacingY,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z",
    fillColor: "#e8f5e9",
    strokeColor: "#2e7d32",
    strokeWidth: 8,
    strokeLineJoin: "Round"
});

win.addText({
    id: "label-path-star-round",
    text: "Path: round joins",
    x: labelX, y: startY + spacingY + 35,
    fontColor: "#111",
    fontSize: 16
});

// 3. Bevel joins + gradient stroke
win.addShape({
    id: "path-star-bevel",
    type: "path",
    x: startX, y: startY + spacingY * 2,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z",
    fillColor: "#fff3e0",
    strokeColor: "linearGradient(0, #ff0000, #00ff00, #0000ff)",
    strokeWidth: 6,
    strokeLineJoin: "Bevel"
});

win.addText({
    id: "label-path-star-bevel",
    text: "Path: bevel join, grad stroke",
    x: labelX, y: startY + spacingY * 2 + 35,
    fontColor: "#111",
    fontSize: 16
});

// 4. Dashed stroke with dash cap
win.addShape({
    id: "path-dashed",
    type: "path",
    x: startX, y: startY + spacingY * 3,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z",
    fillColor: "#ffffff",
    strokeColor: "#1565c0",
    strokeWidth: 6,
    strokeDashes: [10, 6],
    strokeDashCap: "Round",
    strokeLineJoin: "Miter"
});

win.addText({
    id: "label-path-dashed",
    text: "Path: dashed",
    x: labelX, y: startY + spacingY * 3 + 35,
    fontColor: "#111",
    fontSize: 16
});

// 5. Open path with line caps
win.addShape({
    id: "path-open-caps",
    type: "path",
    x: startX, y: startY + spacingY * 4,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z",
    fillColor: "#ffffff",
    strokeColor: "#e91e63",
    strokeWidth: 10,
    strokeStartCap: "Round",
    strokeEndCap: "Square",
    onLeftMouseUp: function() {
        console.log("Path clicked! Reversing direction and toggling caps.");
    }
});


win.addText({
    id: "label-path-open-caps",
    text: "Path: open, caps",
    x: labelX, y: startY + spacingY * 4 + 35,
    fontColor: "#111",
    fontSize: 16
});

console.log("ShapeTest path loaded");
