ui.beginUpdate();

// Background Shape
ui.addShape({
    id: "backgroundShape",
    type: "rectangle",
    x: 1,
    y: 1,
    width: 298,
    height: 298,
    fillColor: "rgba(27 ,27 ,31,0.9)",
    strokeColor: "rgba(200,200,200,0.5)",
    radius: 15,
    strokeWidth: 2
})

// Novadesk Logo
ui.addImage({
    id: "logo_Image",
    x: 100,
    y: 40,
    width: 100,
    height: 100,
    path: "../../assets/Novadesk.png",
    opacity: 1.0
})

// Welcome Title
ui.addText({
    id: "title_Text",
    x: 150,
    y: 160,
    text: "Welcome to Novadesk!",
    fontSize: 18,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

// Description Text
ui.addText({
    id: "desc_Text",
    x: 150,
    y: 190,
    text: "A desktop customization platform",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "rgb(200,200,200)",
})

// Website Button
ui.addShape({
    id: "website_Button",
    type: "rectangle",
    x: 60,
    y: 230,
    width: 80,
    height: 30,
    fillColor: "rgba(81, 188, 254, 0.3)",
    strokeColor: "rgba(81, 188, 254, 0.7)",
    radius: 8,
    strokeWidth: 1,
    onMouseOver: function() {
        ui.setElementProperties("website_Button", {"fillColor": "rgba(81, 188, 254, 0.5)"});
    },
    onMouseLeave: function() {
        ui.setElementProperties("website_Button", {"fillColor": "rgba(81, 188, 254, 0.3)"});
    },
    onLeftMouseUp: function() {
        ipcRenderer.send("openWebsite");
    }
})

// Website Button Text
ui.addText({
    id: "website_Text",
    x: 100,
    y: 245,
    text: "Website",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "centercenter",
    fontColor: "rgb(255,255,255)",
})

// Docs Button
ui.addShape({
    id: "docs_Button",
    type: "rectangle",
    x: 160,
    y: 230,
    width: 80,
    height: 30,
    fillColor: "rgba(189, 52, 254, 0.3)",
    strokeColor: "rgba(189, 52, 254, 0.7)",
    radius: 8,
    strokeWidth: 1,
    onMouseOver: function() {
        ui.setElementProperties("docs_Button", {"fillColor": "rgba(189, 52, 254, 0.5)"});
    },
    onMouseLeave: function() {
        ui.setElementProperties("docs_Button", {"fillColor": "rgba(189, 52, 254, 0.3)"});
    },
    onLeftMouseUp: function() {
        ipcRenderer.send("openDocs");
    }
})

// Docs Button Text
ui.addText({
    id: "docs_Text",
    x: 200,
    y: 245,
    text: "Docs",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "centercenter",
    fontColor: "rgb(255,255,255)",
})

ui.endUpdate();
