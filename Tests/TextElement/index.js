// TextElement Test Widget
// Tests all TextElement properties and methods

// Main test widget
var widget = new widgetWindow({
    id: "textElementTest",
    width: 1000,
    height: 1200,
    backgroundColor: "rgba(40, 40, 50, 0.95)",
    script: "ui.js"
});

var widget = new widgetWindow({
    id: "fontEffect",
    width: 500,
    height: 500,
    backgroundColor: "rgba(0, 0, 0, 1)",
    script: "fontEffect.js"
});

var widget = new widgetWindow({
    id: "fontWeight",
    width: 500,
    height: 500,
    backgroundColor: "rgba(0, 0, 0, 1)",
    script: "fontWeight.js"
});

var widget = new widgetWindow({
    id: "fontPath",
    width: 500,
    height: 500,
    backgroundColor: "rgba(0, 0, 0, 1)",
    script: "fontPath.js"
});

var widget = new widgetWindow({
    id: "fontGradient",
    width: 800,
    height: 800,
    backgroundColor: "rgba(0, 0, 0, 1)",
    script: "fontGradient.js"
});

var widget = new widgetWindow({
    id: "case",
    width: 400,
    height: 400,
    backgroundColor: "rgba(0, 0, 0, 1)",
    script: "case.js"
});


app.enableDebugging(true);