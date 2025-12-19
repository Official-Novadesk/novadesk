function createMainWindow() {
    mainWindow = new widgetWindow({
        width: 100,
        height: 100,
        backgroundColor: "rgb(10,10,10)",
        zPos: "ondesktop"
    });
    novadesk.log("widget Created", mainWindow);
    novadesk.error("Error here");
    novadesk.debug("debug here");
}

function createMainOneMoreWindow() {
    mainWindowMore = new widgetWindow({
        width: 100,
        height: 100,
        backgroundColor: "rgb(10,10,10)",
        zPos: "ondesktop"
    });
}

function novadeskAppReady(){
    createMainWindow();
    createMainOneMoreWindow();
}
