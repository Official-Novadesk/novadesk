ui.beginUpdate();

// ========================================
// ALIGN-ITEMS VALUES COMPARISON
// Based on CSS Flexbox Specification
// ========================================

ui.addText({
  id: "title",
  x: 20,
  y: 10,
  text: "AlignItems Property - All Values Comparison",
  fontSize: 18,
  fontWeight: 700,
  fontColor: "#000000"
});

ui.addText({
  id: "subtitle",
  x: 20,
  y: 40,
  text: "NOTE: 'normal' and 'stretch' are IDENTICAL in flexbox (per CSS spec)",
  fontSize: 13,
  fontWeight: 600,
  fontColor: "#CC0000"
});

// ========================================
// ROW LAYOUTS
// ========================================

ui.addText({
  id: "rowLabel",
  x: 20,
  y: 70,
  text: "ROW LAYOUTS (flexDirection: row) - alignItems controls VERTICAL positioning",
  fontSize: 14,
  fontWeight: 700,
  fontColor: "#333333"
});

// 1. alignItems: "stretch" (or "normal" - same result)
ui.addText({
  id: "stretchLabel",
  x: 20,
  y: 95,
  text: "alignItems: 'stretch' (or 'normal') - Items fill container height",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#0066CC"
});
ui.addLayoutBox({
  id: "rowStretch",
  x: 20,
  y: 115,
  width: 380,
  height: 100,
  flexDirection: "row",
  alignItems: "stretch",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(0, 102, 204, 1)",
  fillColor: "rgba(230, 240, 255, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "stretchChild1",
      type: "rectangle",
      width: 60,
      height: 50, // Will be overridden → 80px
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "stretchChild2",
      type: "rectangle",
      width: 60,
      height: 60, // Will be overridden → 80px
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "stretchChild3",
      type: "rectangle",
      width: 60,
      height: 40, // Will be overridden → 80px
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "stretchChild4",
      type: "rectangle",
      width: 60,
      height: 70, // Will be overridden → 80px
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 2. alignItems: "center"
ui.addText({
  id: "centerLabel",
  x: 420,
  y: 95,
  text: "alignItems: 'center' - Items centered, keep heights",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#009900"
});
ui.addLayoutBox({
  id: "rowCenter",
  x: 420,
  y: 115,
  width: 380,
  height: 100,
  flexDirection: "row",
  alignItems: "center",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(0, 153, 0, 1)",
  fillColor: "rgba(230, 255, 230, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "centerChild1",
      type: "rectangle",
      width: 60,
      height: 50,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "centerChild2",
      type: "rectangle",
      width: 60,
      height: 60,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "centerChild3",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "centerChild4",
      type: "rectangle",
      width: 60,
      height: 70,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 3. alignItems: "start"
ui.addText({
  id: "startLabel",
  x: 20,
  y: 230,
  text: "alignItems: 'start' - Items at top, keep heights",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#CC6600"
});
ui.addLayoutBox({
  id: "rowStart",
  x: 20,
  y: 250,
  width: 380,
  height: 100,
  flexDirection: "row",
  alignItems: "start",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(204, 102, 0, 1)",
  fillColor: "rgba(255, 240, 230, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "startChild1",
      type: "rectangle",
      width: 60,
      height: 50,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "startChild2",
      type: "rectangle",
      width: 60,
      height: 60,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "startChild3",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "startChild4",
      type: "rectangle",
      width: 60,
      height: 70,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 4. alignItems: "end"
ui.addText({
  id: "endLabel",
  x: 420,
  y: 230,
  text: "alignItems: 'end' - Items at bottom, keep heights",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#660099"
});
ui.addLayoutBox({
  id: "rowEnd",
  x: 420,
  y: 250,
  width: 380,
  height: 100,
  flexDirection: "row",
  alignItems: "end",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(102, 0, 153, 1)",
  fillColor: "rgba(240, 230, 255, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "endChild1",
      type: "rectangle",
      width: 60,
      height: 50,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "endChild2",
      type: "rectangle",
      width: 60,
      height: 60,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "endChild3",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "endChild4",
      type: "rectangle",
      width: 60,
      height: 70,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// ========================================
// COLUMN LAYOUTS
// ========================================

ui.addText({
  id: "colLabel",
  x: 20,
  y: 370,
  text: "COLUMN LAYOUTS (flexDirection: column) - alignItems controls HORIZONTAL positioning",
  fontSize: 14,
  fontWeight: 700,
  fontColor: "#333333"
});

// 5. Column alignItems: "stretch"
ui.addText({
  id: "colStretchLabel",
  x: 20,
  y: 395,
  text: "alignItems: 'stretch' - Items fill width",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#0066CC"
});
ui.addLayoutBox({
  id: "colStretch",
  x: 20,
  y: 415,
  width: 180,
  height: 280,
  flexDirection: "column",
  alignItems: "stretch",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(0, 102, 204, 1)",
  fillColor: "rgba(230, 240, 255, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "colStretchChild1",
      type: "rectangle",
      width: 80,  // Will be overridden → 160px
      height: 40,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStretchChild2",
      type: "rectangle",
      width: 120, // Will be overridden → 160px
      height: 40,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStretchChild3",
      type: "rectangle",
      width: 100, // Will be overridden → 160px
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStretchChild4",
      type: "rectangle",
      width: 60,  // Will be overridden → 160px
      height: 40,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 6. Column alignItems: "center"
ui.addText({
  id: "colCenterLabel",
  x: 220,
  y: 395,
  text: "alignItems: 'center' - Items centered",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#009900"
});
ui.addLayoutBox({
  id: "colCenter",
  x: 220,
  y: 415,
  width: 180,
  height: 280,
  flexDirection: "column",
  alignItems: "center",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(0, 153, 0, 1)",
  fillColor: "rgba(230, 255, 230, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "colCenterChild1",
      type: "rectangle",
      width: 80,
      height: 40,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colCenterChild2",
      type: "rectangle",
      width: 120,
      height: 40,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colCenterChild3",
      type: "rectangle",
      width: 100,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colCenterChild4",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 7. Column alignItems: "start"
ui.addText({
  id: "colStartLabel",
  x: 420,
  y: 395,
  text: "alignItems: 'start' - Items at left (LTR)",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#CC6600"
});
ui.addLayoutBox({
  id: "colStart",
  x: 420,
  y: 415,
  width: 180,
  height: 280,
  flexDirection: "column",
  alignItems: "start",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(204, 102, 0, 1)",
  fillColor: "rgba(255, 240, 230, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "colStartChild1",
      type: "rectangle",
      width: 80,
      height: 40,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStartChild2",
      type: "rectangle",
      width: 120,
      height: 40,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStartChild3",
      type: "rectangle",
      width: 100,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colStartChild4",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

// 8. Column alignItems: "end"
ui.addText({
  id: "colEndLabel",
  x: 620,
  y: 395,
  text: "alignItems: 'end' - Items at right (LTR)",
  fontSize: 11,
  fontWeight: 600,
  fontColor: "#660099"
});
ui.addLayoutBox({
  id: "colEnd",
  x: 620,
  y: 415,
  width: 180,
  height: 280,
  flexDirection: "column",
  alignItems: "end",
  gap: 10,
  padding: 10,
  borderRadius: 5,
  borderWidth: 2,
  borderColor: "rgba(102, 0, 153, 1)",
  fillColor: "rgba(240, 230, 255, 0.5)",
  children: [
    {
      elementType: "shape",
      id: "colEndChild1",
      type: "rectangle",
      width: 80,
      height: 40,
      fillColor: "rgba(255, 100, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colEndChild2",
      type: "rectangle",
      width: 120,
      height: 40,
      fillColor: "rgba(100, 255, 100, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colEndChild3",
      type: "rectangle",
      width: 100,
      height: 40,
      fillColor: "rgba(100, 200, 255, 1)",
      borderRadius: 4
    },
    {
      elementType: "shape",
      id: "colEndChild4",
      type: "rectangle",
      width: 60,
      height: 40,
      fillColor: "rgba(255, 200, 100, 1)",
      borderRadius: 4
    }
  ]
});

ui.addText({
  id: "summary",
  x: 20,
  y: 710,
  text: "SUMMARY: stretch/normal = fill cross-axis | center = centered | start = cross-start | end = cross-end",
  fontSize: 12,
  fontWeight: 700,
  fontColor: "#009900"
});

ui.endUpdate();
