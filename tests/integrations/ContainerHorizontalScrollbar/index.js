import { app, widgetWindow } from "novadesk";

console.log("=== ContainerHorizontalScrollbar Integration (Latest API) ===");

new widgetWindow({
  id: "ContainerHorizontalScrollbarWindow",
  x: 180,
  y: 150,
  width: 760,
  height: 360,
  backgroundColor: "rgba(18,20,28,0.96)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
