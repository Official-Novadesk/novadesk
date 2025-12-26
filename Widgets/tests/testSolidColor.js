
testSolidColorWidget = new widgetWindow({
    id: "testSolidColorWidget",
    width: 400,
    height: 300,
    backgroundcolor: "rgba(0, 0, 0, 1)",
});


// testSolidColorWidget.addText({
//     id: "testText",
//     text: "Simple Solid Color",
//     fontsize: 20,
//     x: 10,
//     y: 10,
//     width: 200,
//     height: 50,
//     align: "centercenter",
//     color: "rgba(0, 0, 0, 1)",
//     solidcolor: "rgba(22, 211, 236, 1)",
// });

testSolidColorWidget.addText({
    id: "testText2",
    text: "Solid Color and Solid Color 2",
    fontsize: 20,
    x: 10,
    y: 10,
    width: 300,
    height: 300,
    align: "centercenter",
    color: "rgba(0, 0, 0, 1)",
    solidcolor: "rgb(255, 0, 0)",      // Red
    solidcolor2: "rgb(0, 0, 255)",    // Blue
    gradientangle: 270,
    // solidcolorradius: 10,
});
