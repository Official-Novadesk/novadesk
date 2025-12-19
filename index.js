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
    novadesk.log("Main widget created with snapping and keepOnScreen");
}

function createClickThroughWindow() {
    clickThroughWindow = new widgetWindow({
        width: 150,
        height: 150,
        backgroundColor: "rgba(255,100,100,150)",
        zPos: "ontop",
        clickThrough: true,
        draggable: false
    });
    novadesk.log("Click-through widget created");
}

function novadeskAppReady(){
    createMainWindow();
    createClickThroughWindow();
}
