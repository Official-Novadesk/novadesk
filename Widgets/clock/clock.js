// =====================
// CONFIG
// =====================
var CONFIG = {
    background: {
        width: 210,
        height: 60,
        image: "../assets/background.png"
    },
    colors: {
        font: "rgb(255,255,255)",
        accent: "rgba(74, 81, 183, 1)"
    },
    font: {
        time: 18,
        text: 12
    }
};

var DAYS = [
    "Sunday", "Monday", "Tuesday",
    "Wednesday", "Thursday", "Friday", "Saturday"
];

// Capture window reference (important for callbacks)
var ui = win;

// =====================
// UI ELEMENTS
// =====================
ui.addImage({
    id: "background_Image",
    width: CONFIG.background.width,
    height: CONFIG.background.height,
    path: CONFIG.background.image
});

ui.addText({
    id: "time",
    text: "--:--:--",
    x: CONFIG.background.width / 2,
    y: 10,
    fontsize: CONFIG.font.time,
    textalign: "center",
    fontweight: "bold",
    fontcolor: CONFIG.colors.accent
});

ui.addText({
    id: "day",
    text: "-----",
    x: 10,
    y: 40,
    fontsize: CONFIG.font.text,
    textalign: "left",
    fontweight: "bold",
    fontcolor: CONFIG.colors.font
});

ui.addText({
    id: "date",
    text: "--/--/----",
    x: CONFIG.background.width - 10,
    y: 40,
    fontsize: CONFIG.font.text,
    textalign: "right",
    fontweight: "bold",
    fontcolor: CONFIG.colors.font
});

// =====================
// HELPERS (ES5 SAFE)
// =====================
function pad2(n) {
    return (n < 10) ? ("0" + n) : String(n);
}

function formatTime(d) {
    return pad2(d.getHours()) + ":" +
        pad2(d.getMinutes()) + ":" +
        pad2(d.getSeconds());
}

function formatDate(d) {
    return pad2(d.getDate()) + "/" +
        pad2(d.getMonth() + 1) + "/" +
        d.getFullYear();
}

// =====================
// UPDATE LOGIC
// =====================
function updateTime() {
    var now = new Date();

    ui.setElementProperties("time", {
        text: formatTime(now)
    });

    ui.setElementProperties("day", {
        text: DAYS[now.getDay()]
    });

    ui.setElementProperties("date", {
        text: formatDate(now)
    });
}

// =====================
// IPC
// =====================
ipc.on("update_Elements", function () {
    updateTime();
});