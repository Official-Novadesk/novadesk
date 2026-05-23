const styles = ["double"];
const perSideStyles = [
  ["solid", "dotted"],
  ["dashed", "double", "solid"],
  ["groove", "ridge", "inset", "outset"]
];

ui.beginUpdate();

for (let i = 0; i < styles.length; i++) {
  let style = styles[i];
  ui.addLayoutBox({
    id: "box_" + style,
    x: 80 + (i % 5) * 150,
    y: 80 + Math.floor(i / 5) * 150,
    width: 200,
    height: 200,
    backgroundColor: "rgb(102, 255, 0)",
    borderWidth: 50,
    borderColor: "rgb(100, 100, 200)",
    borderStyle: style,
    children: [{
      type: "text",
      id: "text_" + style,
      text: style,
      x: 10,
      y: 40,
      fontColor: "rgb(0,0,0)"
    }]
  });
}

// for (let i = 0; i < perSideStyles.length; i++) {
//   let styleArr = perSideStyles[i];
//   ui.addLayoutBox({
//     id: "box_perside_" + i,
//     x: 40 + i * 150,
//     y: 350,
//     width: 100,
//     height: 100,
//     backgroundColor: "rgb(255, 230, 230)",
//     borderWidth: 8,
//     borderColor: "rgb(200, 100, 100)",
//     borderStyle: styleArr,
//     children: [{
//       type: "text",
//       id: "text_perside_" + i,
//       text: styleArr.length + " sides",
//       x: 10,
//       y: 40,
//       fontColor: "rgb(0,0,0)"
//     }]
//   });
// }

ui.endUpdate();
