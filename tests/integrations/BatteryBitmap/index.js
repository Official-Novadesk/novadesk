import { app, widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== Battery Bitmap Integration Test ===");

const widget = new widgetWindow({
    id: "batteryBitmapTest",
    width: 300,
    height: 400,
    backgroundColor: "rgba(10, 15, 20, 0.95)",
    script: "./script.ui.js"
});

console.log("[PASS] widgetWindow created: batteryBitmapTest");

const timer = setInterval(function () {
    const power = system.power.getStatus();
    const percent = Math.round(power.percent || 0);
    const charging = !!power.acline;

    ipcMain.send("battery:update", JSON.stringify({
        percent: percent,
        charging: charging
    }));

    console.log("[BATTERY] %=" + percent + " charging=" + charging);
}, 1000);
