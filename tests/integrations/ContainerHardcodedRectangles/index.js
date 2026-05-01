import { app, widgetWindow } from "novadesk";

console.log("=== ContainerHardcodedRectangles Integration (Latest API) ===");

new widgetWindow({
  id: "ContainerHardcodedRectanglesWindow",
  x: 180,
  y: 170,
  width: 760,
  height: 360,
  backgroundColor: "rgba(18,20,28,0.96)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
