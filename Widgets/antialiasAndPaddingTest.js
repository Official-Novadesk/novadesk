// Antialiasing and Padding Test
// Tests antialiasing control and padding for text and images

!logToFile;
var w = new widgetWindow({
    id: "AntialiasAndPaddingTest",
    x: 150,
    y: 150,
    width: 400,
    height: 400,
    backgroundcolor: "rgba(50,50,50,255)",
    draggable: true
});

// // Title
// w.addText({
//     id: "title",
//     text: "Antialiasing & Padding Demo",
//     x: 20, y: 10,
//     fontsize: 24,
//     color: "white",
//     bold: true
// });

// // ===== TEXT ANTIALIASING TESTS =====
// w.addText({
//     id: "text_aa_label",
//     text: "Text Antialiasing",
//     x: 20, y: 50,
//     fontsize: 16,
//     color: "white",
//     bold: true
// });

// // Antialiased text (default)
// w.addText({
//     id: "text_aa_on",
//     text: "Smooth Text\n(Antialiased)",
//     x: 20, y: 80,
//     width: 180, height: 60,
//     fontsize: 18,
//     color: "white",
//     solidcolor: "rgba(60,60,100,255)",
//     solidcolorradius: 8,
//     antialias: true  // Enabled (default)
// });

// // Non-antialiased text
// w.addText({
//     id: "text_aa_off",
//     text: "Pixelated Text\n(No Antialias)",
//     x: 220, y: 80,
//     width: 180, height: 60,
//     fontsize: 18,
//     color: "white",
//     solidcolor: "rgba(100,60,60,255)",
//     solidcolorradius: 8,
//     antialias: false  // Disabled
// });

// // ===== TEXT PADDING TESTS =====
// w.addText({
//     id: "text_pad_label",
//     text: "Text Padding",
//     x: 20, y: 160,
//     fontsize: 16,
//     color: "white",
//     bold: true
// });

// // No padding
// w.addText({
//     id: "text_no_pad",
//     text: "No Padding",
//     x: 20, y: 190,
//     width: 150, height: 50,
//     fontsize: 14,
//     color: "white",
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5
// });

// // Uniform padding (all sides)
// w.addText({
//     id: "text_uniform_pad",
//     text: "Padding: 10px",
//     x: 190, y: 190,
//     width: 150, height: 50,
//     fontsize: 14,
//     color: "white",
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5,
//     padding: 10  // 10px all sides
// });

// // Horizontal/Vertical padding
// w.addText({
//     id: "text_hv_pad",
//     text: "H:15 V:5",
//     x: 360, y: 190,
//     width: 150, height: 50,
//     fontsize: 14,
//     color: "white",
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5,
//     padding: [15, 5]  // [horizontal, vertical]
// });

// // Custom padding (left, top, right, bottom)
// w.addText({
//     id: "text_custom_pad",
//     text: "Custom\nPadding",
//     x: 530, y: 190,
//     width: 150, height: 50,
//     fontsize: 14,
//     color: "white",
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5,
//     padding: [20, 5, 5, 5]  // [left, top, right, bottom]
// });

// // ===== IMAGE ANTIALIASING TESTS =====
// w.addText({
//     id: "img_aa_label",
//     text: "Image Antialiasing (Interpolation)",
//     x: 20, y: 260,
//     fontsize: 16,
//     color: "white",
//     bold: true
// });

// // High quality interpolation (default)
// w.addImage({
//     id: "img_aa_on",
//     path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
//     x: 20, y: 290,
//     width: 120, height: 120,
//     solidcolor: "rgba(60,60,100,255)",
//     solidcolorradius: 8,
//     antialias: true  // High quality bicubic
// });

// w.addText({
//     id: "img_aa_on_label",
//     text: "Smooth\n(Bicubic)",
//     x: 20, y: 420,
//     fontsize: 12,
//     color: "white",
//     align: "center"
// });

// // Nearest neighbor (pixelated)
// w.addImage({
//     id: "img_aa_off",
//     path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
//     x: 160, y: 290,
//     width: 120, height: 120,
//     solidcolor: "rgba(100,60,60,255)",
//     solidcolorradius: 8,
//     antialias: false  // Nearest neighbor
// });

// w.addText({
//     id: "img_aa_off_label",
//     text: "Pixelated\n(Nearest)",
//     x: 160, y: 420,
//     fontsize: 12,
//     color: "white",
//     align: "center"
// });

// // ===== IMAGE PADDING TESTS =====
// w.addText({
//     id: "img_pad_label",
//     text: "Image Padding",
//     x: 300, y: 260,
//     fontsize: 16,
//     color: "white",
//     bold: true
// });

// // No padding
// w.addImage({
//     id: "img_no_pad",
//     path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
//     x: 300, y: 290,
//     width: 100, height: 100,
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5
// });

// w.addText({
//     id: "img_no_pad_label",
//     text: "No Padding",
//     x: 300, y: 400,
//     fontsize: 12,
//     color: "white"
// });

// // Uniform padding
// w.addImage({
//     id: "img_uniform_pad",
//     path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
//     x: 420, y: 290,
//     width: 100, height: 100,
//     solidcolor: "rgba(80,80,80,255)",
//     solidcolorradius: 5,
//     padding: 15  // 15px all sides
// });

// w.addText({
//     id: "img_uniform_pad_label",
//     text: "Padding: 15px",
//     x: 420, y: 400,
//     fontsize: 12,
//     color: "white"
// });

// Custom padding
w.addImage({
    id: "img_custom_pad",
    path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
    x: 50, y: 20,
    // width: 100, height: 100,
    solidcolor: "rgba(80,80,80,255)",
    solidcolorradius: 5,
    padding: [300, 300, 300, 300]  // More padding on top
});

// w.addText({
//     id: "img_custom_pad_label",
//     text: "Top: 20px",
//     x: 540, y: 400,
//     fontsize: 12,
//     color: "white"
// });

// // ===== COMBINED EFFECTS =====
// w.addText({
//     id: "combined_label",
//     text: "Combined: Gradient + Bevel + Padding + Rotation",
//     x: 20, y: 470,
//     fontsize: 16,
//     color: "white",
//     bold: true
// });

// w.addText({
//     id: "combined_text",
//     text: "Fancy Text!",
//     x: 50, y: 510,
//     width: 200, height: 70,
//     fontsize: 20,
//     color: "white",
//     bold: true,
//     solidcolor: "rgba(255,100,150,255)",
//     solidcolor2: "rgba(100,150,255,255)",
//     gradientangle: 45,
//     solidcolorradius: 12,
//     beveltype: 1,
//     bevelwidth: 3,
//     padding: 15,
//     rotate: 5,
//     antialias: true
// });

// w.addImage({
//     id: "combined_img",
//     path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
//     x: 280, y: 510,
//     width: 100, height: 70,
//     solidcolor: "rgba(150,100,255,255)",
//     solidcolor2: "rgba(255,200,100,255)",
//     gradientangle: 90,
//     solidcolorradius: 12,
//     beveltype: 2,
//     bevelwidth: 3,
//     padding: 10,
//     rotate: -5,
//     antialias: true
// });

// novadesk.log("Antialiasing and Padding Test Loaded Successfully!");
