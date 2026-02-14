// Background Shape
win.addShape({
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
win.addImage({
    id: "logo_Image",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2) - 50,
    y: 40,
    width: 100,
    height: 100,
    path: "../../assets/Novadesk.png",
    opacity: 1.0
})

// Welcome Title
win.addText({
    id: "title_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
    y: 160,
    text: "Welcome to Novadesk!",
    fontSize: 18,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

// Description Text
win.addText({
    id: "desc_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
    y: 190,
    text: "A desktop customization platform",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "rgb(200,200,200)",
})

// Website Button
win.addShape({
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
        win.setElementProperties("website_Button", {"fillColor": "rgba(81, 188, 254, 0.5)"});
    },
    onMouseLeave: function() {
        win.setElementProperties("website_Button", {"fillColor": "rgba(81, 188, 254, 0.3)"});
    },
    onLeftMouseUp: function() {
        ipc.send("openWebsite");
    }
})

// Website Button Text
win.addText({
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
win.addShape({
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
        win.setElementProperties("docs_Button", {"fillColor": "rgba(189, 52, 254, 0.5)"});
    },
    onMouseLeave: function() {
        win.setElementProperties("docs_Button", {"fillColor": "rgba(189, 52, 254, 0.3)"});
    },
    onLeftMouseUp: function() {
        ipc.send("openDocs");
    }
})

// Docs Button Text
win.addText({
    id: "docs_Text",
    x: 200,
    y: 245,
    text: "Docs",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "centercenter",
    fontColor: "rgb(255,255,255)",
})
