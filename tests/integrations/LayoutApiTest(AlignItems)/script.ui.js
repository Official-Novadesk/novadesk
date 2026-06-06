ui.beginUpdate();
// LayoutBox with plain object children using elementType
ui.addLayoutBox({
  id: "rowLayoutWithPadding",
  x: 20,
  y: 20,
  width: 200,
  height: 80,
  gap: 10,
  padding: 20,
  borderRadius: 8,
  borderWidth: 3,
  borderColor: "rgba(0, 191, 255, 1)",
  fillColor: "rgba(200, 230, 255, 0.3)",
  children: [
    {
      elementType: "shape",
      id: "childA",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 5
    },
    {
      elementType: "shape",
      id: "childB",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 5
    },
    {
      elementType: "shape",
      id: "childC",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(100, 100, 255, 1)",
      borderRadius: 5
    }
  ]
});

// Second container without padding
ui.addLayoutBox({
  id: "rowLayoutNoPadding",
  x: 20,
  y: 160,
  width: 200,
  height: 80,
  gap: 10,
  padding: 0,
  borderRadius: 8,
  borderWidth: 3,
  borderColor: "rgba(255, 100, 0, 1)",
  fillColor: "rgba(255, 230, 200, 0.3)",
  children: [
    {
      elementType: "shape",
      id: "childD",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 5
    },
    {
      elementType: "shape",
      id: "childE",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 5
    },
    {
      elementType: "shape",
      id: "childF",
      shapeType: "rectangle",
      width: 50,
      height: 50,
      fillColor: "rgba(100, 100, 255, 1)",
      borderRadius: 5
    }
  ]
});

// Mixed content with different element types
ui.addLayoutBox({
  id: "mixedContent",
  x: 20,
  y: 260,
  width: 300,
  height: 60,
  gap: 8,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(100, 100, 100, 1)",
  children: [
    {
      elementType: "text",
      id: "statusLabel",
      text: "Status:",
      fontSize: 14,
      fontWeight: 600,
      fontColor: "#333333"
    },
    {
      elementType: "shape",
      id: "statusIndicator",
      shapeType: "ellipse",
      width: 20,
      height: 20,
      fillColor: "rgba(0, 255, 0, 1)"
    },
    {
      elementType: "text",
      id: "statusValue",
      text: "Active",
      fontSize: 14,
      fontColor: "#666666"
    }
  ]
});

ui.endUpdate();