import { app, widgetWindow } from "novadesk";

console.log("=== AnimationApiTest Integration ===");

const win = new widgetWindow({
  id: "AnimationApiTestWindow",
  x: 220,
  y: 160,
  width: 520,
  height: 520,
  backgroundColor: "rgba(20,24,32,0.95)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: AnimationApiTestWindow");

setTimeout(() => {
  ipcMain.send("animation:test:mid");
}, 120);

setTimeout(() => {
  ipcMain.send("animation:test:replace-start");
}, 180);

setTimeout(() => {
  ipcMain.send("animation:test:end");
}, 420);

setTimeout(() => {
  ipcMain.send("animation:test:extra-end");
}, 650);
