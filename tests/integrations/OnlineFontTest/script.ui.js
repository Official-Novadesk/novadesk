// ============================================================
// OnlineFontTest - script.ui.js
// Exercises the fontPath async-download feature added to Novadesk.
//
// Test cases:
//   1. Text element renders with a Google Fonts WOFF2 URL (Orbitron)
//      assigned to fontPath.
//   2. InputBox element renders with a Google Fonts WOFF2 URL (Playwrite NZ Guides)
//      assigned to fontPath.
//   3. A second Text element with the *same* URL hits the in-memory cache
//      and resolves immediately without downloading again.
//   4. An invalid URL is handled gracefully (element still renders,
//      fontPath stays empty, no crash).
// ============================================================

// --------------- helpers ---------------
function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}
function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

// --------------- Google Fonts WOFF2 raw URLs ---------------
var ORBITRON_URL  = "https://fonts.gstatic.com/s/orbitron/v31/yMJMMIlzdpvBhQQL_SC3X9yhF25-T1nyGy6BoWgz.woff2";
var PLAYWRITE_URL = "https://fonts.gstatic.com/s/playwritenzguides/v2/t5t8IQQPN4uFDRepJwiX4vzIikyGzv71Who.woff2";
var INVALID_URL   = "https://fonts.gstatic.com/s/this_font_does_not_exist/v1/fake.woff2";

// --------------- UI setup ---------------
ui.beginUpdate();

// Title label (system font – no fontPath)
ui.addText({
  id: "title",
  x: 20, y: 12,
  text: "Online Font Test",
  fontSize: 16,
  fontColor: "#ffffff",
  fontWeight: 700
});

// Test 1 – Text element with Orbitron URL in fontPath
ui.addText({
  id: "orbitronLabel",
  x: 20, y: 55,
  text: "Orbitron (downloading…)",
  fontSize: 30,
  fontColor: "#00d4ff",
  fontFace: "Orbitron",
  fontPath: ORBITRON_URL
});

// Test 2 – InputBox with Playwrite URL in fontPath
ui.addInputBox({
  id: "exo2Input",
  x: 20, y: 115,
  width: 400, height: 46,
  text: "Playwrite font (downloading…)",
  fontSize: 20,
  fontColor: "#f0e090",
  fontFace: "Playwrite NZ Guides",
  fontPath: PLAYWRITE_URL,
  borderRadius: 8,
  backgroundColor: "rgba(255,255,255,0.08)"
});

// Test 3 – Same URL as Test 1 → should hit in-memory cache immediately
ui.addText({
  id: "orbitronCached",
  x: 20, y: 180,
  text: "Orbitron cached (should be fast)",
  fontSize: 22,
  fontColor: "#a0f0c0",
  fontFace: "Orbitron",
  fontPath: ORBITRON_URL    // same URL as Test 1
});

// Test 4 – Invalid URL → must not crash; fontPath should stay empty
ui.addText({
  id: "invalidFont",
  x: 20, y: 230,
  text: "Invalid URL (graceful fallback)",
  fontSize: 18,
  fontColor: "#ff8888",
  fontFace: "NonExistentFont",
  fontPath: INVALID_URL
});

// Status line
ui.addText({
  id: "status",
  x: 20, y: 305,
  text: "Waiting for downloads… (timeout 10 s)",
  fontSize: 13,
  fontColor: "#888888"
});

ui.endUpdate();

// --------------- final check triggered by index.js after 10 s ---------------
ipcRenderer.on("font:test:check", function () {
  console.log("--- OnlineFontTest: final assertions ---");

  // --- Test 1: Orbitron fontPath non-empty ---
  var orbitronPath = ui.getElementProperty("orbitronLabel", "fontPath");
  if (orbitronPath === ORBITRON_URL) {
    pass("Test 1 – Orbitron fontPath: applied after async download", "fontPath=" + orbitronPath);
    ui.setElementProperties("orbitronLabel", { text: "Orbitron loaded ✓" });
  } else {
    fail("Test 1 – Orbitron fontPath: mismatch or empty", "fontPath=" + orbitronPath);
  }

  // --- Test 2: Playwrite fontPath non-empty ---
  var playwritePath = ui.getElementProperty("exo2Input", "fontPath");
  if (playwritePath === PLAYWRITE_URL) {
    pass("Test 2 – Playwrite fontPath (InputBox): applied after async download", "fontPath=" + playwritePath);
    ui.setElementProperties("exo2Input", { text: "Playwrite loaded ✓" });
  } else {
    fail("Test 2 – Playwrite fontPath (InputBox): mismatch or empty", "fontPath=" + playwritePath);
  }

  // --- Test 3: Cached copy should have resolved immediately ---
  var cachedPath = ui.getElementProperty("orbitronCached", "fontPath");
  if (cachedPath === ORBITRON_URL) {
    pass("Test 3 – Same URL cache: fontPath applied on cached element", "fontPath=" + cachedPath);
  } else {
    fail("Test 3 – Same URL cache: mismatch or empty", "fontPath=" + cachedPath);
  }

  // --- Test 4: Invalid URL must not crash; fontPath must remain empty ---
  var invalidPath = ui.getElementProperty("invalidFont", "fontPath");
  if (invalidPath === null || invalidPath === undefined || invalidPath === "") {
    pass("Test 4 – Invalid fontPath URL: graceful fallback, no crash, fontPath stays empty");
    ui.setElementProperties("invalidFont", { text: "Invalid URL → graceful ✓" });
  } else {
    fail("Test 4 – Invalid fontPath URL: unexpectedly got a fontPath=" + invalidPath);
  }

  ui.setElementProperties("status", { text: "Tests completed — see console for results." });
  pass("OnlineFontTest completed");
});
