import { app, widgetWindow } from "novadesk";

console.log("=== RoundSliderDragEvents Integration (Latest API) ===");

new widgetWindow({
  id: "RoundSliderDragEventsWindow",
  x: 260,
  y: 160,
  width: 520,
  height: 420,
  backgroundColor: "rgba(20,20,24,0.95)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
