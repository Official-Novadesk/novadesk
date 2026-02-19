console.log("=== Mouse Event Aliases Test Started ===");
console.log("Click inside the test text to inspect mouse aliases.");

win.addText({
    id: "mouse_alias_target",
    x: 20,
    y: 30,
    width: 300,
    height: 60,
    text: "Click me: validates __offsetX/Y and % aliases",
    fontSize: 14,
    fontColor: "rgb(230,230,230)",
    solidColor: "rgba(70,120,180,0.35)",
    onLeftMouseUp: function (e) {
        if (!e) {
            console.log("FAIL: event payload is missing");
            return;
        }

        var hasFields =
            typeof e.__offsetX === "number" &&
            typeof e.__offsetY === "number" &&
            typeof e.__offsetXPercent === "number" &&
            typeof e.__offsetYPercent === "number";

        console.log("payload fields present: " + hasFields);
        console.log("__offsetX=" + e.__offsetX + ", __offsetY=" + e.__offsetY);
        console.log("__offsetXPercent=" + e.__offsetXPercent + ", __offsetYPercent=" + e.__offsetYPercent);
        console.log("__clientX=" + e.__clientX + ", __clientY=" + e.__clientY + ", __screenX=" + e.__screenX + ", __screenY=" + e.__screenY);
    }
});

console.log("=== Mouse Event Aliases Test Ready ===");
