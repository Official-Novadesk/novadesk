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
  id: "title",
  x: 20,
  y: 12,
  text: "Text animation test (keyframes)",
  fontSize: 14,
  fontColor: "#ffffff"
});

ui.addText({
  id: "animText",
  x: 40,
  y: 80,
  text: "Animated",
  fontSize: 24,
  fontWeight: 400,
  fontColor: "rgba(255,80,80,1)",
  letterSpacing: 0
});

ui.endUpdate();

ui.animate({
  id: "animText",
  duration: 5000,
  easing: "linear",
  keyframes: {
    "0%": {
      fontSize: 12,
      fontWeight: 300,
      letterSpacing: -1,
      fontColor: "rgba(80,180,255,0.5)"
    },
    "100%": {
      fontSize: 36,
      fontWeight: 700,
      letterSpacing: 4,
      fontColor: "rgba(255,200,80,1)"
    }
  }
});

expectEq("fontSize snap", ui.getElementProperty("animText", "fontSize"), 12);
expectEq("fontWeight snap", ui.getElementProperty("animText", "fontWeight"), 300);
expectBetween("letterSpacing snap", ui.getElementProperty("animText", "letterSpacing"), -1.1, -0.9);

ipcRenderer.on("animation:text:mid", () => {
  expectBetween("fontSize mid", ui.getElementProperty("animText", "fontSize"), 22, 26);
  expectBetween("fontWeight mid", ui.getElementProperty("animText", "fontWeight"), 480, 520);
  expectBetween("letterSpacing mid", ui.getElementProperty("animText", "letterSpacing"), 1.0, 3.0);
});

ipcRenderer.on("animation:text:end", () => {
  expectEq("fontSize end", ui.getElementProperty("animText", "fontSize"), 36);
  expectEq("fontWeight end", ui.getElementProperty("animText", "fontWeight"), 700);
  expectBetween("letterSpacing end", ui.getElementProperty("animText", "letterSpacing"), 3.5, 4.5);
  pass("AnimationTextTest completed");
});
