import { app, widgetWindow } from "novadesk";

console.log("=== LayoutApiTest Integration ===");

const win = new widgetWindow({
  id: "LayoutApiTestWindow",
  x: 180,
  y: 140,
  // width: 520,
  // height: 360,
  backgroundColor: "rgba(24,28,36,0.95)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: LayoutApiTestWindow");

win.on("close", () => app.exit());
