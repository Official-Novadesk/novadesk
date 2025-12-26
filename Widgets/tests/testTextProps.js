// testTextProps.js
const testWidget = new widgetWindow({
    id: "testTextProps",
    x: 100,
    y: 100,
    width: 400,
    height: 300,
    backgroundcolor: "rgba(50, 50, 50, 200)",
    draggable: true
});

// Test new fontface, fontcolor, and textalign properties
testWidget.addText({
    id: "text1",
    text: "Testing fontface: Courier New",
    x: 10,
    y: 10,
    width: 380,
    height: 40,
    fontface: "Courier New",
    fontsize: 18,
    fontcolor: "rgb(255, 255, 0)", // Yellow
    textalign: "leftcenter"
});

testWidget.addText({
    id: "text2",
    text: "Testing fontcolor: Red",
    x: 10,
    y: 60,
    width: 380,
    height: 40,
    fontface: "Arial",
    fontsize: 20,
    fontcolor: "rgba(255, 0, 0, 1)",
    textalign: "centercenter"
});

testWidget.addText({
    id: "text3",
    text: "Testing textalign: Right",
    x: 10,
    y: 110,
    width: 380,
    height: 40,
    fontface: "Segoe UI",
    fontsize: 16,
    fontcolor: "rgb(0, 255, 255)", // Cyan
    textalign: "rightcenter"
});

testWidget.addText({
    id: "text4",
    text: "Combined Props Test",
    x: 10,
    y: 160,
    width: 380,
    height: 100,
    fontface: "Times New Roman",
    fontsize: 24,
    fontcolor: "rgb(0, 255, 0)",
    textalign: "centercenter",
    solidcolor: "rgba(255, 255, 255, 50)",
    solidcolorradius: 10
});

console.log("Text properties test widget created.");
