console.log("=== RotatorElement UI Started ===");

function assertEq(name, actual, expected) {
    const ok = actual === expected;
    console.log((ok ? "[PASS] " : "[FAIL] ") + name + " expected=" + expected + " actual=" + actual);
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

function clampCpu(v) {
    let n = Number(v);
    if (!isFinite(n) || isNaN(n)) n = 0;
    if (n < 0) n = 0;
    if (n > 100) n = 100;
    return n;
}

ui.beginUpdate();

ui.addText({
    id: "rotator-title",
    x: 20,
    y: 14,
    text: "Rotator Element Test",
    fontSize: 22,
    fontColor: "rgb(236,243,251)"
});

ui.addText({
    id: "rotator-subtitle",
    x: 20,
    y: 44,
    width: 480,
    text: "Needle rotates with CPU usage. 0% = left, 50% = top, 100% = right.",
    fontSize: 13,
    fontColor: "rgba(212,224,236,0.9)"
});

// Dial background
ui.addShape({
    id: "dial-circle",
    shapeType: "ellipse",
    x: 136,
    y: 74,
    width: 248,
    height: 248,
    fillColor: "rgba(255,255,255,0.03)",
    strokeColor: "rgba(215,228,240,0.55)",
    strokeWidth: 2
});

ui.addText({
    id: "dial-left",
    x: 124,
    y: 186,
    text: "0",
    fontSize: 14,
    fontColor: "rgba(230,239,248,0.9)"
});

ui.addText({
    id: "dial-top",
    x: 252,
    y: 92,
    text: "50",
    fontSize: 14,
    fontColor: "rgba(230,239,248,0.9)"
});

ui.addText({
    id: "dial-right",
    x: 388,
    y: 186,
    text: "100",
    fontSize: 14,
    fontColor: "rgba(230,239,248,0.9)"
});

ui.addRotator({
    id: "cpu-needle",
    x: 150,
    y: 86,
    width: 220,
    height: 220,
    value: 0,
    rotatorImageName: "./assets/needle.png",
    offsetX: 110,
    offsetY: 110,
    startAngle: -2.35619449,   // -135 deg
    rotationAngle: 4.71238898, // +270 deg sweep
    maxValue: 100,
    imageAlpha: 255
});

ui.addText({
    id: "cpu-readout",
    x: 20,
    y: 318,
    width: 480,
    text: "CPU: 0.00%",
    textAlign: "left",
    fontSize: 18,
    fontColor: "rgb(255,122,122)"
});

ui.endUpdate();

// API assertions
assertEq("rotatorImageName exists", typeof ui.getElementProperty("cpu-needle", "rotatorImageName"), "string");
assertEq("maxValue", ui.getElementProperty("cpu-needle", "maxValue"), 100);
assertEq("offsetX", ui.getElementProperty("cpu-needle", "offsetX"), 110);
assertEq("offsetY", ui.getElementProperty("cpu-needle", "offsetY"), 110);

ipcRenderer.on("rotator:tick", function (event, payloadArg) {
    const raw = (payloadArg === undefined) ? event : payloadArg;
    const payload = parsePayload(raw);
    if (!payload) return;

    const cpu = clampCpu(payload.cpu);
    const tick = Number(payload.tick || 0);

    ui.setElementProperties("cpu-needle", {
        value: cpu
    });

    ui.setElementProperties("cpu-readout", {
        text: "CPU: " + cpu.toFixed(2) + "% (tick " + tick + ")"
    });
});
