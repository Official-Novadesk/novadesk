import { app, widgetWindow } from "novadesk";

console.log("=== AnimationIterationTest Integration ===");

const win = new widgetWindow({
  id: "AnimationIterationTestWindow",
  x: 200,
  y: 140,
  width: 400,
  height: 280,
  backgroundColor: "rgba(20,24,32,0.95)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: AnimationIterationTestWindow");

setTimeout(() => {
  ipcMain.send("animation:iteration:first-end");
}, 320);

setTimeout(() => {
  ipcMain.send("animation:iteration:second-mid");
}, 480);

setTimeout(() => {
  ipcMain.send("animation:iteration:final");
}, 680);

win.on("close", () => app.exit());
