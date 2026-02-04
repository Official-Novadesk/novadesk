// var win = new widgetWindow({
//     id: "ShapeTestWindow",
//     x: 100,
//     y: 100,
//     width: 600,
//     height: 1000,
//     backgroundColor: "#000",
//     script: "ui.js"
// });

var win = new widgetWindow({
    id: "ShapeTestWindow",
    x: 100,
    y: 100,
    width: 600,
    height: 600,
    backgroundColor: "#ffffff",
    script: "shape_ui.js"
});

// Get the path to Novadesk AppData
const appData = app.getAppDataPath();
console.log("AppData Path: " + appData);