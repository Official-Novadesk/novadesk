// Combine shapes demo

try {
    if (!win) {
        throw new Error("combine.js: 'win' is not defined.");
    }


// Base shapes
win.addShape({
    id: "baseCircle",
    type: "ellipse",
    x: 40, y: 40,
    width: 140, height: 140,
    fillColor: "#4a90e2",
    strokeColor: "#1f3f66",
    strokeWidth: 3
});

win.addShape({
    id: "cutRect",
    type: "rectangle",
    x: 90, y: 20,
    width: 120, height: 120,
    fillColor: "#8bc34a",
    strokeColor: "#3b6a1d",
    strokeWidth: 3,
    rotate: 10
});

// Combined shape (xor)
win.addShape({
    id: "comboXor",
    type: "combine",
    base: "baseCircle",
    consume: true,
    ops: [
        { id: "cutRect", mode: "xor", consume: true }
    ],
    x: 0, y: 0,
    fillColor: "#ffcf33",
    strokeColor: "#333333",
    strokeWidth: 2
});

// Additional shapes for union/intersect
win.addShape({
    id: "leftCircle",
    type: "ellipse",
    x: 260, y: 60,
    width: 110, height: 110,
    fillColor: "#ef5350",
    strokeColor: "#7f1d1d",
    strokeWidth: 2
});

win.addShape({
    id: "rightCircle",
    type: "ellipse",
    x: 320, y: 60,
    width: 110, height: 110,
    fillColor: "#66bb6a",
    strokeColor: "#1b5e20",
    strokeWidth: 2
});

// Union (keep originals)
win.addShape({
    id: "comboUnion",
    type: "combine",
    base: "leftCircle",
    ops: [
        { id: "rightCircle", mode: "union", consume: false }
    ],
    fillColor: "#8e24aa",
    strokeColor: "#2e003e",
    strokeWidth: 2
});
} catch (e) {
    console.log("Combine demo error:", e && (e.stack || e.message || e));
}
