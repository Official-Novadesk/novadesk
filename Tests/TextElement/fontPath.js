// Custom Font Path Test - Ubuntu Family
// Loading fonts from: D:/Novadesk-Project/Ubuntu

var fontDir = "../../../Ubuntu";

var ubuntuWeights = [
    { weight: 300, name: "Light" },
    { weight: 400, name: "Regular" },
    { weight: 500, name: "Medium" },
    { weight: 700, name: "Bold" }
];

var startY = 20;
var spacing = 80;

win.addText({
    id: "title",
    text: "Custom Font Path: Ubuntu",
    x: 20,
    y: startY,
    fontSize: 24,
    fontColor: "#fcaf3e", // Ubuntu orange-ish
    fontWeight: 700,
    fontPath: fontDir,
    fontFace: "Ubuntu"
});

startY += 60;

for (var i = 0; i < ubuntuWeights.length; i++) {
    var item = ubuntuWeights[i];
    
    // Test Regular version
    win.addText({
        id: "ubuntu-" + item.weight,
        text: "Ubuntu " + item.name + " (" + item.weight + ")",
        x: 20,
        y: startY + (i * spacing),
        fontSize: 32,
        fontColor: "#fff",
        fontWeight: item.weight,
        fontPath: fontDir,
        fontFace: "Ubuntu"
    });

    // Test Italic version
    win.addText({
        id: "ubuntu-italic-" + item.weight,
        text: "Ubuntu " + item.name + " Italic",
        x: 20,
        y: startY + (i * spacing) + 35,
        fontSize: 24,
        fontColor: "#ccc",
        fontWeight: item.weight,
        fontStyle: "italic",
        fontPath: fontDir,
        fontFace: "Ubuntu"
    });
}
