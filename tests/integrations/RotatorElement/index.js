import { app, widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== RotatorElement Integration ===");

const widget = new widgetWindow({
    id: "rotatorElementTest",
    width: 520,
    height: 360,
    backgroundColor: "rgba(12,16,22,0.96)",
    script: "./script.ui.js"
});

console.log("[PASS] widgetWindow created: rotatorElementTest");

let tick = 0;
const timer = setInterval(function () {
    tick += 1;
    const cpuRaw = system.cpu.usage();
    const cpu = Math.max(0, Math.min(100, Number(cpuRaw) || 0));

    ipcMain.send("rotator:tick", JSON.stringify({
        tick: tick,
        cpu: cpu
    }));
}, 500);

widget.on("close", function () {
    clearInterval(timer);
    app.exit();
});
