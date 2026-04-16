console.log("=== Battery UI Started ===");

ui.beginUpdate();

ui.addText({
    id: "battery-title",
    x: 0,
    y: 20,
    width: 300,
    text: "Battery Status",
    fontSize: 20,
    fontColor: "white",
    textAlign: "centertop"
});

// The horizontal battery strip has 11 frames (0-100%).
// We set bitmapZeroFrame: true to reserve Frame 0 for exactly 0%.
ui.addBitmap({
    id: "battery-icon",
    x: 100,
    y: 0,
    width: 100,
    height: 400,
    value: 90,
    bitmapImageName: "./assets/battery-strip-11-vertical.png",
    bitmapFrames: 11,
    bitmapExtend: true, // Single image mode
    bitmapZeroFrame: true, // Reserve first frame for 0%
    bitmapOrientation: "vertical", // Slicing the row horizontally
    imageAlpha: 255
});

ui.addText({
    id: "battery-label",
    x: 0,
    y: 300,
    width: 300,
    text: "Waiting...",
    fontSize: 18,
    fontColor: "cyan",
    textAlign: "centertop"
});

ui.endUpdate();

// ipcRenderer.on("battery:update", function (event, raw) {
//     const data = JSON.parse(raw);
//     const percent = data.percent;
//     const charging = data.charging;

//     // Map percentage (0-100) to normalized value (0.0-1.0)
//     const val = percent / 100.0;

//     ui.setElementProperties("battery-icon", {
//         value: val
//     });

//     ui.setElementProperties("battery-label", {
//         text: percent + "%" + (charging ? " (Charging)" : ""),
//         fontColor: percent < 20 ? "red" : "cyan"
//     });
// });
