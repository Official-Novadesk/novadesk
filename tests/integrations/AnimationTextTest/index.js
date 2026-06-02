import { app, widgetWindow } from "novadesk";

console.log("=== AnimationTextTest Integration ===");

const win = new widgetWindow({
  id: "AnimationTextTestWindow",
  x: 180,
  y: 120,
  width: 420,
  height: 320,
  backgroundColor: "rgba(20,24,32,0.95)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: AnimationTextTestWindow");

setTimeout(() => {
  ipcMain.send("animation:text:mid");
}, 200);

setTimeout(() => {
  ipcMain.send("animation:text:end");
}, 550);

win.on("close", () => app.exit());
