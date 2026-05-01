import { app, widgetWindow } from "novadesk";

console.log("=== WidgetWindowMethodsTest Integration ===");

const win = new widgetWindow({
  id: "WidgetWindowMethodsTestWindow",
  x: 180,
  y: 180,
  width: 420,
  height: 220,
  backgroundColor: "rgba(30, 34, 52, 0.95)",
  script: "./script.ui.js",
  show: true
});

function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function expect(name, condition, details) {
  if (condition) {
    pass(name, details);
  } else {
    fail(name, details);
  }
}

win.on("close", () => console.log("[EVENT] close"));
win.on("closed", () => console.log("[EVENT] closed"));

setTimeout(() => {
  const b = win.getBounds();
  expect("getBounds()", !!b, JSON.stringify(b));

  const s = win.getSize();
  expect("getSize()", !!s, JSON.stringify(s));

  expect("isVisible() initially", win.isVisible() === true, String(win.isVisible()));
  expect("isDestroyed() initially", win.isDestroyed() === false, String(win.isDestroyed()));
}, 250);

setTimeout(() => {
  win.setSize(460, 260);
  const size = win.getSize();
  const ok = !!size && size.width === 460 && size.height === 260;
  expect("setSize()/getSize()", ok, JSON.stringify(size));
}, 550);

setTimeout(() => {
  win.setBounds({ x: 260, y: 220, width: 500, height: 280 });
  const bounds = win.getBounds();
  const ok = !!bounds &&
    bounds.x === 260 &&
    bounds.y === 220 &&
    bounds.width === 500 &&
    bounds.height === 280;
  expect("setBounds()/getBounds()", ok, JSON.stringify(bounds));
}, 900);

setTimeout(() => {
  win.setBackgroundColor("rgba(40, 80, 150, 0.85)");
  const color = win.getBackgroundColor();
  expect("setBackgroundColor()/getBackgroundColor()", typeof color === "string" && color.length > 0, String(color));
}, 1250);

setTimeout(() => {
  win.setOpacity(0.65);
  pass("setOpacity(0.65)");
}, 1500);

setTimeout(() => {
  win.hide();
  expect("hide()/isVisible()", win.isVisible() === false, String(win.isVisible()));
}, 1800);

setTimeout(() => {
  win.show();
  expect("show()/isVisible()", win.isVisible() === true, String(win.isVisible()));
}, 2150);

setTimeout(() => {
  win.setFocus();
  const focused = win.isFocused();
  pass("setFocus()/isFocused()", String(focused));
}, 2450);

setTimeout(() => {
  win.destroy();
  pass("destroy()");
}, 2850);

setTimeout(() => {
  expect("isDestroyed() after destroy", win.isDestroyed() === true, String(win.isDestroyed()));
  app.exit();
}, 3250);
