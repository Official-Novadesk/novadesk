import { app, widgetWindow } from "novadesk";

console.log("=== LayoutApiTest Integration ===");

const win = new widgetWindow({
  id: "LayoutApiTestWindow(Display)",
  x: 180,
  y: 140,
  width: 620,
  height: 1800,
  backgroundColor: "rgba(255, 255, 255, 0.95)",
  script: "./image.ui.js",
  show: true
});

