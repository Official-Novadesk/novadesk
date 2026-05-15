import { app, widgetWindow } from "novadesk";

console.log("=== TextElement fontShadow Integration Test ===");

new widgetWindow({
  id: "fontShadowTest",
  width: 800,
  height: 600,
  backgroundColor: "rgba(30, 30, 35, 1)",
  script: "fontShadow.ui.js"
}).on("close", function () { 
    console.log("Test window closed.");
    app.exit(); 
});
