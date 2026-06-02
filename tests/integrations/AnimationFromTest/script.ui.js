function pass(name, details) {
  // console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  // console.log("[FAIL] " + name + (details ? " -> " + details : ""));
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
  text: "Animation from test",
  fontSize: 14,
  fontColor: "#ffffff"
});

ui.addShape({
  id: "fullFromBox",
  type: "rectangle",
  x: 300,
  y: 280,
  width: 80,
  height: 50,
  radius: 6,
  fillColor: "rgba(255,80,160,0.9)"
});

ui.addShape({
  id: "partialFromBox",
  type: "rectangle",
  x: 100,
  y: 120,
  width: 60,
  height: 40,
  radius: 6,
  fillColor: "rgba(80,200,255,0.9)"
});

ui.addShape({
  id: "rotateFromBox",
  type: "rectangle",
  x: 320,
  y: 80,
  width: 70,
  height: 40,
  radius: 6,
  fillColor: "rgba(160,120,255,0.9)"
});

ui.endUpdate();

ui.animate({
  id: "fullFromBox",
  from: { x: 40, y: 80, width: 30, height: 20 },
  to: { x: 300, y: 280, width: 80, height: 50 },
  duration: 400,
  easing: "linear"
});

expectEq("full from snap x", ui.getElementProperty("fullFromBox", "x"), 39);
expectEq("full from snap y", ui.getElementProperty("fullFromBox", "y"), 79);
expectEq("full from snap contentWidth", ui.getElementProperty("fullFromBox", "contentWidth"), 30);
expectEq("full from snap contentHeight", ui.getElementProperty("fullFromBox", "contentHeight"), 20);

ui.animate({
  id: "partialFromBox",
  from: { x: 20 },
  to: { x: 100, y: 120 },
  duration: 300,
  easing: "linear"
});

expectEq("partial from snap x", ui.getElementProperty("partialFromBox", "x"), 19);
expectEq("partial from keeps layout y", ui.getElementProperty("partialFromBox", "y"), 119);

ui.animate({
  id: "rotateFromBox",
  from: { rotate: -20 },
  to: { rotate: 25 },
  duration: 300,
  easing: "easeOutQuad"
});

expectBetween("rotate from snap", ui.getElementProperty("rotateFromBox", "rotate"), -20.5, -19.5);

ipcRenderer.on("animation:from:mid", () => {
  expectBetween("full from mid x", ui.getElementProperty("fullFromBox", "x"), 41, 299);
  expectBetween("partial from mid x", ui.getElementProperty("partialFromBox", "x"), 21, 99);
  expectBetween("rotate from mid", ui.getElementProperty("rotateFromBox", "rotate"), -19.0, 24.0);
});

ipcRenderer.on("animation:from:end", () => {
  expectEq("full from end x", ui.getElementProperty("fullFromBox", "x"), 299);
  expectEq("full from end y", ui.getElementProperty("fullFromBox", "y"), 279);
  expectEq("full from end width", ui.getElementProperty("fullFromBox", "contentWidth"), 80);
  expectEq("full from end height", ui.getElementProperty("fullFromBox", "contentHeight"), 50);

  expectEq("partial from end x", ui.getElementProperty("partialFromBox", "x"), 99);
  expectEq("partial from end y", ui.getElementProperty("partialFromBox", "y"), 119);

  expectBetween("rotate from end", ui.getElementProperty("rotateFromBox", "rotate"), 24.5, 25.5);

  pass("AnimationFromTest completed");
});
