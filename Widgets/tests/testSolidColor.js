
testSolidColorWidget = new widgetWindow({
    id: "testSolidColorWidget",
    width: 400,
    height: 300,
    backgroundcolor: "rgba(30, 30, 40, 0.9)",
});


testSolidColorWidget.addText({
    id: "testText",
    text: "Simple Solid Color",
    fontsize: 20,
    x: 10,
    y: 10,
    width: 200,
    height: 50,
    align: "centercenter",
    color: "rgba(0, 0, 0, 1)",
    solidcolor: "rgba(22, 211, 236, 1)",
});

testSolidColorWidget.addText({
    id: "testText2",
    text: "Solid Color and Solid Color 2",
    fontsize: 20,
    x: 10,
    y: 70,
    width: 200,
    height: 50,
    align: "centercenter",
    color: "rgba(0, 0, 0, 1)",
    solidcolor: "rgba(22, 211, 236, 1)",
    solidcolor2: "rgba(79, 22, 236, 1)",
    gradientangle: 45,
});
