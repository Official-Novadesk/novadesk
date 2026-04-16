// LineElement Test UI
// Covers: lineCount, data/data2..., lineColorN, lineWidth, horizontalLines,
// horizontalLineColor, graphStart, graphOrientation, flip, transformStroke.

function logAssert(name, actual, expected) {
    var ok = actual === expected;
    console.log((ok ? "[PASS] " : "[FAIL] ") + name + " | expected=" + expected + " actual=" + actual);
}

function makeWave(seed, len, amp, base) {
    var out = [];
    for (var i = 0; i < len; i++) {
        var v = base + Math.sin((i + seed) * 0.38) * amp;
        out.push(v);
    }
    return out;
}

var CPU_COLOR = "rgba(255, 96, 96, 1)";
var MEMORY_COLOR = "rgba(82, 200, 255, 1)";
var RX_COLOR = "rgba(130, 255, 130, 1)";
var TX_COLOR = "rgba(255, 196, 92, 1)";
var maxPoints = 10;

ui.beginUpdate();

ui.addText({
    id: "line-title",
    x: 20,
    y: 14,
    text: "LineElement Integration Test",
    fontSize: 24,
    fontColor: "rgba(230, 235, 255, 1)"
});

ui.addLine({
    id: "line-main",
    x: 20,
    y: 60,
    width: 520,
    height: 220,
    lineCount: 3,
    data: makeWave(1, 60, 18, 30),
    data2: makeWave(6, 60, 14, 24),
    data3: makeWave(13, 60, 20, 32),
    lineColor: "rgba(45, 196, 255, 1)",
    lineColor2: "rgba(255, 120, 70, 1)",
    lineColor3: "rgba(132, 255, 140, 1)",
    lineScale: 1.0,
    lineScale2: 0.75,
    lineScale3: 1.2,
    lineWidth: 3,
    horizontalLines: true,
    horizontalLineColor: "rgba(255, 255, 255, 0.16)",
    graphStart: "right",
    graphOrientation: "vertical",
    flip: false,
    transformStroke: "normal",
    backgroundColor: "rgba(255, 255, 255, 0.04)",
    backgroundColorRadius: 8,
    bevelType: "raised",
    maxPoints:10,
    bevelWidth: 1
});

ui.addText({
    id: "line-main-label",
    x: 20,
    y: 290,
    text: "line-main: vertical, start=right, 3 lines",
    fontSize: 14,
    fontColor: "rgba(215, 220, 235, 1)"
});

ui.addLine({
    id: "line-horizontal",
    x: 560,
    y: 60,
    width: 500,
    height: 220,
    lineCount: 2,
    data: makeWave(2, 80, 20, 30),
    data2: makeWave(9, 80, 10, 28),
    lineColor: "rgba(255, 210, 70, 1)",
    lineColor2: "rgba(170, 130, 255, 1)",
    lineWidth: 4,
    horizontalLines: true,
    horizontalLineColor: "rgba(255, 255, 255, 0.2)",
    graphStart: "left",
    graphOrientation: "horizontal",
    flip: true,
    transformStroke: "fixed",
    backgroundColor: "rgba(255, 255, 255, 0.04)",
    backgroundColorRadius: 8
});

ui.addText({
    id: "line-horizontal-label",
    x: 560,
    y: 290,
    text: "line-horizontal: horizontal, start=left, flip=true, fixed stroke",
    fontSize: 14,
    fontColor: "rgba(215, 220, 235, 1)"
});

ui.addText({
    id: "live-title",
    x: 20,
    y: 340,
    text: "Live CPU / Memory Usage",
    fontSize: 20,
    fontColor: "rgba(230, 235, 255, 1)"
});

