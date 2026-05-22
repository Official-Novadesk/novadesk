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

ui.addShape({
  id: "animBox",
  type: "rectangle",
  x: 20,
  y: 40,
  width: 80,
  height: 40,
  radius: 8,
  fillColor: "rgba(0,180,255,0.9)"
});

ui.addText({
  id: "label",
  x: 20,
  y: 10,
  text: "Animation API test",
  fontSize: 14,
  fontColor: "#ffffff"
});

ui.addShape({
  id: "animBox2",
  type: "rectangle",
  x: 40,
  y: 220,
  width: 60,
  height: 30,
  radius: 6,
  fillColor: "rgba(255,170,0,0.9)"
});

ui.addShape({
  id: "rotateOnly",
  type: "rectangle",
  x: 380,
  y: 60,
  width: 70,
  height: 40,
  radius: 6,
  fillColor: "rgba(160,120,255,0.9)"
});

const allEasings = [
  "linear",
  "easeInQuad", "easeOutQuad", "easeInOutQuad",
  "easeInCubic", "easeOutCubic", "easeInOutCubic",
  "easeInQuart", "easeOutQuart", "easeInOutQuart",
  "easeInQuint", "easeOutQuint", "easeInOutQuint",
  "easeInSine", "easeOutSine", "easeInOutSine",
  "easeInExpo", "easeOutExpo", "easeInOutExpo",
  "easeInCirc", "easeOutCirc", "easeInOutCirc",
  "easeInBack", "easeOutBack", "easeInOutBack",
  "easeInElastic", "easeOutElastic", "easeInOutElastic",
  "easeInBounce", "easeOutBounce", "easeInOutBounce"
];

for (let i = 0; i < allEasings.length; i += 1) {
  const id = "easeBox" + i;
  const y = 300 + Math.floor(i / 8) * 16;
  ui.addShape({
    id,
    type: "rectangle",
    x: 10,
    y,
    width: 10,
    height: 10,
    fillColor: "rgba(120,220,120,0.9)"
  });
}

ui.endUpdate();

for (let i = 0; i < allEasings.length; i += 1) {
  const id = "easeBox" + i;
  ui.animate({
    id,
    to: { x: 60 + (i % 8) * 50 },
    duration: 500,
    easing: allEasings[i]
  });
}

expectEq("initial x", ui.getElementProperty("animBox", "x"), 19);
expectEq("initial y", ui.getElementProperty("animBox", "y"), 39);
expectEq("initial contentWidth", ui.getElementProperty("animBox", "contentWidth"), 80);
expectEq("initial contentHeight", ui.getElementProperty("animBox", "contentHeight"), 40);

ui.animate({
  id: "animBox",
  to: {
    x: 260,
    y: 140,
    width: 180,
    height: 90,
    rotate: 15
  },
  duration: 1000,
  easing: "easeOutCubic"
});

ui.animate({
  id: "animBox2",
  to: {
    x: 300,
    y: 230,
    width: 120,
    height: 50
  },
  duration: 500,
  easing: "easeInOutCubic"
});

ui.animate({
  id: "rotateOnly",
  to: { rotate: 30 },
  duration: 300,
  easing: "easeOutQuad"
});

ipcRenderer.on("animation:test:mid", () => {
  const midX = ui.getElementProperty("animBox", "x");
  const midY = ui.getElementProperty("animBox", "y");
  const midW = ui.getElementProperty("animBox", "contentWidth");
  const midH = ui.getElementProperty("animBox", "contentHeight");

  expectBetween("mid x", midX, 21, 259);
  expectBetween("mid y", midY, 41, 139);
  expectBetween("mid contentWidth", midW, 81, 179);
  expectBetween("mid contentHeight", midH, 41, 89);
});

ipcRenderer.on("animation:test:end", () => {
  // At this point replacement has already started (see replace-start event), so
  // validate that animBox is in-flight toward replacement target.
  const endX = ui.getElementProperty("animBox", "x");
  const endY = ui.getElementProperty("animBox", "y");
  const endW = ui.getElementProperty("animBox", "contentWidth");
  const endH = ui.getElementProperty("animBox", "contentHeight");
  const endR = ui.getElementProperty("animBox", "rotate");
  expectBetween("post-replace x in-flight", endX, 110, 260);
  expectBetween("post-replace y in-flight", endY, 139, 180);
  expectBetween("post-replace contentWidth in-flight", endW, 135, 180);
  expectBetween("post-replace contentHeight in-flight", endH, 67, 90);
  expectBetween("post-replace rotate in-flight", endR, 5.0, 15.5);

  // second element should be near completion at this point
  const b2x = ui.getElementProperty("animBox2", "x");
  const b2w = ui.getElementProperty("animBox2", "contentWidth");
  expectBetween("animBox2 in-flight x", b2x, 41, 299);
  expectBetween("animBox2 in-flight width", b2w, 61, 119);

  // rotate-only should have completed
  const rOnly = ui.getElementProperty("rotateOnly", "rotate");
  expectBetween("rotateOnly end rotate", rOnly, 29.5, 30.5);
});

ipcRenderer.on("animation:test:replace-start", () => {
  // Replace running animation for animBox mid-flight.
  ui.animate({
    id: "animBox",
    to: {
      x: 120,
      y: 180,
      width: 140,
      height: 70,
      rotate: 5
    },
    duration: 260,
    easing: "linear"
  });
  pass("replace animation started");
});

ipcRenderer.on("animation:test:extra-end", () => {
  // Final checks after replacement should match replaced target, not original target.
  const x = ui.getElementProperty("animBox", "x");
  const y = ui.getElementProperty("animBox", "y");
  const w = ui.getElementProperty("animBox", "contentWidth");
  const h = ui.getElementProperty("animBox", "contentHeight");
  const r = ui.getElementProperty("animBox", "rotate");
  expectEq("replaced end x", x, 119);
  expectEq("replaced end y", y, 179);
  expectEq("replaced end width", w, 140);
  expectEq("replaced end height", h, 70);
  expectBetween("replaced end rotate", r, 4.5, 5.5);

  // second animation should now be completed.
  expectEq("animBox2 end x", ui.getElementProperty("animBox2", "x"), 299);
  expectEq("animBox2 end y", ui.getElementProperty("animBox2", "y"), 229);
  expectEq("animBox2 end width", ui.getElementProperty("animBox2", "contentWidth"), 120);
  expectEq("animBox2 end height", ui.getElementProperty("animBox2", "contentHeight"), 50);

  for (let i = 0; i < allEasings.length; i += 1) {
    const id = "easeBox" + i;
    const expectedX = 60 + (i % 8) * 50;
    expectEq("all easing end x (" + allEasings[i] + ")", ui.getElementProperty(id, "x"), expectedX - 1);
  }

  pass("AnimationApiTest completed");
});
