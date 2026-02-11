// Shape Element Test

var startY = 20;
var spacing = 120;

// 1. Rectangle: Basic with Fill and Stroke
win.addShape({
    id: "rect-basic",
    type: "rectangle",
    x: 20, y: startY,
    width: 100, height: 80,
    fillColor: "#f00",
    strokeColor: "#fff",
    strokeWidth: 4
});

win.addText({
    id: "label-rect-basic",
    text: "Rect: Fill+Stroke",
    x: 140, y: startY + 30,
    fontColor: "#fff",
    fontSize: 16
});

// 2. Rectangle: Rounded with Gradient Fill
win.addShape({
    id: "rect-round-grad",
    type: "rectangle",
    x: 20, y: startY + spacing,
    width: 100, height: 80,
    radius: 20,
    fillColor: "linearGradient(45, #00f, #0ff)",
    strokeColor: "#fff",
    strokeWidth: 2
});

win.addText({
    id: "label-rect-round",
    text: "Rect: Rounded+Grad",
    x: 140, y: startY + spacing + 30,
    fontColor: "#fff",
    fontSize: 16
});

// 3. Ellipse: Basic
win.addShape({
    id: "ellipse-basic",
    type: "ellipse",
    x: 20, y: startY + spacing * 2,
    width: 100, height: 60,
    fillColor: "#0f0",
    strokeColor: "radialGradient(circle, #fff, #000)", // Gradient stroke!
    strokeWidth: 8
});

win.addText({
    id: "label-ellipse",
    text: "Ellipse: Grad Stroke",
    x: 140, y: startY + spacing * 2 + 20,
    fontColor: "#fff",
    fontSize: 16
});

// 4. Line: Diagonal
win.addShape({
    id: "line-diag",
    type: "line",
    startX: 20, startY: startY + spacing * 3,
    endX: 120, endY: startY + spacing * 3 + 80,
    strokeColor: "#ff0",
    strokeWidth: 5
});

win.addText({
    id: "label-line",
    text: "Line: Simple",
    x: 140, y: startY + spacing * 3 + 40,
    fontColor: "#fff",
    fontSize: 16
});

// 5. Line: Gradient Stroke
win.addShape({
    id: "line-grad",
    type: "line",
    startX: 20, startY: startY + spacing * 4,
    endX: 300, endY: startY + spacing * 4,
    strokeColor: "linearGradient(0, #f00, #00f)",
    strokeWidth: 10
});

win.addText({
    id: "label-line-grad",
    text: "Line: Gradient Stroke",
    x: 20, y: startY + spacing * 4 + 20,
    fontColor: "#fff",
    fontSize: 16
});

// 6. Arc
win.addShape({
    id: "arc-test",
    type: "arc",
    x: 20, y: startY + spacing * 5,
    width: 100, height: 100,
    radius: 40,
    startAngle: 0,
    endAngle: 270,
    clockwise: true,
    strokeColor: "#0ff",
    strokeWidth: 5,
    fillColor: "#333333aa" // Semi-transparent fill to see chord
});

win.addText({
    id: "label-arc",
    text: "Arc: 0-270 deg",
    x: 140, y: startY + spacing * 5 + 40,
    fontColor: "#fff",
    fontSize: 16
});

// 7. Path
win.addShape({
    id: "path-test",
    type: "path",
    x: 20, y: startY + spacing * 6,
    pathData: "M50 5 L61 39 L98 39 L68 59 L79 93 L50 72 L21 93 L32 59 L2 39 L39 39 Z", // Star
    fillColor: "#e0e",
    strokeColor: "#fff",
    strokeWidth: 3,
    strokeLineJoin: "Round",
    strokeDashes: [4, 2] // Dashed stroke!
});

win.addText({
    id: "label-path",
    text: "Path: Dashed Star",
    x: 140, y: startY + spacing * 6 + 40,
    fontColor: "#fff",
    fontSize: 16
});

console.log("ShapeTest loaded");