ui.addLine({
    id: "line-live-usage",
    x: 20,
    y: 375,
    width: 1040,
    height: 300,
    lineCount: 2,
    data: [],
    data2: [],
    lineColor: CPU_COLOR,    // CPU
    lineColor2: MEMORY_COLOR, // Memory
    lineWidth: 3,
    maxPoints: maxPoints,
    horizontalLines: true,
    horizontalLineColor: "rgba(255, 255, 255, 0.18)",
    graphStart: "right",
    graphOrientation: "vertical",
    flip: false,
    transformStroke: "normal",
    autoRange: false,
    rangeMin: 0,
    rangeMax: 100,
    backgroundColor: "rgba(255, 255, 255, 0.04)",
    backgroundColorRadius: 10,
    bevelType: "raised",
    bevelWidth: 1
});

ui.addText({
    id: "live-legend",
    x: 20,
    y: 685,
    text: "CPU / Memory - 0% to 100%",
    fontSize: 14,
    fontColor: "rgba(215, 220, 235, 1)"
});

ui.addText({
    id: "live-cpu-value",
    x: 280,
    y: 685,
    text: "CPU: --%",
    fontSize: 14,
    fontColor: CPU_COLOR
});

ui.addText({
    id: "live-memory-value",
    x: 430,
    y: 685,
    text: "Memory: --%",
    fontSize: 14,
    fontColor: MEMORY_COLOR
});

ui.addText({
    id: "net-title",
    x: 20,
    y: 725,
    text: "Live Network In / Out",
    fontSize: 20,
    fontColor: "rgba(230, 235, 255, 1)"
});

ui.addLine({
    id: "line-live-network",
    x: 20,
    y: 760,
    width: 1040,
    height: 260,
    lineCount: 2,
    data: [],
    data2: [],
    lineColor: RX_COLOR, // Net In
    lineColor2: TX_COLOR, // Net Out
    lineWidth: 3,
    maxPoints: maxPoints,
    horizontalLines: true,
    horizontalLineColor: "rgba(255, 255, 255, 0.18)",
    graphStart: "left",
    graphOrientation: "vertical",
    flip: false,
    transformStroke: "normal",
    autoRange: true,
    backgroundColor: "rgba(255, 255, 255, 0.04)",
    backgroundColorRadius: 10,
    bevelType: "raised",
    bevelWidth: 1
});

ui.addText({
    id: "net-legend",
    x: 20,
    y: 1028,
    text: "RxSpeed / TxSpeed - normalized to visible scale",
    fontSize: 14,
    fontColor: "rgba(215, 220, 235, 1)"
});

ui.addText({
    id: "net-rx-value",
    x: 390,
    y: 1028,
    text: "Rx: --",
    fontSize: 14,
    fontColor: RX_COLOR
});

ui.addText({
    id: "net-tx-value",
    x: 610,
    y: 1028,
    text: "Tx: --",
    fontSize: 14,
    fontColor: TX_COLOR
});

ui.endUpdate();

// Property assertions for parser + getElementProperty mapping.
logAssert("lineCount", ui.getElementProperty("line-main", "lineCount"), 3);
logAssert("lineWidth", ui.getElementProperty("line-main", "lineWidth"), 3);
logAssert("horizontalLines", ui.getElementProperty("line-main", "horizontalLines"), true);
logAssert("graphStart", ui.getElementProperty("line-main", "graphStart"), "right");
logAssert("graphOrientation", ui.getElementProperty("line-main", "graphOrientation"), "vertical");
logAssert("flip", ui.getElementProperty("line-main", "flip"), false);
logAssert("transformStroke", ui.getElementProperty("line-horizontal", "transformStroke"), "fixed");
logAssert("lineScale", ui.getElementProperty("line-main", "lineScale"), 1);
logAssert("lineScale2", ui.getElementProperty("line-main", "lineScale2"), 0.75);
logAssert("lineScale3", ui.getElementProperty("line-main", "lineScale3"), 1.2);
logAssert("maxPoints", ui.getElementProperty("line-live-usage", "maxPoints"), maxPoints);
logAssert("autoRange", ui.getElementProperty("line-live-usage", "autoRange"), false);
logAssert("rangeMin", ui.getElementProperty("line-live-usage", "rangeMin"), 0);
logAssert("rangeMax", ui.getElementProperty("line-live-usage", "rangeMax"), 100);

