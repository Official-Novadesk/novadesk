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

// LayoutBox container (row) using nested children
ui.addLayoutBox({
  id: "rowLayout",
  x: 20,
  y: 20,
  width: 220,
  height: 120,
  direction: "row",
  gap: 5,
  padding: 10,
  alignItems: "center",
  justifyContent: "start",
  backgroundColor: "rgba(255,255,255,0.02)",
  borderRadius: 8,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.1)",
  borderPosition: "outside",
  children: [
    ui.shape({
      id: "childA",
      type: "rectangle",
      x: 999,
      y: 999,
      width: 40,
      height: 20,
      fillColor: "rgba(0,160,255,0.8)"
    }),
    ui.shape({
      id: "childB",
      type: "rectangle",
      x: 999,
      y: 999,
      width: 30,
      height: 30,
      fillColor: "rgba(0,220,120,0.8)"
    })
  ]
});

// Validate row layout behavior using stable relative checks.
const childAX = ui.getElementProperty("childA", "x");
const childAY = ui.getElementProperty("childA", "y");
const childBX = ui.getElementProperty("childB", "x");
const childBY = ui.getElementProperty("childB", "y");
const childAContentW = ui.getElementProperty("childA", "contentWidth");

expectEq("childA.container (children nesting)", ui.getElementProperty("childA", "container"), "rowLayout");
expectEq("childB.container (children nesting)", ui.getElementProperty("childB", "container"), "rowLayout");
expectEq("row gap spacing", childBX - childAX, childAContentW + 5);
expectTrue("row center alignment (B slightly higher than A)", childBY <= childAY, "childA.y=" + childAY + " childB.y=" + childBY);

ui.beginUpdate();
// Column LayoutBox with style keys (paddingX/paddingY/justifyContent/alignItems)
ui.addLayoutBox({
  id: "colLayout",
  x: 280,
  y: 20,
  width: 180,
  height: 160,
  direction: "column",
  gap: 6,
  paddingX: 8,
  paddingY: 12,
  alignItems: "center",
  justifyContent: "center",
  children: [
    ui.text({
      id: "colText1",
      text: "A",
      x: 0,
      y: 0,
      width: 40,
      height: 20,
      fontSize: 16,
      fontColor: "#ffffff"
    }),
    ui.text({
      id: "colText2",
      text: "B",
      x: 0,
      y: 0,
      width: 50,
      height: 20,
      fontSize: 16,
      fontColor: "#ffffff"
    })
  ]
});

// Validate column layout relative spacing and ordering.
const colY1 = ui.getElementProperty("colText1", "y");
const colY2 = ui.getElementProperty("colText2", "y");
const colH1 = ui.getElementProperty("colText1", "height");
expectEq("column gap spacing", colY2 - colY1, colH1 + 6);
expectTrue("column ordering", colY2 > colY1, "colText1.y=" + colY1 + " colText2.y=" + colY2);

// Example 1: Dashboard card with header/body/footer
ui.addLayoutBox({
  id: "cardMain",
  x: 20, y: 170, width: 260, height: 180,
  direction: "column",
  gap: 8,
  padding: 10,
  backgroundColor: "rgba(30,30,38,0.9)",
  borderRadius: 10,
  borderWidth: 1,
  borderColor: "rgba(255,255,255,0.12)",
  borderPosition: "outside",
  children: [
    ui.text({ id: "cardTitle", x: 0, y: 0, text: "System", fontSize: 16, fontWeight: 700, fontColor: "#ffffff" }),
    ui.layoutBox({
      id: "cardBodyRow",
      width: 240, height: 28, direction: "row", gap: 6, justifyContent: "end", alignItems: "center",
      children: [
        ui.text({ id: "cpuLbl", x: 0, y: 0, text: "CPU", fontSize: 13, fontColor: "#cfd3da" }),
        ui.text({ id: "cpuVal", x: 0, y: 0, text: "32%", fontSize: 13, fontColor: "#7fe3a1" })
      ]
    }),
    ui.layoutBox({
      id: "cardFooter",
      width: 240, height: 28, direction: "row", gap: 8, justifyContent: "end", alignItems: "center",
      children: [
        ui.shape({ id: "btnA", x: 0, y: 0, type: "rectangle", width: 60, height: 24, radiusX: 6, radiusY: 6, fillColor: "rgba(70,120,255,0.8)" }),
        ui.shape({ id: "btnB", x: 0, y: 0, type: "rectangle", width: 60, height: 24, radiusX: 6, radiusY: 6, fillColor: "rgba(95,95,110,0.8)" })
      ]
    })
  ]
});

// Example 2: Two-column settings area
ui.addLayoutBox({
  id: "settingsGrid",
  x: 300, y: 170, width: 300, height: 180,
  direction: "row",
  gap: 12,
  paddingX: 10,
  paddingY: 10,
  alignItems: "stretch",
  backgroundColor: "rgba(18,22,30,0.85)",
  borderRadius: 8,
  borderPosition: "outside",
  children: [
    ui.layoutBox({
      id: "leftCol",
      width: 130, height: 160, direction: "column", gap: 8, minWidth: 130,
      children: [
        ui.text({ id: "left1", x: 0, y: 0, text: "Theme", fontSize: 13, fontColor: "#fff" }),
        ui.text({ id: "left2", x: 0, y: 0, text: "Language", fontSize: 13, fontColor: "#fff" }),
        ui.text({ id: "left3", x: 0, y: 0, text: "Timezone", fontSize: 13, fontColor: "#fff" })
      ]
    }),
    ui.layoutBox({
      id: "rightCol",
      width: 130, height: 160, direction: "column", gap: 8, minWidth: 130, alignItems: "end",
      children: [
        ui.text({ id: "right1", x: 0, y: 0, text: "Dark", fontSize: 13, fontColor: "#a7d9ff" }),
        ui.text({ id: "right2", x: 0, y: 0, text: "English", fontSize: 13, fontColor: "#a7d9ff" }),
        ui.text({ id: "right3", x: 0, y: 0, text: "UTC+5", fontSize: 13, fontColor: "#a7d9ff" })
      ]
    })
  ]
});

ui.endUpdate();
pass("LayoutApiTest completed");
