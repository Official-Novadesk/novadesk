import { app, widgetWindow } from "novadesk";

console.log("=== ImageElement Integration (Latest API) ===");
new widgetWindow({
  id: "imageElementTest",
  width: 900,
  height: 1200,
  backgroundColor: "rgba(40, 40, 50, 0.95)",
  script: "./script.ui.js"
});
