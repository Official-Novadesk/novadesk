ui.addText({
    id: "shadow-title",
    text: "Font Shadow Tests",
    x: 50,
    y: 30,
    fontSize: 40,
    fontWeight: 700,
    fontColor: "#FFFFFF"
});

// Single Shadow
ui.addText({
    id: "single-shadow",
    text: "Single Drop Shadow",
    x: 50,
    y: 100,
    fontSize: 30,
    fontColor: "#FFFFFF",
    fontShadow: { x: 4, y: 4, blur: 5, color: "rgba(0,0,0,0.8)" }
});

// Multi Shadow (Glow)
ui.addText({
    id: "multi-shadow-glow",
    text: "Neon Glow Effect",
    x: 50,
    y: 180,
    fontSize: 30,
    fontColor: "#00FFFF",
    fontShadow: [
        { blur: 5, color: "#00FFFF" },
        { blur: 10, color: "#00FFFF" },
        { blur: 20, color: "#00FFFF" }
    ]
});

// 3D Effect
ui.addText({
    id: "shadow-3d",
    text: "3D Extruded Text",
    x: 50,
    y: 260,
    fontSize: 40,
    fontColor: "#FFFFFF",
    fontShadow: [
        { x: 1, y: 1, color: "#AAAAAA" },
        { x: 2, y: 2, color: "#999999" },
        { x: 3, y: 3, color: "#888888" },
        { x: 4, y: 4, color: "#777777" },
        { x: 5, y: 5, color: "#666666" }
    ]
});

// Outline Effect
ui.addText({
    id: "shadow-outline",
    text: "Outline Shadow",
    x: 50,
    y: 360,
    fontSize: 40,
    fontColor: "#FFFFFF",
    fontShadow: [
        { x: -2, y: -2, color: "#FF0000" },
        { x: 2, y: -2, color: "#FF0000" },
        { x: -2, y: 2, color: "#FF0000" },
        { x: 2, y: 2, color: "#FF0000" }
    ]
});

// Dynamic Shadow Update Test
ui.addText({
    id: "dynamic-shadow",
    text: "I will change shadow",
    x: 50,
    y: 460,
    fontSize: 30,
    fontColor: "#FFFFFF"
});

setTimeout(() => {
    ui.setProperties("dynamic-shadow", {
        fontShadow: { x: 0, y: 0, blur: 15, color: "#FF00FF" }
    });
}, 2000);
