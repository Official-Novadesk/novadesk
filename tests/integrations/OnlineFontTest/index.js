import { app, widgetWindow } from "novadesk";

console.log("=== OnlineFontTest Integration ===");

// Keep a reference on globalThis to prevent garbage collection of the window.
globalThis.win = new widgetWindow({
  id: "OnlineFontTestWindow",
  x: 200,
  y: 150,
  width: 620,
  height: 480,
  backgroundColor: "rgba(18,22,32,0.97)",
  script: "./script.ui.js",
  show: true
});

globalThis.win.on("close", function () {
  app.exit();
});

console.log("[PASS] widgetWindow created: OnlineFontTestWindow");

// Give the async downloads up to 10 s to complete, then signal the UI to
// perform its post-download assertions.
setTimeout(() => {
  console.log("[INFO] Triggering font assertions check via IPC...");
  ipcMain.send("font:test:check");
}, 10000);

// Close app 2 s after the check triggers
setTimeout(() => {
  console.log("[INFO] OnlineFontTest exiting...");
  app.exit();
}, 12000);
