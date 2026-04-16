console.log("=== HistogramElement UI Started ===");

function clampPercent(v) {
    let n = Number(v);
    if (!isFinite(n) || isNaN(n)) n = 0;
    if (n < 0) n = 0;
    if (n > 100) n = 100;
    return n;
}

function parsePayload(raw) {
    if (raw && typeof raw === "object") return raw;
    if (typeof raw === "string") {
        try {
            return JSON.parse(raw);
        } catch (e) {
            return null;
        }
    }
    return null;
}

function assertEq(name, actual, expected) {
    const ok = actual === expected;
    console.log((ok ? "[PASS] " : "[FAIL] ") + name + " expected=" + expected + " actual=" + actual);
}

ui.beginUpdate();

ui.addText({
    id: "hist-title",
    x: 20,
    y: 14,
    text: "Histogram Element Test",
    fontSize: 24,
    fontColor: "rgb(236,243,251)"
});

ui.addText({
    id: "hist-subtitle",
    x: 20,
    y: 46,
    width: 940,
    text: "Top: CPU vs Memory overlap. Bottom: CPU autoRange horizontal.",
    fontSize: 13,
    fontColor: "rgba(208,220,232,0.95)"
});

ui.addHistogram({
    id: "hist-main",
    x: 20,
    y: 80,
    width: 920,
    height: 260,
    data: [],
    data2: [],
    autoRange: false,
    graphStart: "right",
    graphOrientation: "vertical",
    flip: false,
    primaryColor: "rgba(46,184,255,0.95)",
    secondaryColor: "rgba(255,110,110,0.90)",
    bothColor: "rgba(255,232,120,0.95)",
    backgroundColor: "rgba(255,255,255,0.04)",
    backgroundColorRadius: 8,
    bevelType: "raised",
    bevelWidth: 1
});

ui.addText({
    id: "hist-main-label",
    x: 20,
    y: 348,
    text: "hist-main (vertical, graphStart=right, fixed 0..100 range)",
    fontSize: 13,
    fontColor: "rgba(212,224,236,0.95)"
});

ui.addHistogram({
    id: "hist-horizontal",
    x: 20,
    y: 380,
    width: 920,
    height: 220,
    data: [],
    autoRange: true,
    graphStart: "left",
    graphOrientation: "horizontal",
    flip: false,
    primaryColor: "rgba(110,255,152,0.95)",
    backgroundColor: "rgba(255,255,255,0.04)",
    backgroundColorRadius: 8
});

ui.addText({
    id: "hist-live",
    x: 20,
    y: 618,
    width: 940,
    text: "CPU: --  Memory: --",
    fontSize: 18,
    fontColor: "rgb(255,156,156)"
});

ui.endUpdate();

assertEq("graphStart", ui.getElementProperty("hist-main", "graphStart"), "right");
assertEq("graphOrientation", ui.getElementProperty("hist-main", "graphOrientation"), "vertical");
assertEq("autoRange", ui.getElementProperty("hist-main", "autoRange"), false);
assertEq("horizontal.autoRange", ui.getElementProperty("hist-horizontal", "autoRange"), true);

const cpuHistory = [];
const memoryHistory = [];
const maxPoints = 920;

ipcRenderer.on("histogram:tick", function (event, payloadArg) {
    const rawPayload = (payloadArg === undefined) ? event : payloadArg;
    const payload = parsePayload(rawPayload);
    if (!payload) return;

    const cpu = clampPercent(payload.cpu);
    const memory = clampPercent(payload.memory);
    const tick = Number(payload.tick || 0);

    cpuHistory.push(cpu);
    memoryHistory.push(memory);
    if (cpuHistory.length > maxPoints) cpuHistory.shift();
    if (memoryHistory.length > maxPoints) memoryHistory.shift();

    ui.setElementProperties("hist-main", {
        data: cpuHistory,
        data2: memoryHistory
    });

    ui.setElementProperties("hist-horizontal", {
        data: cpuHistory
    });

    ui.setElementProperties("hist-live", {
        text: "CPU: " + cpu.toFixed(2) + "%   Memory: " + memory.toFixed(2) + "%   (tick " + tick + ")"
    });
});

