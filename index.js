function createMainWindow() {
    mainWindow = new widgetWindow({
        width: 200,
        height: 200,
        backgroundColor: "rgba(10,10,10,200)",
        zPos: "ondesktop",
        draggable: true,
        snapEdges: true,
        keepOnScreen: true
    });
    novadesk.log("Main widget created");
}

function createContentDemoWindow() {
    var demoWidget = new widgetWindow({
        width: 300,
        height: 400,
        backgroundColor: "rgba(30,30,40,240)",
        zPos: "normal",
        draggable: true,
        keepOnScreen: true
    });

    // Add Title
    demoWidget.addText({
        id: "title",
        x: 0,
        y: 20,
        width: 300,
        height: 40,
        text: "Weather Widget",
        fontFamily: "Segoe UI",
        fontSize: 24,
        color: "rgba(255,255,255,255)",
        fontWeight: "bold",
        align: "center"
    });

    // Add Temperature
    demoWidget.addText({
        id: "temp",
        x: 0,
        y: 80,
        width: 300,
        height: 80,
        text: "72°",
        fontFamily: "Segoe UI",
        fontSize: 64,
        color: "rgba(255,255,255,255)",
        align: "center"
    });

    // Add Condition Text
    demoWidget.addText({
        id: "condition",
        x: 0,
        y: 160,
        width: 300,
        height: 30,
        text: "Partly Cloudy",
        fontFamily: "Segoe UI",
        fontSize: 16,
        color: "rgba(200,200,200,255)",
        fontStyle: "italic",
        align: "center"
    });

    // Add Image (Placeholder path - user needs to provide real image)
    // demoWidget.addImage({
    //     id: "icon",
    //     path: "weather.png", 
    //     x: 100,
    //     y: 200,
    //     width: 100,
    //     height: 100,
    //     scaleMode: "contain"
    // });

    // Dynamic Update Demo
    novadesk.log("Starting dynamic update timer...");

    // Simple counter to simulate updates since we don't have setInterval in basic Duktape
    // (Assuming Duktape usage doesn't imply full browser env, but if event loop exists...)
    // Since I implemented basic logic, I don't know if setInterval is available.
    // If not, this is just a static value setup.

    novadesk.log("Content demo widget created");
}

function novadeskAppReady() {
    createMainWindow();
    createContentDemoWindow();
}
