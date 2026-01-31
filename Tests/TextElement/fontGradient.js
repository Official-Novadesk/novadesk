// Gradient Font Support Test
// Verify linear and radial gradients on fontColor

var startY = 20;
var spacing = 70;

win.addText({
    id: "title",
    text: "Gradient Font Support Test",
    x: 20,
    y: startY,
    fontSize: 28,
    fontWeight: 700,
    fontColor: "linearGradient(to right, #ff8c00, #ff0080)"
});

startY += 60;

// 1. Linear Gradient: Left to Right
win.addText({
    id: "linear-lrt",
    x: 20, y: startY,
    text: "Linear: To Right (Red -> Green -> Blue)",
    fontSize: 24,
    fontWeight: 700,
    fontColor: "linearGradient(to right, #ff0000, #00ff00, #0000ff)"
});

// 2. Linear Gradient: Diagonal
win.addText({
    id: "linear-diagonal",
    x: 20, y: startY + spacing,
    text: "Linear: 45deg (Purple -> Orange)",
    fontSize: 24,
    fontWeight: 700,
    fontColor: "linearGradient(45deg, #a020f0, #ffa500)"
});

// 3. Linear Gradient: Vertical
win.addText({
    id: "linear-vertical",
    x: 20, y: startY + spacing * 2,
    text: "Linear: To Bottom (Cyan -> White -> Magenta)",
    fontSize: 24,
    fontWeight: 700,
    fontColor: "linearGradient(to bottom, #00ffff, #ffffff, #ff00ff)"
});

// 4. Radial Gradient: Circle
win.addText({
    id: "radial-circle",
    x: 20, y: startY + spacing * 3,
    text: "Radial: Circle (Yellow -> Red)",
    fontSize: 32,
    fontWeight: 900,
    fontColor: "radialGradient(circle, #ffff00, #ff0000)"
});

// 5. Radial Gradient: Ellipse
win.addText({
    id: "radial-ellipse",
    x: 20, y: startY + spacing * 4 + 10,
    text: "Radial: Ellipse (Cyan -> Transparent)",
    fontSize: 32,
    fontWeight: 900,
    width: 500,
    height: 100,
    fontColor: "radialGradient(ellipse, rgba(0,255,255,1), rgba(0,255,255,0))"
});

// 6. Gradient with Shadow
win.addText({
    id: "gradient-shadow",
    x: 20, y: startY + spacing * 5 + 40,
    text: "Gradient + Shadow",
    fontSize: 48,
    fontWeight: 900,
    fontColor: "linearGradient(to right, #00f, #f0f)",
    fontShadow: "2px 2px 4px rgba(0,0,0,200)"
});

// 7. Letter Spacing (Positive)
win.addText({
    id: "spacing-pos",
    x: 20, y: startY + spacing * 6 + 100,
    text: "Letter Spacing: 10px",
    fontSize: 24,
    fontWeight: 700,
    fontColor: "#fff",
    letterSpacing: 10,
    onLeftMouseUp: function() {
        console.log("Letter Spacing: 10px (Wider) clicked");
    }
});

// 8. Letter Spacing (Negative)
win.addText({
    id: "spacing-neg",
    x: 20, y: startY + spacing * 7 + 100,
    text: "Letter Spacing: -2px (Tighter)",
    fontSize: 24,
    fontWeight: 700,
    fontColor: "#fff",
    letterSpacing: -2,
    onLeftMouseUp: function() {
        console.log("Letter Spacing: -2px (Tighter) clicked");
    }
});

console.log("Gradient and spacing test widget created.");
