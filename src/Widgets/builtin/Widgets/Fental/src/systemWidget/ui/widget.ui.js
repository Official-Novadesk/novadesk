ui.beginUpdate();

// Background Shape
ui.addShape({
    id: "backgroundShape",
    type: "rectangle",
    x: 1,
    y: 1,
    width: 210,
    height: 120,
    fillColor: "rgba(27 ,27 ,31,0.8)",
    strokeColor: "rgba(200,200,200,0.5)",
    radius: 10,
    strokeWidth: 2
})

// Title Text
ui.addText({
    id: "title_Text",
    x: 106,
    y: 10,
    text: "System",
    fontSize: 25,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

/*
** CPU Content
**
*/
ui.addText({
    id: "cpu_Label",
    x: 15,
    y: 45,
    text: "CPU:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})


ui.addText({
    id: "cpu_Text",
    x: 195,
    y: 45,
    text: "--%",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

ui.addBar({
    id: "cpu-Bar",
    x: 15,
    y: 65,
    width: 180,
    height: 8,
    value: 0,
    barColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
    backgroundColor: "rgba(200,200,200,0.5)",
    backgroundColorRadius: 4,
    barCornerRadius: 4
});

/*
** CPU Content
**
*/

ui.addText({
    id: "memory_Label",
    x: 15,
    y: 85,
    text: "Memory:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})

ui.addText({
    id: "memory_Text",
    x: 195,
    y: 85,
    text: "--%",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

ui.addBar({
    id: "memory-Bar",
    x: 15,
    y: 105,
    width: 180,
    height: 8,
    value: 0,
    barColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
    backgroundColor: "rgba(200,200,200,0.5)",
    backgroundColorRadius: 4,
    barCornerRadius: 4
});

ipcRenderer.on("system-stats", function (event, data) {
    ui.setElementProperties("cpu_Text", { "text": data.cpu + "%" });
    ui.setElementProperties("cpu-Bar", { "value": data.cpu / 100 });
    
    ui.setElementProperties("memory_Text", { "text": data.memory + "%" });
    ui.setElementProperties("memory-Bar", { "value": data.memory / 100 });
})

ui.endUpdate();
