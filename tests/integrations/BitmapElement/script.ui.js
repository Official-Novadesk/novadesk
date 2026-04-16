console.log("=== BitmapElement CPU UI Started ===");

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

function clampPercent(v) {
    var n = Number(v);
    if (!isFinite(n) || isNaN(n)) n = 0;
    if (n < 0) n = 0;
    if (n > 100) n = 100;
    return Math.round(n);
}

ui.beginUpdate();

ui.addText({
    id: "cpu-title",
    x: 22,
    y: 18,
    text: "CPU Usage (Bitmap Digits)",
    fontSize: 22,
    fontColor: "rgb(236,244,253)"
});

ui.addText({
    id: "cpu-help",
    x: 22,
    y: 50,
    width: 470,
    text: "Bitmap strip is 0-9. Value is rendered as 3 digits using BitmapExtend=true.",
    fontSize: 13,
    fontColor: "rgba(210,222,236,0.9)"
});

ui.addBitmap({
    id: "cpu-bitmap",
    x: 28,
    y: 95,
    value: 0,
    bitmapImageName: "./assets/digits-0-9.png",
    bitmapFrames: 10,
    bitmapZeroFrame: false,
    bitmapExtend: true,
    maxValue: 100, // Engine auto-normalizes: value/maxValue
    bitmapOrientation: "horizontal",
    bitmapDigits: 3,
    bitmapAlign: "left",
    bitmapSeparation: -8,
    imageTint: "rgba(230,245,255,1)",
    imageAlpha: 255
});

ui.addText({
    id: "cpu-percent-sign",
    x: 208,
    y: 116,
    text: "%",
    fontSize: 34,
    fontColor: "rgb(230,245,255)"
});

ui.addText({
    id: "cpu-readout",
    x: 22,
    y: 165,
    width: 460,
    text: "CPU: 0%",
    fontSize: 14,
    fontColor: "rgba(210,222,236,0.9)"
});

ui.endUpdate();

console.log("[PASS] bitmap created with digits image");
console.log("[PASS] bitmapFrames = " + ui.getElementProperty("cpu-bitmap", "bitmapFrames"));
console.log("[PASS] bitmapExtend = " + ui.getElementProperty("cpu-bitmap", "bitmapExtend"));
console.log("[PASS] bitmapSmoothRoll = " + ui.getElementProperty("cpu-bitmap", "bitmapSmoothRoll"));

// ipcRenderer.on("bitmap:cpu", function (event, payloadArg) {
//     var rawPayload = (payloadArg === undefined) ? event : payloadArg;
//     var payload = parsePayload(rawPayload);
//     if (!payload) return;

//     var usage = clampPercent(payload.usage);
//     var tick = Number(payload.tick || 0);

//     // Just pass the raw value — the engine handles normalization
//     // via maxValue when bitmapExtend=false
//     ui.setElementProperties("cpu-bitmap", {
//         value: usage
//     });

//     ui.setElementProperties("cpu-readout", {
//         text: "CPU: " + usage + "%   (tick " + tick + ")"
//     });
// });
