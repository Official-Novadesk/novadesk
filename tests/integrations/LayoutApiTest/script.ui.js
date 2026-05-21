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
function expectTrue(name, condition, details) {
  if (condition) pass(name, details || "");
  else fail(name, details || "");
}

// Layout container (row)
ui.addLayout({
  id: "rowLayout",
  x: 20,
  y: 20,
  width: 220,
  height: 120,
  direction: "row",
  gap: 5,
  padding: 10,
  align: "center",
  justify: "start"
});

// Child via parentId alias
ui.addShape({
  id: "childA",
  type: "rectangle",
  parentId: "rowLayout",
  x: 999,
  y: 999,
  width: 40,
  height: 20,
  fillColor: "rgba(0,160,255,0.8)"
});

// Child via legacy container property (should still work)
ui.addShape({
  id: "childB",
  type: "rectangle",
  container: "rowLayout",
  x: 999,
  y: 999,
  width: 30,
  height: 30,
  fillColor: "rgba(0,220,120,0.8)"
});

// Validate row layout behavior using stable relative checks.
const childAX = ui.getElementProperty("childA", "x");
const childAY = ui.getElementProperty("childA", "y");
const childBX = ui.getElementProperty("childB", "x");
const childBY = ui.getElementProperty("childB", "y");
const childAContentW = ui.getElementProperty("childA", "contentWidth");

expectEq("childA.container (parentId alias)", ui.getElementProperty("childA", "container"), "rowLayout");
expectEq("childB.container (legacy container)", ui.getElementProperty("childB", "container"), "rowLayout");
expectEq("row gap spacing", childBX - childAX, childAContentW + 5);
expectTrue("row center alignment (B slightly higher than A)", childBY <= childAY, "childA.y=" + childAY + " childB.y=" + childBY);

// Column layout with center justify
ui.addLayout({
  id: "colLayout",
  x: 280,
  y: 20,
  width: 180,
  height: 160,
  direction: "column",
  gap: 6,
  padding: [8, 12],
  align: "center",
  justify: "center"
});

ui.addText({
  id: "colText1",
  parentId: "colLayout",
  text: "A",
  width: 40,
  height: 20,
  fontSize: 16,
  fontColor: "#ffffff"
});

ui.addText({
  id: "colText2",
  parentId: "colLayout",
  text: "B",
  width: 50,
  height: 20,
  fontSize: 16,
  fontColor: "#ffffff"
});

// Validate column layout relative spacing and ordering.
const colY1 = ui.getElementProperty("colText1", "y");
const colY2 = ui.getElementProperty("colText2", "y");
const colH1 = ui.getElementProperty("colText1", "height");
expectEq("column gap spacing", colY2 - colY1, colH1 + 6);
expectTrue("column ordering", colY2 > colY1, "colText1.y=" + colY1 + " colText2.y=" + colY2);

pass("LayoutApiTest completed");
