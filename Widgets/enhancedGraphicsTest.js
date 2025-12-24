// Enhanced Graphics Features Test
// Tests gradients, rotation, transformation matrices, and bevel effects

var w = new widgetWindow({
    id: "EnhancedGraphicsTest",
    x: 100,
    y: 100,
    width: 900,
    height: 700,
    backgroundcolor: "rgba(40,40,40,255)",
    draggable: true
});

// Title
w.addText({
    id: "title",
    text: "Enhanced Graphics Features Demo",
    x: 20, y: 10,
    fontsize: 24,
    color: "white",
    bold: true
});

// ===== GRADIENT TESTS =====
w.addText({
    id: "gradient_label",
    text: "Gradients (SolidColor2 + GradientAngle)",
    x: 20, y: 50,
    fontsize: 16,
    color: "white",
    bold: true
});

// Horizontal gradient (0°)
w.addText({
    id: "grad1",
    text: "Horizontal",
    x: 20, y: 80,
    width: 150, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(255,0,0,255)",
    solidcolor2: "rgba(0,0,255,255)",
    gradientangle: 0,
    solidcolorradius: 10
});

// Vertical gradient (90°)
w.addText({
    id: "grad2",
    text: "Vertical",
    x: 190, y: 80,
    width: 150, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(0,255,0,255)",
    solidcolor2: "rgba(255,255,0,255)",
    gradientangle: 90,
    solidcolorradius: 10
});

// Diagonal gradient (45°)
w.addText({
    id: "grad3",
    text: "Diagonal 45°",
    x: 360, y: 80,
    width: 150, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(255,0,255,255)",
    solidcolor2: "rgba(0,255,255,255)",
    gradientangle: 45,
    solidcolorradius: 10
});

// ===== TEXT ROTATION TESTS =====
w.addText({
    id: "rotation_label",
    text: "Text Rotation",
    x: 20, y: 160,
    fontsize: 16,
    color: "white",
    bold: true
});

// Rotated text 45°
w.addText({
    id: "rot1",
    text: "Rotated 45°",
    x: 50, y: 200,
    width: 120, height: 40,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(100,100,200,200)",
    rotate: 45
});

// Rotated text 90° (vertical)
w.addText({
    id: "rot2",
    text: "Vertical 90°",
    x: 200, y: 200,
    width: 120, height: 40,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(200,100,100,200)",
    rotate: 90
});

// Rotated text -30°
w.addText({
    id: "rot3",
    text: "Tilted -30°",
    x: 350, y: 200,
    width: 120, height: 40,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(100,200,100,200)",
    rotate: -30
});

// ===== BEVEL EFFECTS TESTS =====
w.addText({
    id: "bevel_label",
    text: "Bevel Effects",
    x: 20, y: 280,
    fontsize: 16,
    color: "white",
    bold: true
});

// Raised bevel
w.addText({
    id: "bevel1",
    text: "Raised",
    x: 20, y: 310,
    width: 120, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(80,80,80,255)",
    beveltype: 1,
    bevelwidth: 3,
    bevelcolor1: "rgba(255,255,255,200)",
    bevelcolor2: "rgba(0,0,0,150)"
});

// Sunken bevel
w.addText({
    id: "bevel2",
    text: "Sunken",
    x: 160, y: 310,
    width: 120, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(80,80,80,255)",
    beveltype: 2,
    bevelwidth: 3,
    bevelcolor1: "rgba(255,255,255,200)",
    bevelcolor2: "rgba(0,0,0,150)"
});

// Emboss bevel
w.addText({
    id: "bevel3",
    text: "Emboss",
    x: 300, y: 310,
    width: 120, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(80,80,80,255)",
    beveltype: 3,
    bevelwidth: 3,
    bevelcolor1: "rgba(255,255,255,200)",
    bevelcolor2: "rgba(0,0,0,150)"
});

// Pillow bevel
w.addText({
    id: "bevel4",
    text: "Pillow",
    x: 440, y: 310,
    width: 120, height: 60,
    fontsize: 14,
    color: "white",
    solidcolor: "rgba(80,80,80,255)",
    beveltype: 4,
    bevelwidth: 5,
    bevelcolor1: "rgba(255,255,255,200)",
    bevelcolor2: "rgba(0,0,0,150)"
});

// ===== IMAGE ROTATION & TRANSFORMATION TESTS =====
w.addText({
    id: "transform_label",
    text: "Image Transformations",
    x: 20, y: 390,
    fontsize: 16,
    color: "white",
    bold: true
});

// Simple rotation
w.addImage({
    id: "img_rotate",
    path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
    x: 20, y: 420,
    width: 80, height: 80,
    solidcolor: "rgba(60,60,60,255)",
    rotate: 30
});

w.addText({
    id: "img_rotate_label",
    text: "Rotate 30°",
    x: 20, y: 510,
    fontsize: 12,
    color: "white"
});

// Transformation matrix (scale + rotate)
w.addImage({
    id: "img_transform",
    path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
    x: 140, y: 420,
    width: 80, height: 80,
    solidcolor: "rgba(60,60,60,255)",
    // Scale 1.2x and rotate 45° matrix
    transformmatrix: [0.8485, -0.8485, 0.8485, 0.8485, 0, 0]
});

w.addText({
    id: "img_transform_label",
    text: "Matrix Transform",
    x: 120, y: 510,
    fontsize: 12,
    color: "white"
});

// ===== COMBINED EFFECTS =====
w.addText({
    id: "combined_label",
    text: "Combined Effects",
    x: 20, y: 540,
    fontsize: 16,
    color: "white",
    bold: true
});

// Gradient + Bevel + Rotation
w.addText({
    id: "combined1",
    text: "All Effects!",
    x: 50, y: 580,
    width: 180, height: 80,
    fontsize: 18,
    color: "white",
    bold: true,
    solidcolor: "rgba(255,100,100,255)",
    solidcolor2: "rgba(100,100,255,255)",
    gradientangle: 135,
    solidcolorradius: 15,
    beveltype: 1,
    bevelwidth: 4,
    bevelcolor1: "rgba(255,255,255,220)",
    bevelcolor2: "rgba(0,0,0,180)",
    rotate: 15
});

// Image with gradient background + bevel
w.addImage({
    id: "img_combined",
    path: "D:\\GITHUB\\Novadesk\\assets\\Novadesk.ico",
    x: 280, y: 580,
    width: 100, height: 80,
    solidcolor: "rgba(255,200,100,255)",
    solidcolor2: "rgba(100,200,255,255)",
    gradientangle: 90,
    beveltype: 2,
    bevelwidth: 3,
    rotate: -10
});

novadesk.log("Enhanced Graphics Features Test Loaded Successfully!");
