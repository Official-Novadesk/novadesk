// TextElement Test Suite
// Comprehensive test of all TextElement features and options

// === SECTION 1: BASIC TEXT PROPERTIES ===
// Test 1: Basic text with default settings
win.addText({
    id: "basic-text",
    text: "Basic Text",
    x: 20,
    y: 20,
    fontSize: 24,
    fontColor: "rgb(255,255,255)"
});

// Test 2: Font size variations
win.addText({
    id: "size-small",
    text: "Small Text (14px)",
    x: 20,
    y: 70,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addText({
    id: "size-medium",
    text: "Medium Text (24px)",
    x: 20,
    y: 100,
    fontSize: 24,
    fontColor: "rgb(255,255,255)"
});

win.addText({
    id: "size-large",
    text: "Large Text (36px)",
    x: 20,
    y: 140,
    fontSize: 36,
    fontColor: "rgb(255,255,255)"
});

// Test 3: Font color variations
win.addText({
    id: "color-red",
    text: "Red Text",
    x: 250,
    y: 70,
    fontSize: 18,
    fontColor: "rgb(255, 0, 0)"
});

win.addText({
    id: "color-green",
    text: "Green Text",
    x: 250,
    y: 100,
    fontSize: 18,
    fontColor: "rgb(0, 255, 0)"
});

win.addText({
    id: "color-blue",
    text: "Blue Text",
    x: 250,
    y: 130,
    fontSize: 18,
    fontColor: "rgb(0, 0, 255)"
});

// Test 4: Font weight - bold
win.addText({
    id: "bold-text",
    text: "Bold Text",
    x: 20,
    y: 200,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    fontWeight: "bold"
});

// Test 5: Font style - italic
win.addText({
    id: "italic-text",
    text: "Italic Text",
    x: 20,
    y: 230,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    fontStyle:"italic"
});

// Test 6: Bold and italic combined
win.addText({
    id: "bold-italic",
    text: "Bold Italic Text",
    x: 20,
    y: 260,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    fontWeight: "bold",
    fontStyle:"italic"
});

// Test 7: Font face variations
win.addText({
    id: "font-face-default",
    text: "Default Arial",
    x: 20,
    y: 290,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    fontFace: "Arial"
});

win.addText({
    id: "font-face-consolas",
    text: "Consolas Font",
    x: 20,
    y: 320,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    fontFace: "Consolas"
});

// === SECTION 2: TEXT ALIGNMENT ===
// Test 8: Left alignment (default)
win.addText({
    id: "align-lefttop",
    text: "Left Top\n2nd line",
    x: 250,
    y: 200,
    width: 200,
    height: 50,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    textAlign: "lefttop"
});

// Test 9: Center alignment
win.addText({
    id: "align-center",
    text: "Center Center\n2nd line",
    x: 250,
    y: 260,
    width: 200,
    height: 50,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    textAlign: "centercenter"
});

// Test 10: Right alignment
win.addText({
    id: "align-right",
    text: "Right Bottom\n2nd line",
    x: 250,
    y: 320,
    width: 200,
    height: 50,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    textAlign: "rightbottom"
});

// Test 11: Different alignment options
win.addText({
    id: "align-leftcenter",
    text: "Left Center",
    x: 250,
    y: 380,
    width: 200,
    height: 30,
    fontSize: 16,
    fontColor: "rgb(255,255,255)",
    textAlign: "leftcenter"
});

// === SECTION 3: CLIPPING OPTIONS ===
// Test 12: No clipping (default)
win.addText({
    id: "clip-none",
    text: "This is a long text that should wrap because it exceeds the width of the element",
    x: 500,
    y: 70,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)",
    clipString: "none"
});

// Test 13: Clip at boundaries
win.addText({
    id: "clip-boundary",
    text: "This text is clipped at element boundaries",
    x: 500,
    y: 110,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)",
    clipString: "clip"
});

// Test 14: Ellipsis clipping
win.addText({
    id: "clip-ellipsis",
    text: "This text ends with an ellipsis. john doe",
    x: 500,
    y: 150,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)",
    clipString: "ellipsis"
});

// Test 15: Wrap explicitly
win.addText({
    id: "clip-wrap",
    text: "This text should wrap explicitly to the next line when it reaches the edge of the text element.",
    x: 500,
    y: 190,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)",
    clipString: "wrap"
});

// === SECTION 4: BACKGROUND AND PADDING ===
// Test 16: Solid color background
win.addText({
    id: "bg-solid",
    text: "Text with Background",
    x: 20,
    y: 380,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(255, 0, 0)"
});

