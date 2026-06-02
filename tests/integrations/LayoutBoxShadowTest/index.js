import { app, widgetWindow } from "novadesk";

console.log("=== LayoutBoxShadowTest Integration ===");

const win = new widgetWindow({
  id: "LayoutBoxShadowTestWindow",
  x: 180,
  y: 120,
  width: 760,
  height: 460,
  backgroundColor: "#000000",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: LayoutBoxShadowTestWindow");

win.on("close", () => app.exit());
