import { app, widgetWindow } from "novadesk";

// Test: Gradient color support for all InputBox color properties
// Verifies that linearGradient/radialGradient strings work for:
//   fillColor, borderColor, borderFocusColor, fontColor,
//   placeholderColor, caretColor, selectionColor

console.log("=== InputBoxGradientTest ===");

new widgetWindow({
    id: "inputBoxGradientTest",
    width: 560,
    height: 580,
    backgroundColor: "linearGradient(160, #0f0f1a, #1a1a2e)",
    script: "script.ui.js",
}).on("close", function () { app.exit(); });
