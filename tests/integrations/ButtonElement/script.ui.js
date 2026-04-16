console.log("=== ButtonElement UI Script Started ===");
console.log("Hover and click the button to verify frame changes and buttonAction.");

ui.addText({
    id: "button-title",
    text: "Button Element Test",
    x: 20,
    y: 18,
    fontSize: 22,
    fontColor: "rgb(240,244,248)"
});

ui.addText({
    id: "button-subtitle",
    text: "Tests buttonImageName, buttonAction, transparent hit testing, and hover/down frames.",
    x: 20,
    y: 52,
    width: 360,
    fontSize: 13,
    fontColor: "rgba(214,223,233,0.9)"
});

ui.addButton({
    id: "play-button",
    x: 28,
    y: 96,
    buttonImageName: "./assets/button-strip.png",
    buttonAction: function (e) {
        console.log("buttonAction fired");
        if (e) {
            console.log("offset=" + e.__offsetX + "," + e.__offsetY);
        }

        ui.setElementProperties("button-status", {
            text: "Clicked at " + new Date().toLocaleTimeString()
        });
    },
    onMouseOver: function () {
        ui.setElementProperties("button-status", {
            text: "Hovering button"
        });
    },
    onMouseLeave: function () {
        ui.setElementProperties("button-status", {
            text: "Button idle"
        });
    },
    imageTint: "rgba(255,255,255,1)",
    imageAlpha: 255
});

ui.addText({
    id: "button-status",
    text: "Button idle",
    x: 110,
    y: 112,
    fontSize: 16,
    fontColor: "rgb(255,255,255)"
});

ui.addText({
    id: "button-note",
    text: "Only the visible circle should click. Transparent corners should be ignored.",
    x: 20,
    y: 176,
    width: 360,
    fontSize: 13,
    fontColor: "rgba(214,223,233,0.9)"
});

console.log("=== ButtonElement UI Script Ready ===");
