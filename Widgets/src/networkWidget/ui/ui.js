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
    text: "Network",
    fontSize: 25,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

/*
** Download Content
**
*/
win.addText({
    id: "download_Label",
    x: 15,
    y: 45,
    text: "Download:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})


win.addText({
    id: "download_Text",
    x: 195,
    y: 45,
    text: "-- KB/s",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

win.addBar({
    id: "download-Bar",
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
** Upload Content
**
*/

win.addText({
    id: "upload_Label",
    x: 15,
    y: 85,
    text: "Upload:",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})

win.addText({
    id: "upload_Text",
    x: 195,
    y: 85,
    text: "-- KB/s",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

win.addBar({
    id: "upload-Bar",
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

// IPC listener for network stats
ipc.on("networkStats", function(stats) {
    // Update download text with dynamic units
    win.setElementProperties("download_Text", { 
        "text": stats.downloadRate + " " + stats.downloadUnit 
    });
    
    // Update upload text with dynamic units
    win.setElementProperties("upload_Text", { 
        "text": stats.uploadRate + " " + stats.uploadUnit 
    });
    
    // Update progress bars based on rates (0.0 to 1.0 scale)
    // Use the same maxRate for normalization regardless of display units
    var maxRate = 1024; // 1 MB/s as max for bar scaling
    var downloadValue = Math.min(1.0, (parseFloat(stats.downloadRate) * (stats.downloadUnit === "MB/s" ? 1024 : 1)) / maxRate);
    var uploadValue = Math.min(1.0, (parseFloat(stats.uploadRate) * (stats.uploadUnit === "MB/s" ? 1024 : 1)) / maxRate);
    
    win.setElementProperties("download-Bar", { 
        "value": downloadValue 
    });
    
    win.setElementProperties("upload-Bar", { 
        "value": uploadValue 
    });
});