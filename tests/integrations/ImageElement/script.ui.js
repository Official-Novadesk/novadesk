ui.beginUpdate();
// ImageElement Test Suite
// Comprehensive test of all ImageElement features and options

// === SECTION 1: BASIC IMAGE LOADING ===
// Test 1: Basic image with default settings
ui.addImage({
    id: "basic-image",
    path: "../assets/pic.png",
    x: 20,
    y: 20,
    width: 120,
    height: 120
});
ui.addText({
    id: "label-basic",
    text: "Basic Image\nDefault settings",
    x: 30,
    y: 150,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 2: ASPECT RATIO CONTROL ===
// Test 2: Stretch (default) - image stretched to fill dimensions
ui.addImage({
    id: "aspect-stretch",
    path: "../assets/pic.png",
    x: 180,
    y: 20,
    width: 150,
    height: 100,
    preserveAspectRatio: "stretch"
});
ui.addText({
    id: "label-stretch",
    text: "Stretch Mode\nDistorts to fit",
    x: 190,
    y: 130,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 3: Preserve aspect ratio - maintains proportions
ui.addImage({
    id: "aspect-preserve",
    path: "../assets/pic.png",
    x: 360,
    y: 20,
    width: 150,
    height: 100,
    preserveAspectRatio: "preserve"
});
ui.addText({
    id: "label-preserve",
    text: "Preserve Mode\nMaintains aspect",
    x: 370,
    y: 130,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 4: Crop mode - fills area by cropping
ui.addImage({
    id: "aspect-crop",
    path: "../assets/pic.png",
    x: 540,
    y: 20,
    width: 150,
    height: 100,
    preserveAspectRatio: "crop"
});
ui.addText({
    id: "label-crop",
    text: "Crop Mode\nFills by cropping",
    x: 550,
    y: 130,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 3: TRANSPARENCY AND OPACITY ===
// Test 5: Full opacity (255)
ui.addImage({
    id: "alpha-255",
    path: "../assets/pic.png",
    x: 20,
    y: 200,
    width: 100,
    height: 100,
    imageAlpha: 255
});
ui.addText({
    id: "label-alpha-255",
    text: "Alpha: 255\n(Full opacity)",
    x: 25,
    y: 310,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 6: 50% opacity (128)
ui.addImage({
    id: "alpha-128",
    path: "../assets/pic.png",
    x: 150,
    y: 200,
    width: 100,
    height: 100,
    imageAlpha: 128
});
ui.addText({
    id: "label-alpha-128",
    text: "Alpha: 128\n(50% opacity)",
    x: 155,
    y: 310,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 7: Low opacity (64)
ui.addImage({
    id: "alpha-64",
    path: "../assets/pic.png",
    x: 280,
    y: 200,
    width: 100,
    height: 100,
    imageAlpha: 64
});
ui.addText({
    id: "label-alpha-64",
    text: "Alpha: 64\n(25% opacity)",
    x: 285,
    y: 310,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 4: COLOR EFFECTS ===
// Test 8: Grayscale filter
ui.addImage({
    id: "grayscale",
    path: "../assets/pic.png",
    x: 410,
    y: 200,
    width: 100,
    height: 100,
    grayscale: true
});
ui.addText({
    id: "label-grayscale",
    text: "Grayscale: true\nBlack & white",
    x: 415,
    y: 310,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 9: Color tint - red
ui.addImage({
    id: "tint-red",
    path: "../assets/pic.png",
    x: 540,
    y: 200,
    width: 100,
    height: 100,
    imageTint: "rgb(255, 0, 0)"
});
ui.addText({
    id: "label-tint-red",
    text: "Tint: Red\nrgb(255, 0, 0)",
    x: 545,
    y: 310,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 10: Color tint - blue
ui.addImage({
    id: "tint-blue",
    path: "../assets/pic.png",
    x: 20,
    y: 380,
    width: 100,
    height: 100,
    imageTint: "rgb(0, 0, 255)"
});
ui.addText({
    id: "label-tint-blue",
    text: "Tint: Blue\nrgb(0, 0, 255)",
    x: 25,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 11: Color tint - green
ui.addImage({
    id: "tint-green",
    path: "../assets/pic.png",
    x: 150,
    y: 380,
    width: 100,
    height: 100,
    imageTint: "rgb(0, 255, 0)"
});
ui.addText({
    id: "label-tint-green",
    text: "Tint: Green\nrgb(0, 255, 0)",
    x: 155,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 5: ADVANCED TRANSFORMATIONS ===
// Test 12: Tiling - repeat image
ui.addImage({
    id: "tile",
    path: "../assets/pic2.png",
    x: 280,
    y: 380,
    width: 150,
    height: 100,
    tile: true
});
ui.addText({
    id: "label-tile",
    text: "Tile: true\nRepeated pattern",
    x: 285,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 13: Color matrix - custom grayscale
ui.addImage({
    id: "color-matrix",
    path: "../assets/pic.png",
    x: 460,
    y: 380,
    width: 100,
    height: 100,
    colorMatrix: [
        0.33, 0.33, 0.33, 0, 0,
        0.59, 0.59, 0.59, 0, 0,
        0.11, 0.11, 0.11, 0, 0,
        0,    0,    0,    1, 0,
        0,    0,    0,    0, 1
    ]
});
ui.addText({
    id: "label-color-matrix",
    text: "Color Matrix\nCustom grayscale",
    x: 465,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 14: Transform matrix - skew effect
ui.addImage({
    id: "transform-matrix",
    path: "../assets/pic.png",
    x: 590,
    y: 380,
    width: 100,
    height: 100,
    transformMatrix: [1, 0.3, 0.2, 1, 0, 0]
});
ui.addText({
    id: "label-transform",
    text: "Transform Matrix\nSkew effect",
    x: 595,
    y: 490,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 6: COMBINATION EFFECTS ===
// Test 15: Grayscale + tint + alpha
ui.addImage({
    id: "combined-1",
    path: "../assets/pic.png",
    x: 20,
    y: 560,
    width: 100,
    height: 100,
    grayscale: true,
    imageTint: "rgb(255, 165, 0)",
    imageAlpha: 192
});
ui.addText({
    id: "label-combined-1",
    text: "Combined:\nGrayscale + Tint + Alpha",
    x: 25,
    y: 670,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 16: Preserve aspect + tint
ui.addImage({
    id: "combined-2",
    path: "../assets/pic.png",
    x: 150,
    y: 560,
    width: 150,
    height: 100,
    preserveAspectRatio: "preserve",
    imageTint: "rgb(255, 0, 255)"
});
ui.addText({
    id: "label-combined-2",
    text: "Combined:\nPreserve + Tint",
    x: 195,
    y: 670,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 17: Crop + color matrix (brightness)
ui.addImage({
    id: "combined-3",
    path: "../assets/pic.png",
    x: 330,
    y: 560,
    width: 150,
    height: 100,
    preserveAspectRatio: "crop",
    colorMatrix: [
        1, 0, 0, 0, 0.2,
        0, 1, 0, 0, 0.1,
        0, 0, 1, 0, 0.3,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1
    ]
});
ui.addText({
    id: "label-combined-3",
    text: "Combined:\nCrop + Brightness Matrix",
    x: 335,
    y: 670,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 18: Tile + grayscale
ui.addImage({
    id: "combined-4",
    path: "../assets/pic2.png",
    x: 510,
    y: 560,
    width: 150,
    height: 100,
    tile: true,
    grayscale: true
});
ui.addText({
    id: "label-combined-4",
    text: "Combined:\nTile + Grayscale",
    x: 515,
    y: 670,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// === SECTION 7: GENERAL ELEMENT OPTIONS ===
// Test 19: Solid color background
ui.addImage({
    id: "solid-color",
    path: "../assets/pic.png",
    x: 20,
    y: 740,
    width: 100,
    height: 100,
    backgroundColor: "rgb(255, 0, 0)"
});
ui.addText({
    id: "label-solid",
    text: "Solid Color\nBackground",
    x: 25,
    y: 850,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 20: Rounded corners
ui.addImage({
    id: "rounded",
    path: "../assets/pic.png",
    x: 150,
    y: 740,
    width: 100,
    height: 100,
    backgroundColor: "rgb(0, 255, 0)",
    backgroundColorRadius: 15
});
ui.addText({
    id: "label-rounded",
    text: "Rounded Corners\nRadius: 15",
    x: 155,
    y: 850,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 21: Rotation
ui.addImage({
    id: "rotated",
    path: "../assets/pic.png",
    x: 280,
    y: 740,
    width: 100,
    height: 100,
    rotate: 45
});
ui.addText({
    id: "label-rotated",
    text: "Rotation\n45 degrees",
    x: 285,
    y: 850,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 22: Bevel effect - raised
ui.addImage({
    id: "bevel-raised",
    path: "../assets/pic.png",
    x: 410,
    y: 740,
    width: 100,
    height: 100,
    bevelType: "raised",
    bevelWidth: 3,
    bevelColor: "rgb(255, 255, 255)",
    bevelColor2: "rgb(0, 0, 0)"
});
ui.addText({
    id: "label-bevel-raised",
    text: "Bevel: Raised\n3D effect",
    x: 415,
    y: 850,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 23: Bevel effect - sunken
ui.addImage({
    id: "bevel-sunken",
    path: "../assets/pic.png",
    x: 540,
    y: 740,
    width: 100,
    height: 100,
    bevelType: "sunken",
    bevelWidth: 3,
    bevelColor: "rgb(0, 0, 0)",
    bevelColor2: "rgb(255, 255, 255)"
});
ui.addText({
    id: "label-bevel-sunken",
    text: "Bevel: Sunken\n3D inset",
    x: 545,
    y: 850,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 24: No anti-aliasing
ui.addImage({
    id: "no-antialias",
    path: "../assets/pic.png",
    x: 20,
    y: 920,
    width: 100,
    height: 100,
    antiAlias: false,
    rotate: 30
});
ui.addText({
    id: "label-no-aa",
    text: "Anti-alias: false\nAliased edges",
    x: 25,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 25: ImageFlip modes
ui.addImage({
    id: "flip-horizontal",
    path: "../assets/pic.png",
    x: 150,
    y: 920,
    width: 100,
    height: 100,
    imageFlip: "horizontal"
});
ui.addText({
    id: "label-flip-horizontal",
    text: "ImageFlip:\nhorizontal",
    x: 155,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

ui.addImage({
    id: "flip-vertical",
    path: "../assets/pic.png",
    x: 280,
    y: 920,
    width: 100,
    height: 100,
    imageFlip: "vertical"
});
ui.addText({
    id: "label-flip-vertical",
    text: "ImageFlip:\nvertical",
    x: 285,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

ui.addImage({
    id: "flip-both",
    path: "../assets/pic.png",
    x: 410,
    y: 920,
    width: 100,
    height: 100,
    imageFlip: "both"
});
ui.addText({
    id: "label-flip-both",
    text: "ImageFlip:\nboth",
    x: 415,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

ui.addImage({
    id: "flip-none",
    path: "../assets/pic.png",
    x: 540,
    y: 920,
    width: 100,
    height: 100,
    imageFlip: "none"
});
ui.addText({
    id: "label-flip-none",
    text: "ImageFlip:\nnone",
    x: 545,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 26: EXIF orientation toggle
ui.addImage({
    id: "exif-orientation",
    path: "../assets/pic.png",
    x: 670,
    y: 920,
    width: 100,
    height: 100,
    useExifOrientation: true
});
ui.addText({
    id: "label-exif-orientation",
    text: "useExifOrientation:\ntrue",
    x: 675,
    y: 1030,
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

// Test 27: ImageCrop [X, Y, W, H, Origin]
// Origin: 0=TL, 1=TR, 2=BR, 3=BL, 4=CENTER
ui.addImage({
    id: "crop-center-auto-size",
    path: "../assets/pic.png",
    x: 790,
    y: 920,
    imageCrop: [-50,-30,100,60,5]
});
ui.addText({
    id: "label-crop-center-auto-size",
    text: "ImageCrop:\n[-30,-20,60,40,4]",
    x: 770,
    y: 975,
    fontSize: 12,
    fontColor: "rgb(255,255,255)"
});

// Test 28: ScaleMargins [Left, Top, Right, Bottom]
// Active when Tile=false and PreserveAspectRatio=stretch
ui.addImage({
    id: "scale-margins",
    path: "../assets/pic.png",
    x: 20,
    y: 1040,
    width: 220,
    height: 50,
    preserveAspectRatio: "stretch",
    tile: false,
    scaleMargins: [20, 20, 20, 20]
});
ui.addText({
    id: "label-scale-margins",
    text: "ScaleMargins:\n[20,20,20,20]",
    x: 20,
    y: 1068,
    fontSize: 12,
    fontColor: "rgb(255,255,255)"
});

ui.endUpdate();
