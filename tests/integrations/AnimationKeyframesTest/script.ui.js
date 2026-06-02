function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function parseRgba(str) {
  const m = String(str).match(/rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)/i);
  if (!m) return null;
  return { r: Number(m[1]), g: Number(m[2]), b: Number(m[3]) };
}

function expectColorDominant(name, actual, channel, min) {
  const c = parseRgba(actual);
  if (!c) {
    fail(name, "could not parse color: " + actual);
    return;
  }
  const dominant = Math.max(c.r, c.g, c.b);
  const value = c[channel];
  if (value === dominant && value >= min) {
    pass(name, actual + " dominant=" + channel);
  } else {
    fail(name, actual + " expected dominant " + channel + " >= " + min);
  }
}

ui.beginUpdate();

ui.addText({
  id: "title",
  x: 20,
  y: 14,
  text: "RGB keyframes (fontColor)",
  fontSize: 13,
  fontColor: "rgba(200,200,200,1)"
});

ui.addText({
  id: "rgbText",
  x: 120,
  y: 110,
  width: 200,
  height: 80,
  text: "RGB",
  fontSize: 56,
  fontWeight: 700,
  fontColor: "rgba(255,255,255,1)",
  textAlign: "centercenter"
});

ui.endUpdate();

ui.animate({
  id: "rgbText",
  duration: 1000,
  easing: "linear",
  iterationCount: "infinite",
  keyframes: {
    "0%":   { fontColor: "rgba(255,50,50,1)" },    // red
    "16%":  { fontColor: "rgba(255,165,0,1)" },    // orange
    "32%":  { fontColor: "rgba(255,255,50,1)" },   // yellow
    "48%":  { fontColor: "rgba(50,255,90,1)" },    // green
    "64%":  { fontColor: "rgba(70,130,255,1)" },   // blue
    "80%":  { fontColor: "rgba(138,43,226,1)" },   // violet
    "90%":  { fontColor: "rgba(255,20,147,1)" },   // pink
    "100%": { fontColor: "rgba(255,50,50,1)" }     // back to red
  }
});

expectColorDominant("rgb start", ui.getElementProperty("rgbText", "fontColor"), "r", 200);

ipcRenderer.on("animation:keyframes:green", () => {
  expectColorDominant("rgb green phase", ui.getElementProperty("rgbText", "fontColor"), "g", 200);
});

ipcRenderer.on("animation:keyframes:blue", () => {
  expectColorDominant("rgb blue phase", ui.getElementProperty("rgbText", "fontColor"), "b", 200);
});

ipcRenderer.on("animation:keyframes:cycle", () => {
  expectColorDominant("rgb cycle red again", ui.getElementProperty("rgbText", "fontColor"), "r", 180);
  pass("AnimationKeyframesTest completed");
});