// Test 17: Gradient background
win.addText({
    id: "bg-gradient",
    text: "Gradient Background",
    x: 20,
    y: 420,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(255, 0, 0)",
    solidColor2: "rgb(0, 0, 255)",
    gradientAngle: 45
});

// Test 18: Rounded corners
win.addText({
    id: "bg-rounded",
    text: "Rounded Background",
    x: 20,
    y: 460,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(0, 255, 0)",
    backgroundColorRadius: 10
});

// Test 19: Padding
win.addText({
    id: "padding",
    text: "Text with Padding",
    x: 250,
    y: 450,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(255, 255, 0)",
    padding: 15
});

// Test 20: Padding array [horizontal, vertical]
win.addText({
    id: "padding-hv",
    text: "H/V Padding",
    x: 250,
    y: 500,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(255, 0, 255)",
    padding: [10, 20]
});

// Test 21: Padding array [left, top, right, bottom]
win.addText({
    id: "padding-ltrb",
    text: "L/T/R/B Padding",
    x: 250,
    y: 560,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    backgroundColor: "rgb(0, 255, 255)",
    padding: [5, 10, 15, 20]
});

// === SECTION 5: TRANSFORMATIONS ===
// Test 22: Rotation
win.addText({
    id: "rotate",
    text: "Rotated Text (45°)",
    x: 500,
    y: 250,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    rotate: 45
});

// Test 23: No anti-aliasing
win.addText({
    id: "no-aa",
    text: "No Anti-alias",
    x: 500,
    y: 300,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    antiAlias: false,
    rotate: 30
});

// === SECTION 6: BEVEL EFFECTS ===
// Test 24: Raised bevel
win.addText({
    id: "bevel-raised",
    text: "Raised Bevel",
    x: 20,
    y: 550,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    bevelType: "raised",
    bevelWidth: 2,
    bevelColor: "rgb(255, 255, 255)",
    bevelColor2: "rgb(0, 0, 0)"
});

// Test 25: Sunken bevel
win.addText({
    id: "bevel-sunken",
    text: "Sunken Bevel",
    x: 20,
    y: 590,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    bevelType: "sunken",
    bevelWidth: 2,
    bevelColor: "rgb(0, 0, 0)",
    bevelColor2: "rgb(255, 255, 255)"
});

// Test 26: Emboss effect
win.addText({
    id: "bevel-emboss",
    text: "Emboss Effect",
    x: 20,
    y: 630,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    bevelType: "emboss",
    bevelWidth: 3
});

// Test 27: Pillow effect
win.addText({
    id: "bevel-pillow",
    text: "Pillow Effect",
    x: 20,
    y: 670,
    fontSize: 20,
    fontColor: "rgb(255,255,255)",
    bevelType: "pillow",
    bevelWidth: 3
});

// === SECTION 7: MULTILINE TEXT AND WRAPPING ===
// Test 28: Multiline text with \n
win.addText({
    id: "multiline",
    text: "Line 1\nLine 2\nLine 3\nLine 4",
    x: 500,
    y: 350,
    fontSize: 16,
    fontColor: "rgb(255,255,255)"
});

// Test 29: Auto-wrapping
win.addText({
    id: "wrap-auto",
    text: "This is a long text that should automatically wrap to the next line when it reaches the edge of the text element.",
    x: 500,
    y: 450,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 30: Word wrapping
win.addText({
    id: "wrap-word",
    text: "This text demonstrates word wrapping behavior with longer words like supercalifragilisticexpialidocious.",
    x: 500,
    y: 550,
    width: 200,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 8: SPECIAL CHARACTERS AND FORMATTING ===
// Test 31: Unicode characters
win.addText({
    id: "unicode",
    text: "Unicode: α β γ δ ε\nSymbols: ♥ ♦ ♣ ♠",
    x: 500,
    y: 650,
    fontSize: 16,
    fontColor: "rgb(255,255,255)"
});

// Test 32: Mixed formatting
win.addText({
    id: "mixed-format",
    text: "Mixed: Bold Italic",
    x: 500,
    y: 720,
    fontSize: 18,
    fontColor: "rgb(255,255,255)",
    fontWeight: "bold",
    fontStyle:"italic"
});

// Test 33: Long text with scrolling (if supported)
win.addText({
    id: "long-text",
    text: "This is a very long text that demonstrates how the text element handles long content. It should wrap appropriately based on the width setting.",
    x: 500,
    y: 770,
    width: 250,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});