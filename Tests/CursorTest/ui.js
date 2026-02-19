console.log("=== Cursor Test Started ===");

function logMouseAliases(name, e) {
    if (!e) {
        console.log(name + " clicked (no event payload)");
        return;
    }

    console.log(
        name +
        " clicked: __offsetX=" + e.__offsetX +
        ", __offsetY=" + e.__offsetY +
        ", __offsetXPercent=" + e.__offsetXPercent +
        ", __offsetYPercent=" + e.__offsetYPercent +
        ", __clientX=" + e.__clientX +
        ", __clientY=" + e.__clientY
    );
}

win.addText({
    id: "default_hand",
    x: 16,
    y: 18,
    text: "Default hand cursor",
    fontSize: 14,
    fontColor: "rgb(255,255,255)",
    onLeftMouseUp: function (e) { logMouseAliases("default_hand", e); }
});

win.addText({
    id: "cursor_off",
    x: 16,
    y: 56,
    text: "Cursor disabled",
    fontSize: 14,
    fontColor: "rgb(255,220,120)",
    mouseEventCursor: false,
    onLeftMouseUp: function (e) { logMouseAliases("cursor_off", e); }
});

win.addText({
    id: "busy_cursor",
    x: 16,
    y: 94,
    text: "Built-in busy cursor",
    fontSize: 14,
    fontColor: "rgb(120,220,255)",
    mouseEventCursorName: "busy",
    onLeftMouseUp: function (e) { logMouseAliases("busy_cursor", e); }
});

win.addText({
    id: "custom_ani_cursor",
    x: 16,
    y: 132,
    text: "Custom ANI cursor",
    fontSize: 14,
    fontColor: "rgb(180,255,180)",
    cursorsDir: "../assets",
    mouseEventCursorName: "cur1164",
    onLeftMouseUp: function (e) { logMouseAliases("custom_ani_cursor", e); }
});

win.addText({
    id: "custom_cur_cursor",
    x: 220,
    y: 132,
    text: "Custom CUR cursor",
    fontSize: 14,
    fontColor: "rgb(255,200,200)",
    cursorsDir: "../assets",
    mouseEventCursorName: "1",
    onLeftMouseUp: function (e) { logMouseAliases("custom_cur_cursor", e); }
});

console.log("default_hand.mouseEventCursor=" + win.getElementProperty("default_hand", "mouseEventCursor"));
console.log("cursor_off.mouseEventCursor=" + win.getElementProperty("cursor_off", "mouseEventCursor"));
console.log("busy_cursor.mouseEventCursorName=" + win.getElementProperty("busy_cursor", "mouseEventCursorName"));
console.log("custom_ani_cursor.cursorsDir=" + win.getElementProperty("custom_ani_cursor", "cursorsDir"));
console.log("custom_cur_cursor.mouseEventCursorName=" + win.getElementProperty("custom_cur_cursor", "mouseEventCursorName"));
console.log("=== Cursor Test Completed ===");
