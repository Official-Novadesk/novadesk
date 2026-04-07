ui.beginUpdate();

ui.addShape({
    id: "backgroundShape",
    type: "rectangle",
    x: 1,
    y: 1,
    width: 210,
    height: 70,
    fillColor: "rgba(27 ,27 ,31,0.8)",
    strokeColor: "rgba(200,200,200,0.5)",
    radius: 10,
    strokeWidth: 2
})

ui.addText({
    id: "time_Text",
    x: 106,
    y: 10,
    text: "00:00",
    fontSize: 25,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

ui.addText({
    id: "day_Text",
    x: 15,
    y: 45,
    text: "---",
    fontSize: 15,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})

ui.addText({
    id: "date_Text",
    x: 195,
    y: 45,
    text: "--------, ---",
    fontSize: 15,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})


ipcRenderer.on("timeUpdate", function (event, timeStr) {
    ui.setElementProperties("time_Text", { "text": timeStr });
});

ipcRenderer.on("dayUpdate", function (event, dayStr) {
    ui.setElementProperties("day_Text", { "text": dayStr });
});

ipcRenderer.on("dateUpdate", function (event, dateStr) {
    ui.setElementProperties("date_Text", { "text": dateStr });
});

ui.endUpdate();
