import { app, widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== LineElement Integration ===");

const widget = new widgetWindow({
    id: "lineElementTest",
    width: 1100,
    height: 1120,
    backgroundColor: "rgba(28, 30, 36, 0.96)",
    script: "./script.ui.js"
});

let phase = 0;
const timer = setInterval(function () {
    phase += 1;
    const cpu = system.cpu.usage();
    const totalMemory = system.memory.totalBytes();
    const usedMemory = system.memory.usedBytes();
    const memory = (totalMemory > 0)
        ? ((usedMemory / totalMemory) * 100.0)
        : system.memory.usagePercent();
    const netIn = system.network.rxSpeed();
    const netOut = system.network.txSpeed();
    // Send as JSON string for maximum compatibility with the ui bridge.
    ipcMain.send("line:tick", JSON.stringify({
        phase: phase,
        cpu: cpu,
        memory: memory,
        netIn: netIn,
        netOut: netOut
    }));
    console.log("[LINE LIVE] t=" + phase + " cpu=" + cpu + "% memory=" + memory + "% netIn=" + netIn + " netOut=" + netOut);
}, 1000);

// widget.on("close", function () {
//     clearInterval(timer);
//     app.exit();
// });
