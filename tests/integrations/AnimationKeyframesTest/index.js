import { app, widgetWindow } from "novadesk";

console.log("=== AnimationKeyframesTest Integration ===");

const win = new widgetWindow({
  id: "AnimationKeyframesTestWindow",
  x: 160,
  y: 100,
  width: 440,
  height: 300,
  backgroundColor: "rgba(12,14,20,0.98)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: AnimationKeyframesTestWindow");

setTimeout(() => {
  ipcMain.send("animation:keyframes:green");
}, 900);

setTimeout(() => {
  ipcMain.send("animation:keyframes:blue");
}, 1700);

setTimeout(() => {
  ipcMain.send("animation:keyframes:cycle");
}, 2600);

win.on("close", () => app.exit());
