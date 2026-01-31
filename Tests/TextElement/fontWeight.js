// Font Weight Test - Demonstrating all weight values

var weights = [
    { value: 100, name: "Thin" },
    { value: 200, name: "ExtraLight" },
    { value: 300, name: "Light" },
    { value: 400, name: "Normal" },
    { value: 500, name: "Medium" },
    { value: 600, name: "SemiBold" },
    { value: 700, name: "Bold" },
    { value: 800, name: "ExtraBold" },
    { value: 900, name: "Black" }
];

var startY = 20;
var spacing = 50;

// Test numeric values
for (var i = 0; i < weights.length; i++) {
    var weight = weights[i];
    win.addText({
        id: "weight-" + weight.value,
        text: weight.value + " - " + weight.name,
        x: 20,
        y: startY + (i * spacing),
        fontSize: 32,
        fontColor: "#fff",
        fontWeight: weight.value
    });
}

// Test string aliases on the right side
var stringWeights = [
    "thin",
    "extralight",
    "light",
    "normal",
    "medium",
    "semibold",
    "bold",
    "extrabold",
    "black"
];

for (var j = 0; j < stringWeights.length; j++) {
    var weightStr = stringWeights[j];
    win.addText({
        id: "weight-string-" + weightStr,
        text: '"' + weightStr + '"',
        x: 300,
        y: startY + (j * spacing),
        fontSize: 32,
        fontColor: "#0ff",
        fontWeight: weightStr
    });
}
