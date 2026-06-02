import { app, widgetWindow } from "novadesk";

console.log("=== AnimationFromTest Integration ===");

const win = new widgetWindow({
  id: "AnimationFromTestWindow",
  x: 200,
  y: 140,
  width: 480,
  height: 400,
  backgroundColor: "rgba(20,24,32,0.95)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: AnimationFromTestWindow");

setTimeout(() => {
  ipcMain.send("animation:from:mid");
}, 120);

setTimeout(() => {
  ipcMain.send("animation:from:end");
}, 500);

win.on("close", () => app.exit());
