const styles = ["double", "dotted", "dashed", "groove", "ridge", "inset", "outset", "solid"];
const perSideStyles = [
  ["groove", "dashed", "ridge", "double"]
];

ui.beginUpdate();

// for (let i = 0; i < styles.length; i++) {
//   let style = styles[i];
//   ui.addLayoutBox({
//     id: "box_" + style,
//     x: 80 + (i % 5) * 150,
//     y: 80 + Math.floor(i / 5) * 150,
//     width: 100,
//     height: 100,
//     backgroundColor: "rgb(102, 255, 0)",
//     borderWidth: 10,
//     borderColor: "rgb(100, 100, 200)",
//     borderStyle: style,
//     borderRadius: 20,
//     borderPosition: "inside",
//     children: [{
//       type: "text",
//       id: "text_" + style,
//       // text: style,
//       x: 50,
//       y: 40,
//       fontColor: "rgb(0,0,0)"
//     }]
//   });
// }

for (let i = 0; i < perSideStyles.length; i++) {
  let styleArr = perSideStyles[i];
  ui.addLayoutBox({
    id: "box_perside_" + i,
    x: 40 + i * 150,
    y: 350,
    width: 100,
    height: 100,
    backgroundColor: "rgb(28, 191, 166)",
    borderWidth: 10,
    borderRadius: 10,
    borderPosition: "outside",
    borderColor: "rgb(200, 100, 100)",
    borderStyle: styleArr,
    // children: [{
    //   type: "text",
    //   id: "text_perside_" + i,
    //   text: styleArr.length + " sides",
    //   x: 10,
    //   y: 40,
    //   fontColor: "rgb(0,0,0)"
    // }]
  });
}

ui.endUpdate();
