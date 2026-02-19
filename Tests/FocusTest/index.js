// Focus Test Widget
var widget = new widgetWindow({
    id: "focusTest",
    width: 400,
    height: 200,
    backgroundColor: "rgba(30, 30, 40, 0.9)",
    script: "ui.js"
});

// Event Listeners
widget.on("focus", function() {
    console.log("Widget Focused");
});

widget.on("unFocus", function() {
    console.log("Widget Unfocused");
});

widget.on("mouseDown", function(e) {
    console.log("mouseDown __clientX=" + e.__clientX + ", __clientY=" + e.__clientY);
});

widget.on("mouseUp", function(e) {
    console.log("mouseUp __clientX=" + e.__clientX + ", __clientY=" + e.__clientY);
});

widget.on("mouseMove", function(e) {
    console.log("mouseMove __clientX=" + e.__clientX + ", __clientY=" + e.__clientY);
});

widget.on("mouseOver", function(e) {
    console.log("mouseOver __clientX=" + e.__clientX + ", __clientY=" + e.__clientY);
});

widget.on("mouseLeave", function(e) {
    if (e) {
        console.log("mouseLeave __clientX=" + e.__clientX + ", __clientY=" + e.__clientY);
    } else {
        console.log("mouseLeave");
    }
});

setTimeout(function() {
    widget.setFocus();
    console.log("Widget Focused");
}, 2000);

setTimeout(function() {
    widget.unFocus();
    console.log("Widget Unfocused");
}, 4000);

var windowHandle = widget.getHandle();
console.log("Window Handle: " + windowHandle);

var internalPointer = widget.getInternalPointer();
console.log("Internal Pointer: " + internalPointer);

var title = widget.getTitle();
console.log("Title: " + title);