var c1 = ui.getElementProperty("line-main", "lineColor");
var c2 = ui.getElementProperty("line-main", "lineColor2");
var d2 = ui.getElementProperty("line-main", "data2");
console.log("[INFO] lineColor=" + c1);
console.log("[INFO] lineColor2=" + c2);
console.log("[INFO] data2 length=" + (d2 ? d2.length : 0));

var cpuHistory = [];
var memoryHistory = [];
var netInHistory = [];
var netOutHistory = [];

function clampPercent(v) {
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

function normalizeTickPayload(payload) {
    if (payload && typeof payload === "object") {
        return payload;
    }

    if (typeof payload === "string") {
        try {
            var parsed = JSON.parse(payload);
            if (parsed && typeof parsed === "object") {
                return parsed;
            }
        } catch (e) {
            // Ignore and fall back.
        }
    }

    // Legacy fallback if only one number arrives.
    if (typeof payload === "number") {
        return {
            cpu: payload,
            memory: 0
        };
    }

    return null;
}

function toNumber(v) {
    var n = Number(v);
    if (!isFinite(n) || isNaN(n)) return 0;
    return n;
}

function formatRate(v) {
    var n = toNumber(v);
    if (n < 1024) return n.toFixed(0) + " B/s";
    if (n < 1024 * 1024) return (n / 1024).toFixed(1) + " KB/s";
    return (n / (1024 * 1024)).toFixed(2) + " MB/s";
}

ipcRenderer.on("line:tick", function (event, payloadArg) {
    var rawPayload = (payloadArg === undefined) ? event : payloadArg;
    var payload = normalizeTickPayload(rawPayload);
    if (!payload) return;

    var cpu = clampPercent(toNumber(payload.cpu));
    var memory = clampPercent(toNumber(payload.memory));
    var netIn = Math.max(0, toNumber(payload.netIn));
    var netOut = Math.max(0, toNumber(payload.netOut));

    cpuHistory.push(cpu);
    memoryHistory.push(memory);
    netInHistory.push(netIn);
    netOutHistory.push(netOut);
    if (cpuHistory.length > maxPoints) cpuHistory.shift();
    if (memoryHistory.length > maxPoints) memoryHistory.shift();
    if (netInHistory.length > maxPoints) netInHistory.shift();
    if (netOutHistory.length > maxPoints) netOutHistory.shift();

    // Normalize net data to 0-100 for visual clarity across different traffic levels.
    var netMax = 1;
    for (var i = 0; i < netInHistory.length; i++) {
        if (netInHistory[i] > netMax) netMax = netInHistory[i];
    }
    for (var j = 0; j < netOutHistory.length; j++) {
        if (netOutHistory[j] > netMax) netMax = netOutHistory[j];
    }
    var netInNormalized = [];
    var netOutNormalized = [];
    for (var k = 0; k < netInHistory.length; k++) {
        netInNormalized.push((netInHistory[k] / netMax) * 100.0);
    }
    for (var m = 0; m < netOutHistory.length; m++) {
        netOutNormalized.push((netOutHistory[m] / netMax) * 100.0);
    }

    ui.setElementProperties("line-live-usage", {
        data: cpuHistory,
        data2: memoryHistory
    });
    ui.setElementProperties("line-live-network", {
        data: netInNormalized,
        data2: netOutNormalized
    });

    ui.setElementProperties("live-cpu-value", {
        text: "CPU: " + cpu.toFixed(1) + "%"
    });
    ui.setElementProperties("live-memory-value", {
        text: "Memory: " + memory.toFixed(1) + "%"
    });
    ui.setElementProperties("net-rx-value", {
        text: "Rx: " + formatRate(netIn)
    });
    ui.setElementProperties("net-tx-value", {
        text: "Tx: " + formatRate(netOut)
    });

    if ((payload.phase || 0) % 5 === 0) {
        console.log("[LIVE UI] cpu=" + cpu.toFixed(2) + " memory=" + memory.toFixed(2) +
            " netIn=" + formatRate(netIn) + " netOut=" + formatRate(netOut) +
            " points=" + cpuHistory.length);
    }
});
