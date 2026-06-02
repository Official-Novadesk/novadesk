function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function expectEq(name, actual, expected) {
  if (actual === expected) pass(name, "expected=" + expected + " actual=" + actual);
  else fail(name, "expected=" + expected + " actual=" + actual);
}

function expectBetween(name, actual, min, max) {
  if (actual >= min && actual <= max) pass(name, "value=" + actual + " range=[" + min + "," + max + "]");
  else fail(name, "value=" + actual + " range=[" + min + "," + max + "]");
}

ui.beginUpdate();

ui.addText({
  id: "label",
  x: 20,
  y: 10,
  text: "iterationCount test",
  fontSize: 14,
  fontColor: "#ffffff"
});

ui.addShape({
  id: "loopBox",
  type: "rectangle",
  x: 40,
  y: 80,
  width: 40,
  height: 40,
  radius: 6,
  fillColor: "rgba(0,180,255,0.9)"
});

ui.endUpdate();

ui.animate({
  id: "loopBox",
  from: { x: 40 },
  to: { x: 200 },
  duration: 300,
  iterationCount: 2,
  easing: "linear"
});

expectEq("iteration start x", ui.getElementProperty("loopBox", "x"), 39);

ipcRenderer.on("animation:iteration:first-end", () => {
  expectEq("first iteration end x", ui.getElementProperty("loopBox", "x"), 199);
});

ipcRenderer.on("animation:iteration:second-mid", () => {
  expectBetween("second iteration mid x", ui.getElementProperty("loopBox", "x"), 90, 150);
});

ipcRenderer.on("animation:iteration:final", () => {
  expectEq("all iterations end x", ui.getElementProperty("loopBox", "x"), 199);
  pass("AnimationIterationTest completed");
});
