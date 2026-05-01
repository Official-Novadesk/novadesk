import { app, widgetWindow } from "novadesk";

console.log("=== ElementDragEvents Integration (Latest API) ===");

new widgetWindow({
  id: "ElementDragEventsWindow",
  x: 260,
  y: 220,
  width: 520,
  height: 220,
  backgroundColor: "rgba(20,20,24,0.95)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
