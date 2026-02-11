// RoundLineElement// Main test widget
var widget = new widgetWindow({
    id: "roundLineTest",
    width: 600,
    height: 400,
    backgroundColor: "rgba(30, 30, 40, 0.9)",
    script: "ui.js"
});

var val = 0.0;
setInterval(function() {
    val += 0.01;
    if (val > 1.0) val = 0;
    ipc.send("update-ring", val);
}, 50);
