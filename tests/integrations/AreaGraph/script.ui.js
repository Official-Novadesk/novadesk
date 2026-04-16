console.log("=== AreaGraph UI Started ===");

ui.beginUpdate();

ui.addText({
    id: "label",
    x: 20,
    y: 20,
    text: "AreaGraph Live CPU Monitor",
    fontSize: 20,
    fontColor: "#ffffff"
});

ui.addAreaGraph({
    id: "cpu-graph",
    x: 20,
    y: 60,
    width: 560,
    height: 200,
    data: [],
    minValue: 0,
    maxValue: 100,
    autoRange: false,
    lineColor: "#00b4ff",
    lineAlpha: 255,
    lineWidth: 2,
    fillColor: "#00b3ff47",
    fillAlpha: 80,
    gridColor: "#ffffff",
    gridAlpha: 40,
    gridX: 30,
    gridY: 20,
    maxPoints: 20,
    graphStart: "right"
});

ui.addText({
    id: "status",
    x: 20,
    y: 270,
    text: "Waiting for data...",
    fontSize: 14,
    fontColor: "#cccccc"
});

ui.endUpdate();

const cpuHistory = [];
// const maxPoints = 20;

ipcRenderer.on("graph:tick", function (event, payloadArg) {
    const rawPayload = (payloadArg === undefined) ? event : payloadArg;
    let payload;
    try {
        payload = JSON.parse(rawPayload);
    } catch (e) {
        return;
    }

    const cpu = payload.cpu;
    cpuHistory.push(cpu);
    // if (cpuHistory.length > maxPoints) cpuHistory.shift();

    ui.setElementProperties("cpu-graph", {
        data: cpuHistory
    });

    ui.setElementProperties("status", {
        text: "CPU Usage: " + cpu.toFixed(1) + "% (" + cpuHistory.length + " points)"
    });
});
