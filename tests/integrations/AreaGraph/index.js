import { widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== AreaGraph Integration Test Started ===");

const widget = new widgetWindow({
    id: "areaGraphTest",
    width: 600,
    height: 400,
    backgroundColor: "rgba(10, 15, 25, 0.95)",
    script: "./script.ui.js"
});

let tick = 0;
const timer = setInterval(function () {
    tick += 1;
    const cpu = system.cpu.usage();
    ipcMain.send("graph:tick", JSON.stringify({
        tick: tick,
        cpu: cpu
    }));
}, 1000);
