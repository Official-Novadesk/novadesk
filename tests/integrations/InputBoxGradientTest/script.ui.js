// InputBoxGradientTest - script.ui.js
// Exercises every gradient-capable color property on an InputBoxElement.
// Each input box is labeled so it is easy to visually verify the result.

ui.beginUpdate();

// ── Section header ────────────────────────────────────────────────────────────
ui.addText({
    id: "header",
    x: 20, y: 14,
    text: "InputBox Gradient Color Test",
    fontSize: 18,
    fontFace: "Segoe UI",
    fontColor: "linearGradient(90, #a78bfa, #60a5fa)",
    fontWeight: 700,
});

ui.addText({
    id: "subHeader",
    x: 20, y: 42,
    text: "Every color property below uses linearGradient or radialGradient.",
    fontSize: 11,
    fontFace: "Segoe UI",
    fontColor: "rgba(180, 180, 200, 1)",
});

// ── Helper: section label factory ─────────────────────────────────────────────
function addLabel(id, y, text) {
    ui.addText({
        id: id,
        x: 20, y: y,
        text: text,
        fontSize: 11,
        fontFace: "Segoe UI",
        fontColor: "rgba(140, 140, 170, 1)",
    });
}

// ── 1. fillColor gradient ─────────────────────────────────────────────────────
addLabel("lbl1", 70, "1. fillColor: linearGradient(135, #1e3a5f, #0d6efd)");
ui.addInputBox({
    id: "input_fillGrad",
    x: 20, y: 88,
    width: 520, height: 38,
    placeholder: "Click me — gradient fill background",
    fillColor: "linearGradient(135, #1e3a5f, #0d6efd)",
    fontColor: "#e8f0fe",
    placeholderColor: "rgba(180, 200, 240, 0.7)",
    caretColor: "#ffffff",
    borderWidth: 0,
    borderRadius: 8,
    padding: 10,
});

// ── 2. borderColor gradient ───────────────────────────────────────────────────
addLabel("lbl2", 140, "2. borderColor: linearGradient(90, #f43f5e, #fb923c)");
ui.addInputBox({
    id: "input_borderGrad",
    x: 20, y: 158,
    width: 520, height: 38,
    placeholder: "Gradient border (no fill)",
    fillColor: "rgba(25, 25, 35, 1)",
    fontColor: "#f8fafc",
    placeholderColor: "rgba(148, 163, 184, 1)",
    caretColor: "#fb923c",
    borderColor: "linearGradient(90, #f43f5e, #fb923c)",
    borderWidth: 2,
    borderRadius: 8,
    padding: 10,
});

// ── 3. borderFocusColor gradient ─────────────────────────────────────────────
addLabel("lbl3", 210, "3. borderFocusColor gradient — click to see it");
ui.addInputBox({
    id: "input_focusBorderGrad",
    x: 20, y: 228,
    width: 520, height: 38,
    placeholder: "Focus me to see gradient focus border",
    fillColor: "rgba(20, 20, 30, 1)",
    fontColor: "#f0fdf4",
    placeholderColor: "rgba(134, 239, 172, 0.6)",
    caretColor: "#4ade80",
    borderColor: "rgba(40, 60, 40, 1)",
    borderFocusColor: "linearGradient(90, #4ade80, #22d3ee)",
    borderWidth: 2,
    borderRadius: 8,
    padding: 10,
});

// ── 4. fontColor gradient ─────────────────────────────────────────────────────
addLabel("lbl4", 280, "4. fontColor: linearGradient(0, #fbbf24, #f472b6)");
ui.addInputBox({
    id: "input_fontGrad",
    x: 20, y: 298,
    width: 520, height: 38,
    text: "Gradient text colour — type more to see it!",
    fillColor: "rgba(18, 18, 28, 1)",
    fontColor: "linearGradient(0, #fbbf24, #f472b6)",
    caretColor: "#f472b6",
    selectionColor: "rgba(244, 114, 182, 0.25)",
    borderWidth: 0,
    borderRadius: 8,
    padding: 10,
});

// ── 5. placeholderColor gradient ──────────────────────────────────────────────
addLabel("lbl5", 350, "5. placeholderColor: radialGradient(circle, #818cf8, #c084fc)");
ui.addInputBox({
    id: "input_placeholderGrad",
    x: 20, y: 368,
    width: 520, height: 38,
    placeholder: "This placeholder uses a radial gradient colour",
    fillColor: "rgba(15, 15, 25, 1)",
    fontColor: "#e0e7ff",
    placeholderColor: "radialGradient(circle, #818cf8, #c084fc)",
    caretColor: "#818cf8",
    borderWidth: 0,
    borderRadius: 8,
    padding: 10,
});

// ── 6. selectionColor gradient ────────────────────────────────────────────────
addLabel("lbl6", 420, "6. selectionColor gradient — select text here");
ui.addInputBox({
    id: "input_selectionGrad",
    x: 20, y: 438,
    width: 520, height: 38,
    text: "Select this text to see the gradient selection highlight",
    fillColor: "rgba(16, 16, 24, 1)",
    fontColor: "#f1f5f9",
    caretColor: "#38bdf8",
    selectionColor: "linearGradient(90, rgba(56,189,248,0.45), rgba(168,85,247,0.45))",
    borderWidth: 0,
    borderRadius: 8,
    padding: 10,
});

// ── 7. all-gradient combo + dynamic set/get roundtrip ────────────────────────
addLabel("lbl7", 490, "7. All-gradient combo (fillColor + fontColor + borderFocusColor)");
ui.addInputBox({
    id: "input_allGrad",
    x: 20, y: 508,
    width: 520, height: 38,
    placeholder: "Combo: fill + font + focus-border are all gradients",
    fillColor: "linearGradient(120, #312e81, #1e1b4b)",
    fontColor: "linearGradient(90, #a78bfa, #818cf8)",
    placeholderColor: "rgba(165, 130, 250, 0.55)",
    caretColor: "#a78bfa",
    selectionColor: "rgba(139, 92, 246, 0.35)",
    borderFocusColor: "linearGradient(90, #7c3aed, #4f46e5)",
    borderWidth: 2,
    borderRadius: 8,
    padding: 10,
    onFocus: function (e) {
        // ── getElementProperty round-trip check ──────────────────────────────
        const fc  = ui.getElementProperty("input_allGrad", "fontColor");
        const fil = ui.getElementProperty("input_allGrad", "fillColor");
        const bfc = ui.getElementProperty("input_allGrad", "borderFocusColor");
        console.log("[GradientTest] getElementProperty round-trip on focus:");
        console.log("  fontColor       =", fc);
        console.log("  fillColor       =", fil);
        console.log("  borderFocusColor=", bfc);

        // ── dynamic setElementProperty with a gradient ────────────────────────
        ui.setElementProperty("input_allGrad", "borderFocusColor",
            "linearGradient(90, #f472b6, #fb7185)");
        console.log("  borderFocusColor overridden to pink gradient on focus.");
    },
    onBlur: function (e) {
        // Restore the original focus-border gradient on blur
        ui.setElementProperty("input_allGrad", "borderFocusColor",
            "linearGradient(90, #7c3aed, #4f46e5)");
        console.log("[GradientTest] borderFocusColor restored on blur.");
    },
});

ui.endUpdate();
