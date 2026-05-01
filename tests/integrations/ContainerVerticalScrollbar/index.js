import { app, widgetWindow } from "novadesk";

console.log("=== ContainerVerticalScrollbar Integration (Latest API) ===");

new widgetWindow({
  id: "ContainerVerticalScrollbarWindow",
  x: 220,
  y: 120,
  width: 560,
  height: 700,
  backgroundColor: "rgba(18,20,28,0.96)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
