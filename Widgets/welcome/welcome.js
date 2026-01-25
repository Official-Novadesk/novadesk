// =====================
// CONFIG
// =====================
var CONFIG = {
    background: {
        width: 400,
        height: 200,
        padding: 10,
        image: "../assets/background.png"
    },
    colors: {
        font: "rgb(255,255,255)",
        accent: "rgba(74, 81, 183, 1)",
        hover: "rgba(100, 110, 255, 1)",
        hoverFont: "rgb(255,255,255)" 
    },
    font: {
        title: 20,
        text: 12
    }
};

// =====================
// WINDOW & LAYOUT
// =====================
var ui = win;

var PAD = CONFIG.background.padding;
var WIDTH = CONFIG.background.width;

// =====================
// Welcome Widget UI Script
// =====================

win.addImage({
    id: "background_Image",
    width: WIDTH,
    height: CONFIG.background.height,
    path: CONFIG.background.image
});

win.addImage({
    id: "logo",
    path: "../assets/Novadesk.png",
    x: 1,
    y: 10,
    width: 180,
    height: 180,
});

win.addText({
    id: "title",
    text: "Welcome to Novadesk",
    x: 160,
    y: 25,
    fontSize: CONFIG.font.title,
    fontWeight:"bold",
    fontColor: CONFIG.colors.accent,
    textAlign: "left"
});

win.addText({
    id: "description",
    text: "Novadesk a desktop app for windows.Customize your desktop with widgets.Make widgets for your needs.",
    x: 160,
    y: 70,
    fontSize: CONFIG.font.text,
    clipString: "wrap",
    width: 220,
    fontColor: CONFIG.colors.font,
    textAlign: "left"
});

win.addText({
    id: "website-button",
    text: "Website",
    x: 210,
    y: 170,
    fontSize: CONFIG.font.text,
    fontColor: CONFIG.colors.font,
    width: 100,
    height: 25,
    solidColor: CONFIG.colors.accent,
    solidColorRadius: 5,
    textAlign: "centercenter",
    onMouseOver: function () {
        win.setElementProperties("website-button", { 
            solidColor: CONFIG.colors.hover, 
            fontColor: CONFIG.colors.hoverFont 
        });
    },
    onMouseLeave: function () {
        win.setElementProperties("website-button", { 
            solidColor: CONFIG.colors.accent, 
            fontColor: CONFIG.colors.font 
        });
    },
    onLeftMouseUp: function () {
        ipc.send("executeURL", "https://novadesk.pages.dev");
    }
});

win.addText({
    id: "docs-button",
    text: "Docs",
    x: 320,
    y: 170,
    fontSize: CONFIG.font.text,
    fontColor: CONFIG.colors.font,
    width: 100,
    height: 25,
    solidColor: CONFIG.colors.accent,
    solidColorRadius: 5,
    textAlign: "centercenter",
    onMouseOver: function () {
        win.setElementProperties("docs-button", { 
            solidColor: CONFIG.colors.hover, 
            fontColor: CONFIG.colors.hoverFont 
        });
    },
    onMouseLeave: function () {
        win.setElementProperties("docs-button", { 
            solidColor: CONFIG.colors.accent, 
            fontColor: CONFIG.colors.font 
        });
    },
    onLeftMouseUp: function () {
        ipc.send("executeURL", "https://novadesk-docs.pages.dev/");
    }
});