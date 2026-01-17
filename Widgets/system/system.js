// =====================
// CONFIG
// =====================
var CONFIG = {
    background: {
        width: 210,
        height: 120,
        padding: 10,
        image: "../assets/background.png"
    },
    colors: {
        font: "rgb(255,255,255)",
        accent: "rgba(74, 81, 183, 1)",
        accent2: "rgba(35, 212, 201, 1)",
        subaccent: "rgba(255,255,255,0.2)"
    },
    font: {
        title: 18,
        text: 12
    },
    bar: {
        height: 8,
        radius: 8,
        gradientAngle: 45
    }
};

// Window reference
var ui = win;

// Pre-calc layout values
var PAD = CONFIG.background.padding;
var WIDTH = CONFIG.background.width;
var BAR_WIDTH = WIDTH - (PAD * 2);

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

function addValue(id, y) {
    ui.addText({
        id: id,
        text: "--%",
        x: WIDTH - PAD,
        y: y,
        fontSize: CONFIG.font.text,
        textAlign: "right",
        fontWeight: "bold",
        fontColor: CONFIG.colors.font
    });
}

function addBar(id, y) {
    ui.addBar({
        id: id,
        x: PAD,
        y: y,
        value: 0,
        width: BAR_WIDTH,
        height: CONFIG.bar.height,
        barColor: CONFIG.colors.accent,
        barColor2: CONFIG.colors.accent2,
        barGradientAngle: CONFIG.bar.gradientAngle,
        barCornerRadius: CONFIG.bar.radius,
        solidColor: CONFIG.colors.subaccent,
        solidColorRadius: CONFIG.bar.radius
    });
}

function clampPercent(v) {
    v = Number(v) || 0;
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

function updateMetric(valueId, barId, percent) {
    percent = clampPercent(percent);

    ui.setElementProperties(valueId, {
        text: percent + "%"
    });

    ui.setElementProperties(barId, {
        value: percent / 100
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
    text: "System",
    x: WIDTH / 2,
    y: PAD,
    fontSize: CONFIG.font.title,
    textAlign: "center",
    fontWeight: "bold",
    fontColor: CONFIG.colors.accent
});

// =====================
// CPU
// =====================
addLabel("cpu-label", "CPU:", 40);
addValue("cpu-value", 40);
addBar("cpu-bar", 60);

// =====================
// MEMORY
// =====================
addLabel("memory-label", "Memory:", 80);
addValue("memory-value", 80);
addBar("memory-bar", 100);

// =====================
// IPC
// =====================
ipc.on("cpu-usage", function (msg) {
    updateMetric("cpu-value", "cpu-bar", msg);
});

ipc.on("memory-usage", function (msg) {
    updateMetric("memory-value", "memory-bar", msg);
});
