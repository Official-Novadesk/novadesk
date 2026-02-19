// Background Shape
win.addShape({
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
win.addText({
    id: "title_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
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
win.addText({
    id: "cpu_Label",
    x: 15,
    y: 45,
    text: "CPU:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})


win.addText({
    id: "cpu_Text",
    x: 195,
    y: 45,
    text: "--%",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

win.addBar({
    id: "cpu-Bar",
    x: 15,
    y: 65,
    width: win.getElementProperty("backgroundShape", "width") - 30,
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

win.addText({
    id: "memory_Label",
    x: 15,
    y: 85,
    text: "Memory:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})

win.addText({
    id: "memory_Text",
    x: 195,
    y: 85,
    text: "--%",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

win.addBar({
    id: "memory-Bar",
    x: 15,
    y: 105,
    width: win.getElementProperty("backgroundShape", "width") - 30,
    height: 8,
    value: 0,
    barColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
    backgroundColor: "rgba(200,200,200,0.5)",
    backgroundColorRadius: 4,
    barCornerRadius: 4
});

ipc.on("system-stats", function (data) {
    win.setElementProperties("cpu_Text", { "text": data.cpu + "%" });
    win.setElementProperties("cpu-Bar", { "value": data.cpu / 100 });
    
    win.setElementProperties("memory_Text", { "text": data.memory + "%" });
    win.setElementProperties("memory-Bar", { "value": data.memory / 100 });
})
