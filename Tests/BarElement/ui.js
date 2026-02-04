// BarElement Test Suite
// Comprehensive test of all BarElement features and options

// === SECTION 1: BASIC BAR PROPERTIES ===

// Test 4: Value variations (0.0 to 1.0)
win.addBar({
    id: "value-0",
    x: 20,
    y: 70,
    width: 200,
    height: 20,
    value: 0.0,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-value-0",
    text: "Value: 0.0 (Empty)",
    x: 230,
    y: 70,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addBar({
    id: "value-0.25",
    x: 20,
    y: 100,
    width: 200,
    height: 20,
    value: 0.25,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-value-0.25",
    text: "Value: 0.25 (25%)",
    x: 230,
    y: 100,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addBar({
    id: "value-0.5",
    x: 20,
    y: 130,
    width: 200,
    height: 20,
    value: 0.5,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-value-0.5",
    text: "Value: 0.5 (50%)",
    x: 230,
    y: 130,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addBar({
    id: "value-0.75",
    x: 20,
    y: 160,
    width: 200,
    height: 20,
    value: 0.75,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-value-0.75",
    text: "Value: 0.75 (75%)",
    x: 230,
    y: 160,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addBar({
    id: "value-1.0",
    x: 20,
    y: 190,
    width: 200,
    height: 20,
    value: 1.0,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-value-1.0",
    text: "Value: 1.0 (Full)",
    x: 230,
    y: 190,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 2: ORIENTATION ===
// Test 4: Horizontal orientation (default)
win.addBar({
    id: "horizontal",
    x: 400,
    y: 70,
    width: 200,
    height: 25,
    value: 0.6,
    orientation: "horizontal",
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-horizontal",
    text: "Horizontal (default)",
    x: 400,
    y: 100,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 5: Vertical orientation
win.addBar({
    id: "vertical",
    x: 650,
    y: 70,
    width: 25,
    height: 200,
    value: 0.6,
    orientation: "vertical",
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-vertical",
    text: "Vertical",
    x: 680,
    y: 160,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 3: BAR COLORS ===
// Test 6: Simple bar color
win.addBar({
    id: "color-red",
    x: 20,
    y: 250,
    width: 200,
    height: 25,
    value: 0.7,
    barColor: "rgb(255, 0, 0)"
});
win.addText({
    id: "label-color-red",
    text: "Red Bar",
    x: 230,
    y: 255,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 7: Gradient bar color
win.addBar({
    id: "color-gradient",
    x: 20,
    y: 290,
    width: 200,
    height: 25,
    value: 0.7,
    barColor: "rgb(255, 0, 0)",
    barColor2: "rgb(0, 0, 255)"
});
win.addText({
    id: "label-color-gradient",
    text: "Red to Blue Gradient",
    x: 230,
    y: 295,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 8: Gradient with angle
win.addBar({
    id: "color-gradient-angle",
    x: 20,
    y: 330,
    width: 200,
    height: 25,
    value: 0.7,
    barColor: "rgb(255, 0, 0)",
    barColor2: "rgb(0, 0, 255)",
    barGradientAngle: 45
});
win.addText({
    id: "label-color-gradient-angle",
    text: "Gradient at 45°",
    x: 230,
    y: 335,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 9: Alpha transparency
win.addBar({
    id: "alpha-transparent",
    x: 20,
    y: 370,
    width: 200,
    height: 25,
    value: 0.7,
    barColor: "rgba(255, 255, 0, 0.5)",
    barColor2: "rgba(255, 0, 255, 0.5)"
});
win.addText({
    id: "label-alpha-transparent",
    text: "Transparent Gradient",
    x: 230,
    y: 375,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 10: Explicit alpha values
win.addBar({
    id: "alpha-explicit",
    x: 20,
    y: 410,
    width: 200,
    height: 25,
    value: 0.7,
    barColor: "rgb(0, 255, 0)",
    barColor2: "rgb(0, 0, 255)",
    barAlpha: 128,
    barAlpha2: 192
});
win.addText({
    id: "label-alpha-explicit",
    text: "Explicit Alpha (128, 192)",
    x: 230,
    y: 415,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 4: CORNER RADIUS ===
// Test 11: No corner radius (default)
win.addBar({
    id: "corner-none",
    x: 400,
    y: 250,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 165, 0)"
});
win.addText({
    id: "label-corner-none",
    text: "No Corner Radius",
    x: 400,
    y: 280,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 12: Small corner radius
win.addBar({
    id: "corner-small",
    x: 400,
    y: 320,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 165, 0)",
    barCornerRadius: 5
});
win.addText({
    id: "label-corner-small",
    text: "Corner Radius: 5",
    x: 400,
    y: 350,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 13: Large corner radius
win.addBar({
    id: "corner-large",
    x: 400,
    y: 390,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 165, 0)",
    barCornerRadius: 12
});
win.addText({
    id: "label-corner-large",
    text: "Corner Radius: 12",
    x: 400,
    y: 420,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 14: Full corner radius (circular)
win.addBar({
    id: "corner-full",
    x: 400,
    y: 460,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 165, 0)",
    barCornerRadius: 12.5
});
win.addText({
    id: "label-corner-full",
    text: "Full Corner Radius",
    x: 400,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 5: BACKGROUND AND SOLID COLOR ===
// Test 15: Solid color background
win.addBar({
    id: "bg-solid",
    x: 650,
    y: 250,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(0, 255, 0)",
    backgroundColor: "rgb(100, 100, 100)"
});
win.addText({
    id: "label-bg-solid",
    text: "Gray Background",
    x: 650,
    y: 280,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 16: Solid color with radius
win.addBar({
    id: "bg-radius",
    x: 650,
    y: 320,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(0, 255, 0)",
    backgroundColor: "rgb(100, 100, 100)",
    backgroundColorRadius: 8
});
win.addText({
    id: "label-bg-radius",
    text: "Rounded Background",
    x: 650,
    y: 350,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 17: Gradient background
win.addBar({
    id: "bg-gradient",
    x: 650,
    y: 390,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(0, 255, 0)",
    backgroundColor: "rgb(100, 100, 100)",
    solidColor2: "rgb(50, 50, 50)",
    backgroundColorRadius: 8
});
win.addText({
    id: "label-bg-gradient",
    text: "Gradient Background",
    x: 650,
    y: 420,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 6: TRANSFORMATIONS ===
// Test 18: Rotation
win.addBar({
    id: "rotate",
    x: 20,
    y: 500,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 0, 255)",
    rotate: 30
});
win.addText({
    id: "label-rotate",
    text: "Rotated 30°",
    x: 20,
    y: 530,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 19: No anti-aliasing
win.addBar({
    id: "no-aa",
    x: 250,
    y: 500,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(0, 255, 255)",
    antiAlias: false
});
win.addText({
    id: "label-no-aa",
    text: "No Anti-alias",
    x: 250,
    y: 530,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 7: BEVEL EFFECTS ===
// Test 20: Raised bevel
win.addBar({
    id: "bevel-raised",
    x: 500,
    y: 500,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 255, 0)",
    bevelType: "raised",
    bevelWidth: 2,
    bevelColor: "rgb(255, 255, 255)",
    bevelColor2: "rgb(0, 0, 0)"
});
win.addText({
    id: "label-bevel-raised",
    text: "Raised Bevel",
    x: 500,
    y: 530,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 21: Sunken bevel
win.addBar({
    id: "bevel-sunken",
    x: 500,
    y: 570,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 255, 0)",
    bevelType: "sunken",
    bevelWidth: 2,
    bevelColor: "rgb(0, 0, 0)",
    bevelColor2: "rgb(255, 255, 255)"
});
win.addText({
    id: "label-bevel-sunken",
    text: "Sunken Bevel",
    x: 500,
    y: 600,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 22: Emboss effect
win.addBar({
    id: "bevel-emboss",
    x: 500,
    y: 640,
    width: 200,
    height: 25,
    value: 0.6,
    barColor: "rgb(255, 255, 0)",
    bevelType: "emboss",
    bevelWidth: 3
});
win.addText({
    id: "label-bevel-emboss",
    text: "Emboss Effect",
    x: 500,
    y: 670,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 8: COMBINATION TESTS ===
// Test 23: Complex combination - all features
win.addBar({
    id: "complex-combination",
    x: 20,
    y: 600,
    width: 300,
    height: 30,
    value: 0.8,
    orientation: "horizontal",
    barColor: "rgb(255, 0, 0)",
    barColor2: "rgb(0, 0, 255)",
    barGradientAngle: 90,
    barCornerRadius: 15,
    backgroundColor: "rgb(50, 50, 50)",
    solidColor2: "rgb(30, 30, 30)",
    backgroundColorRadius: 15,
    bevelType: "raised",
    bevelWidth: 2
});
win.addText({
    id: "label-complex",
    text: "Complex Combination Test",
    x: 20,
    y: 640,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 24: Vertical with gradient and effects
win.addBar({
    id: "vertical-complex",
    x: 350,
    y: 550,
    width: 40,
    height: 150,
    value: 0.4,
    orientation: "vertical",
    barColor: "rgb(0, 255, 0)",
    barColor2: "rgb(0, 100, 0)",
    barGradientAngle: 0,
    barCornerRadius: 20,
    backgroundColor: "rgb(40, 40, 40)",
    backgroundColorRadius: 20
});
win.addText({
    id: "label-vertical-complex",
    text: "Vertical Complex",
    x: 400,
    y: 610,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});