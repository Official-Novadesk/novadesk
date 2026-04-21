import { app, widgetWindow } from "novadesk";


const widget = new widgetWindow({
    id: "toolbar-test-widget",
    width: 200,
    height: 100,
    showInToolbar: true,
    backgroundColor: "#000000",
    toolbarIcon: "../assets/icon.ico",
    toolbarTitle: "Toolbar Integration Test",
    script: "./script.ui.js"
});

widget.on("close", () => console.log("Widget closing"));
widget.on("closed", () => console.log("Widget closed"));

function testWidgetMinimization() {
    widget.minimize();
   
}

function testWidgetRestoration() {
    widget.unMinimize();
    
}
 widget.on("minimize", () => console.log("minimized"));
 widget.on("unMinimize", () => console.log("restored"));
setTimeout(() => {
    testWidgetMinimization();
    // testWidgetRestoration();
}, 5000);

setTimeout(() => {
    // testWidgetUnMinimization();
    testWidgetRestoration();
}, 8000);