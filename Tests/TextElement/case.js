startY = 20;
spacing = 70;

win.addText({
    id: "case-upper",
    x: 20, y: startY,
    text: "text case: upper",
    fontSize: 20,
    fontColor: "#fff",
    case: "upper"
});

win.addText({
    id: "case-lower",
    x: 20, y: startY + spacing,
    text: "TEXT CASE: LOWER",
    fontSize: 20,
    fontColor: "#fff",
    case: "lower"
});

win.addText({
    id: "case-capitalize",
    x: 20, y: startY + spacing * 2,
    text: "capitalize this sentence",
    fontSize: 20,
    fontColor: "#fff",
    case: "capitalize"
});

win.addText({
    id: "case-sentence",
    x: 20, y: startY + spacing * 3,
    text: "first letter only.",
    fontSize: 20,
    fontColor: "#fff",
    case: "sentence"
});

console.log("Gradient, spacing, underline, and case test widget created.");
