// --- Example 1: Neon Glow ---
win.addText({
    id: "neon-glow",
    text: "Neon Glow",
    x: 20,
    y: 20,
    fontSize: 50,
    fontColor: "#0ff",
    fontShadow: [
        { blur: 5, color: "#0ff" },
        { blur: 10, color: "#0ff" },
        { blur: 20, color: "#0ff" }
    ]
});

// --- Example 2: 3D Extrusion ---
win.addText({
    id: "3d-text",
    text: "3D Extruded",
    x: 20,
    y: 100,
    fontSize: 50,
    fontColor: "#fff",
    fontShadow: [
        { x: 1, y: 1, color: "#888" },
        { x: 2, y: 2, color: "#777" },
        { x: 3, y: 3, color: "#666" },
        { x: 4, y: 4, color: "#555" }
    ]
});

// --- Example 3: Retro Outline (Sticker) ---
win.addText({
    id: "sticker-text",
    text: "Retro Sticker",
    x: 20,
    y: 180,
    fontSize: 50,
    fontColor: "#fff",
    fontShadow: [
        { x: -3, y: -3, color: "#ff00ff" },
        { x: 3, y: -3, color: "#ff00ff" },
        { x: -3, y: 3, color: "#ff00ff" },
        { x: 3, y: 3, color: "#ff00ff" }
    ]
});

// --- Example 4: Soft Drop Shadow ---
win.addText({
    id: "soft-shadow",
    text: "Soft Shadow",
    x: 20,
    y: 260,
    fontSize: 50,
    fontColor: "#fff",
    fontShadow: { x: 5, y: 5, blur: 10, color: "rgba(0,0,0,0.7)" }
});

// --- Example 5: Multi-Color Aura ---
win.addText({
    id: "aura-text",
    text: "Aura Glow",
    x: 20,
    y: 340,
    fontSize: 50,
    fontColor: "#fff",
    fontShadow: [
        { x: -6, y: -6, blur: 12, color: "#f00" },
        { x: 6, y: -6, blur: 12, color: "#0f0" },
        { x: 0, y: 6, blur: 15, color: "#00f" }
    ]
});