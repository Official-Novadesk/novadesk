// =====================
// CONFIG
// =====================
var CONFIG = {
    background: {
        width: 210,
        height: 90,
        padding: 10,
        image: "../assets/background.png"
    },
    colors: {
        font: "rgb(255,255,255)",
        accent: "rgba(74, 81, 183, 1)"
    },
    font: {
        title: 18,
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
// UI HELPERS
// =====================
function addLabel(id, text, y) {
    ui.addText({
        id: id,
        text: text,
        x: PAD,
        y: y,
        fontSize: CONFIG.font.text,
        textAlign: "left",
        fontWeight: "bold",
        fontColor: CONFIG.colors.font
    });
}

function addValue(id, suffix, y) {
    ui.addText({
        id: id,
        text: "-- " + suffix,
        x: WIDTH - PAD,
        y: y,
        fontSize: CONFIG.font.text,
        textAlign: "right",
        fontWeight: "bold",
        fontColor: CONFIG.colors.font
    });
}

function formatSpeed(v) {
    v = Number(v) || 0;

    if (v >= 1024) {
        return (v / 1024).toFixed(1) + " MB/s";
    }
    return Math.round(v) + " KB/s";
}

function updateValue(id, value) {
    ui.setElementProperties(id, {
        text: formatSpeed(value)
    });
}

// =====================
// BACKGROUND & TITLE
// =====================
ui.addImage({
    id: "background_Image",
    width: WIDTH,
    height: CONFIG.background.height,
    path: CONFIG.background.image
});

ui.addText({
    id: "title",
    text: "Network",
    x: WIDTH / 2,
    y: PAD,
    fontSize: CONFIG.font.title,
    textAlign: "center",
    fontWeight: "bold",
    fontColor: CONFIG.colors.accent
});

// =====================
// NETWORK METRICS
// =====================
addLabel("download-label", "Download:", 40);
addValue("download-value", "KB/s", 40);

addLabel("upload-label", "Upload:", 60);
addValue("upload-value", "KB/s", 60);

// =====================
// IPC
// =====================
ipc.on("net-in", function (msg) {
    updateValue("download-value", msg);
});

ipc.on("net-out", function (msg) {
    updateValue("upload-value", msg);
});
