import { widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== HistogramElement Integration ===");

const widget = new widgetWindow({
    id: "histogramElementTest",
    width: 980,
    height: 700,
    backgroundColor: "rgba(16, 20, 28, 0.96)",
    script: "./script.ui.js"
});

let tick = 0;
const timer = setInterval(function () {
    tick += 1;
    const cpu = system.cpu.usage();
    const memory = system.memory.usagePercent();
    ipcMain.send("histogram:tick", JSON.stringify({
        tick: tick,
        cpu: cpu,
        memory: memory
    }));
}, 500);

// widget.on("close", function () {
//     clearInterval(timer);
// });

