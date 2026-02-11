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