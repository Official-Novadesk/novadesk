import { app, widgetWindow } from "novadesk";

console.log("=== LayoutBoxBorderTest Integration ===");

// const win = new widgetWindow({
//   id: "LayoutBoxBorderTestWindow",
//   x: 100,
//   y: 100,
//   width: 410,
//   height: 600,
//   backgroundColor: "rgba(240, 240, 240, 1.0)",
//   script: "./script.ui.js",
//   show: true
// });

const solidTest = new widgetWindow({
  id: "solidTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./solid.ui.js",
  show: true
});

const insetTest = new widgetWindow({
  id: "insetTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./inset.ui.js",
  show: true
});

const outsetTest = new widgetWindow({
  id: "outsetTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./outset.ui.js",
  show: true
});

const grooveTest = new widgetWindow({
  id: "grooveTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./groove.ui.js",
  show: true
});


const ridgeTest = new widgetWindow({
  id: "ridgeTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./ridge.ui.js",
  show: true
});

const dottedTest = new widgetWindow({
  id: "dottedTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./dotted.ui.js",
  show: true
});

const doubleTest = new widgetWindow({
  id: "doubleTest",
  width: 410,
  height: 200,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./double.ui.js",
  show: true
});