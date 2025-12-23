
// Create a widget window
var w = new widgetWindow({
    id: "ImageTest",
    x: 100,
    y: 100,
    width: 400,
    height: 400,
    backgroundcolor: "rgba(30,30,30,255)",
    clickthrough: false,
    draggable: true
});

// Assuming we have a test image. Using a placeholder path, user might need to adjust.
// Or I can try to use a solid color standard image if I had one, but I don't.
// I'll try to use a known file if possible, or just expect the user to have 'test.png'
var imgPath = "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico";

// w.addText({
//     id: "t1",
//     text: "Original (0: Stretch, 1: Fit, 2: Crop)",
//     x: 10, y: 10,
//     fontsize: 20,
//     color: "white"
// });

// // 1. Stretch (Default)
// w.addText({ id: "l1", text: "Stretch (0)", x: 10, y: 50, color: "white" });
// w.addImage({
//     id: "img_stretch",
//     path: imgPath,
//     x: 10, y: 80,
//     width: 200, height: 100,
//     solidcolor: "rgba(255,255,255,50)", // Semi-transparent bg to see the box
//     preserveaspectratio: 0
// });

// // 2. Preserve (Fit)
// w.addText({ id: "l2", text: "Preserve (1)", x: 250, y: 50, color: "white" });
// w.addImage({
//     id: "img_preserve",
//     path: imgPath,
//     x: 250, y: 80,
//     width: 200, height: 100,
//     solidcolor: "rgba(255,255,255,50)",
//     preserveaspectratio: 1
// });

// // 3. Crop (Fill)
// w.addText({ id: "l3", text: "Crop (2)", x: 500, y: 50, color: "white" });
// w.addImage({
//     id: "img_crop",
//     path: imgPath,
//     x: 500, y: 80,
//     width: 200, height: 100,
//     solidcolor: "rgba(255,255,255,50)",
//     preserveaspectratio: 2
// });

// // 4. Tint
// w.addText({ id: "l4", text: "Tint (Red)", x: 10, y: 250, color: "white" });
// w.addImage({
//     id: "img_tint",
//     path: imgPath,
//     x: 10, y: 280,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     imagetint: "rgb(255,0,0)"
// });

// // 5. Tint + Alpha
// w.addText({ id: "l5", text: "Tint (Blue + 50% Alpha)", x: 250, y: 250, color: "white" });
// w.addImage({
//     id: "img_tint_alpha",
//     path: imgPath,
//     x: 250, y: 280,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     imagetint: "rgb(0,0,255)"
// });


// // 6. Image Alpha
// w.addText({ id: "l6", text: "Image Alpha (50%)", x: 10, y: 400, color: "white" });
// w.addImage({
//     id: "img_alpha",
//     path: imgPath,
//     x: 10, y: 430,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     imagealpha: 10
// });

// // 7. Grayscale
// w.addText({ id: "l7", text: "Grayscale", x: 250, y: 400, color: "white" });
// w.addImage({
//     id: "img_gray",
//     path: imgPath,
//     x: 250, y: 430,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     grayscale: true
// });

// // 8. ColorMatrix (Sepia)
// w.addText({ id: "l8", text: "ColorMatrix (Sepia)", x: 500, y: 400, color: "white" });
// w.addImage({
//     id: "img_matrix",
//     path: imgPath,
//     x: 500, y: 430,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     colormatrix: [
//         0.393, 0.349, 0.272, 0, 0,
//         0.769, 0.686, 0.534, 0, 0,
//         0.189, 0.168, 0.131, 0, 0,
//         0, 0, 0, 1, 0,
//         0, 0, 0, 0, 1
//     ]
// });

// // 9. ColorMatrix Array (Negative)
// w.addText({ id: "l9", text: "ColorMatrix Array (Negative)", x: 10, y: 550, color: "white" });
// w.addImage({
//     id: "img_matrix_array",
//     path: imgPath,
//     x: 10, y: 580,
//     width: 200, height: 100,
//     preserveaspectratio: 1,
//     colormatrix: [
//         -1, 0, 0, 0, 0,
//         0, -1, 0, 0, 0,
//         0, 0, -1, 0, 0,
//         0, 0, 0, 1, 0,
//         1, 1, 1, 0, 1
//     ]
// });

// // 10. Tile
// w.addText({ id: "l10", text: "Tile", x: 250, y: 550, color: "white" });
// w.addImage({
//     id: "img_tile",
//     path: imgPath,
//     x: 250, y: 580,
//     width: 200, height: 100,
//     tile: true
// });

// // 11. Rotate (45 degrees)
// w.addText({ id: "l11", text: "Rotate (45 deg)", x: 500, y: 550, color: "white" });
// w.addImage({
//     id: "img_rotate",
//     path: imgPath,
//     x: 500, y: 580,
//     width: 100, height: 100,
//     preserveaspectratio: 1,
//     imagerotate: 45
// });

// 12. ScaleMargins (9-slice)
// Note: This effect is best seen with a specific UI border image, but we'll test valid property parsing here.
w.addText({ id: "l12", text: "ScaleMargins (10,10,10,10)", x: 10, y: 700, color: "white" });
w.addImage({
    id: "img_margins",
    path: imgPath,
    x: 10, y: 30,
    tile: true,
    width: 300, height: 100,
    // scalemargins: [10, 10, 10, 10]
});

// novadesk.log("Image Test Loaded");
