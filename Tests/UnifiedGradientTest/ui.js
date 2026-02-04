// Test background with gradient
win.addText({
    id: "text1",
    text: "Unified Gradient Test",
    x: 20, y: 20,
    fontSize: 24,
    fontColor: "#fff"
});

// Bar with gradient
win.addBar({
    id: "bar1",
    x: 20, y: 60,
    width: 200, height: 20,
    value: 0.7,
    barColor: "linearGradient(0, #f00, #ff0)",
    backgroundColor: "#333", // Background of the bar
    backgroundColorRadius: 5
});

// RoundLine with gradient
win.addRoundLine({
    id: "roundline1",
    x: 250, y: 60,
    width: 100, height: 100,
    value: 0.75,
    radius: 40,
    thickness: 10,
    lineColor: "linearGradient(90, #00f, #0ff)",
    lineColorBg: "linearGradient(90, #222, #444)"
});

// Box with gradient background (via solidColor)
win.addImage({
    id: "image1",
    x: 20, y: 120,
    width: 100, height: 100,
    backgroundColor: "linearGradient(135, #800080, #ff00ff)",
    backgroundColorRadius: 10
});

win.addText({
    id: "text2",
    text: "All elements above use the primary color property for gradients.",
    x: 20, y: 250,
    fontSize: 14,
    fontColor: "#aaa"
});
