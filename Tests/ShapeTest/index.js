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
    width: 700,
    height: 900,
    backgroundColor: "#ffffff",
    script: "combine.js"
});

// Get the path to Novadesk AppData
const appData = app.getAppDataPath();
console.log("AppData Path: " + appData);
