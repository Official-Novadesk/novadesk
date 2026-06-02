function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

ui.beginUpdate();

ui.addLayoutBox({
  id: "shadowSingle",
  x: 30,
  y: 40,
  width: 210,
  height: 120,
  backgroundColor: "#ffffff",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "#202020",
  boxShadow: {
    x: 0,
    y: 10,
    blur: 26,
    spread: -6,
    color: "rgba(0,0,0,0.25)",
    inset: false
  },
  children: [
    ui.text({ id: "s1t", x: 0, y: 0, text: "single shadow", fontSize: 18, fontColor: "#111111" })
  ]
});

ui.addLayoutBox({
  id: "shadowMulti",
  x: 280,
  y: 40,
  width: 210,
  height: 120,
  backgroundColor: "#ffffff",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "#202020",
  boxShadow: [
    { x: 0, y: 1, blur: 2, spread: 0, color: "rgba(0,0,0,0.12)", inset: false },
    { x: 0, y: 12, blur: 32, spread: -6, color: "rgba(0,0,0,0.28)", inset: false }
  ],
  children: [
    ui.text({ id: "s2t", x: 0, y: 0, text: "multi shadow", fontSize: 18, fontColor: "#111111" })
  ]
});

ui.addLayoutBox({
  id: "shadowInset",
  x: 530,
  y: 40,
  width: 210,
  height: 120,
  backgroundColor: "#ffffff",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "#202020",
  boxShadow: {
    x: 0,
    y: 3,
    blur: 8,
    spread: 0,
    color: "rgba(0,0,0,0.18)",
    inset: true
  },
  children: [
    ui.text({ id: "s3t", x: 0, y: 0, text: "inset shadow", fontSize: 18, fontColor: "#111111" })
  ]
});

ui.addLayoutBox({
  id: "shadowGlowLight",
  x: 30,
  y: 175,
  width: 210,
  height: 26,
  backgroundColor: "#2a6bff",
  borderRadius: 8,
  borderWidth: 0,
  boxShadow: [
    { x: 0, y: 0, blur: 8, spread: 1, color: "rgba(42,107,255,0.85)", inset: false },
    { x: 0, y: 0, blur: 18, spread: 4, color: "rgba(42,107,255,0.55)", inset: false },
    { x: 0, y: 0, blur: 34, spread: 8, color: "rgba(42,107,255,0.35)", inset: false }
  ],
  children: [
    ui.text({ id: "sGlowLabel", x: 8, y: 4, text: "glow (light bg)", fontSize: 13, fontWeight: 600, fontColor: "#ffffff" })
  ]
});

ui.addLayoutBox({
  id: "darkStage",
  x: 20,
  y: 210,
  width: 720,
  height: 220,
  backgroundColor: "#131722",
  borderRadius: 12,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.10)",
  children: [
    ui.text({ id: "darkTitle", x: 16, y: 10, text: "Dark Mode Shadow Cases", fontSize: 18, fontWeight: 700, fontColor: "#e9eefc" })
  ]
});

ui.addLayoutBox({
  id: "darkCardOuter",
  x: 45,
  y: 250,
  width: 205,
  height: 140,
  backgroundColor: "#1d2433",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.12)",
  boxShadow: [
    { x: 0, y: 0, blur: 0, spread: 1, color: "rgba(255,255,255,0.08)", inset: false },
    { x: 0, y: 16, blur: 34, spread: -10, color: "rgba(0,0,0,0.60)", inset: false }
  ],
  children: [
    ui.text({ id: "d1", x: 0, y: 0, text: "dark outer", fontSize: 17, fontColor: "#f6f9ff" })
  ]
});

ui.addLayoutBox({
  id: "darkCardInset",
  x: 275,
  y: 250,
  width: 205,
  height: 140,
  backgroundColor: "#1d2433",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.12)",
  boxShadow: [
    { x: 0, y: 1, blur: 0, spread: 1, color: "rgba(255,255,255,0.06)", inset: false },
    { x: 0, y: 3, blur: 9, spread: 0, color: "rgba(0,0,0,0.40)", inset: true }
  ],
  children: [
    ui.text({ id: "d2", x: 0, y: 0, text: "dark inset", fontSize: 17, fontColor: "#f6f9ff" })
  ]
});

ui.addLayoutBox({
  id: "darkCardNegativeSpread",
  x: 505,
  y: 250,
  width: 205,
  height: 140,
  backgroundColor: "#1d2433",
  borderRadius: 14,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.12)",
  boxShadow: {
    x: 0,
    y: 14,
    blur: 30,
    spread: -12,
    color: "rgba(0,0,0,0.72)",
    inset: false
  },
  children: [
    ui.text({ id: "d3", x: 0, y: 0, text: "negative spread", fontSize: 17, fontColor: "#f6f9ff" })
  ]
});

ui.addLayoutBox({
  id: "darkGlow",
  x: 275,
  y: 400,
  width: 205,
  height: 22,
  backgroundColor: "#22d3ee",
  borderRadius: 6,
  borderWidth: 0,
  boxShadow: [
    { x: 0, y: 0, blur: 6, spread: 1, color: "rgba(34,211,238,0.95)", inset: false },
    { x: 0, y: 0, blur: 14, spread: 4, color: "rgba(34,211,238,0.65)", inset: false },
    { x: 0, y: 0, blur: 26, spread: 8, color: "rgba(34,211,238,0.45)", inset: false }
  ],
  children: [
    ui.text({ id: "darkGlowLabel", x: 8, y: 2, text: "glow (dark bg)", fontSize: 12, fontWeight: 700, fontColor: "#052028" })
  ]
});

ui.endUpdate();

pass("LayoutBoxShadowTest completed");
