import { widgetWindow } from "novadesk";

console.log("=== ButtonElement Integration ===");

new widgetWindow({
  id: "buttonElementTest",
  width: 420,
  height: 240,
  backgroundColor: "rgba(20, 24, 31, 0.96)",
  script: "./script.ui.js"
});
