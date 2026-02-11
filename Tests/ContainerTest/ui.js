// Container demo test

// Container element (any element can be a container)
win.addShape({
    id: "containerBox",
    type: "rectangle",
    x: 40, y: 40,
    width: 220, height: 140,
    radius: 10,
    fillColor: "rgba(255, 235, 59, 0.3)",
    strokeColor: "rgba(255, 193, 7, 0)",
    strokeWidth: 2
});

// Contained text (position relative to container)
win.addText({
    id: "containedText",
    text: "Inside container",
    x: 10, y: 10,
    fontColor: "#111111",
    fontSize: 14,
    container: "containerBox"
});

// Contained image (clipped if outside bounds)
win.addImage({
    id: "containedImage",
    x: 160, y: 90,
    width: 80, height: 80,
    path: "../assets/pic.png",
    preserveAspectRatio: "preserve",
    container: "containerBox"
});

// Another container to show nesting
win.addShape({
    id: "innerContainer",
    type: "rectangle",
    x: 20, y: 50,
    width: 120, height: 70,
    radius: 6,
    fillColor: "#e0f7fa",
    strokeColor: "#006064",
    strokeWidth: 2,
    container: "containerBox"
});

// Nested contained element
win.addText({
    id: "nestedText",
    text: "Nested",
    x: 8, y: 8,
    fontColor: "#006064",
    fontSize: 12,
    container: "innerContainer"
});

// Clickable container area to toggle visibility
var hidden = false;
win.addShape({
    id: "toggleButton",
    type: "rectangle",
    x: 300, y: 60,
    width: 160, height: 60,
    radius: 10,
    fillColor: "#eeeeee",
    strokeColor: "#333333",
    strokeWidth: 2,
    onLeftMouseUp: function () {
        hidden = !hidden;
        win.setElementProperties("containerBox", { show: !hidden });
        console.log("Container show:", !hidden);
    }
});

win.addText({
    id: "toggleLabel",
    text: "Click to hide/show\ncontainer",
    x: 312, y: 70,
    fontColor: "#111111",
    fontSize: 12
});



win.addText({
    id: "Label",
    text: "Click",
    x: 312, y: 150,
    fontColor: "#111111",
    fontSize: 55
});

win.addImage({
    id: "containedImage5",
    x: 1, y: 1,
    // width: 0, height: 80,
    path: "../assets/pic.png",
    preserveAspectRatio: "preserve",
    // container: "Label"
});
