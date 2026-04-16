import { app, widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== BitmapElement CPU Integration ===");

const widget = new widgetWindow({
    id: "bitmapCpuPercentTest",
    width: 520,
    height: 220,
    backgroundColor: "rgba(14,18,24,0.97)",
    script: "./script.ui.js"
});

console.log("[PASS] widgetWindow created: bitmapCpuPercentTest");

let tick = 0;
const timer = setInterval(function () {
    tick += 1;
    const usageRaw = system.cpu.usage();
    const usage = Math.max(0, Math.min(100, Math.round(usageRaw)));

    ipcMain.send("bitmap:cpu", JSON.stringify({
        tick: tick,
        usage: usage
    }));

    console.log("[BITMAP CPU] tick=" + tick + " usage=" + usage + "%");
}, 1000);
