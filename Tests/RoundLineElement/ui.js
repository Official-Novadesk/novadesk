// RoundLineElement Test UI

win.addRoundLine({
    id: "ring_bg_trail",
    x: 20,
    y: 20,
    width: 150,
    height: 150,
    radius: 60,
    thickness: 10,
    lineColor: "rgba(0, 255, 255, 1)",
    lineColorBg: "rgba(255, 255, 255, 0.1)",
    value: 0.6,
    capType: "round"
});

win.addRoundLine({
    id: "ring_counter",
    x: 180,
    y: 20,
    width: 150,
    height: 150,
    radius: 60,
    thickness: 10,
    lineColor: "rgba(255, 100, 0, 1)",
    clockwise: false,
    value: 0.4,
    capType: "round"
});

win.addRoundLine({
    id: "ring_dashed",
    x: 340,
    y: 20,
    width: 150,
    height: 150,
    radius: 60,
    thickness: 5,
    lineColor: "rgba(255, 255, 0, 1)",
    dashArray: "10, 5",
    value: 1.0,
    onLeftMouseDown: function() {
        console.log("Dashed ring clicked!");
    }
});

win.addRoundLine({
    id: "ring_tapered",
    x: 20,
    y: 180,
    width: 150,
    height: 150,
    radius: 60,
    thickness: 2,
    endThickness: 20,
    lineColor: "rgba(255, 0, 255, 1)",
    value: 0.8,
    capType: "round"
});

win.addRoundLine({
    id: "ring_ticks",
    x: 180,
    y: 180,
    width: 150,
    height: 150,
    radius: 60,
    thickness: 8,
    lineColor: "rgba(100, 255, 100, 1)",
    ticks: 12,
    value: 0.5
});

win.addText({
    id: "label",
    x: 400,
    y: 180,
    width: 240,
    height: 150,
    text: "Advanced\nRoundLine\nFeatures",
    fontColor: "rgba(255, 255, 255, 1)",
    fontSize: 20,
    textAlign: "center"
});

ipc.on("update-ring", function(data) {
    win.setElementProperties("ring_bg_trail", { value: data });
    win.setElementProperties("ring_counter", { value: data });
    win.setElementProperties("ring_dashed", { value: data });
    win.setElementProperties("ring_tapered", { value: data });
    win.setElementProperties("ring_ticks", { value: data });
});
