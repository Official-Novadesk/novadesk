import { tray, app } from "novadesk";

// Create primary tray icon
const mainTray = new tray("../assets/icon.ico");
mainTray.setToolTip("Novadesk Tray Test");

// Create secondary tray icon
const secondaryTray = new tray("../assets/icon.ico");
secondaryTray.setToolTip("Novadesk Secondary Icon");

function logEvent(name, event) {
    if (event) {
        console.log("[TrayTest] " + name + " " + JSON.stringify(event));
        return;
    }
    console.log("[TrayTest] " + name);
}

// Primary tray event handlers
// mainTray.on("click", (event) => logEvent("Main Click", event));
mainTray.on("right-click", (event) => logEvent("Main Right-Click", event));
mainTray.on("scroll-up", (event) => {
    console.log("Tray Scrolled UP!");
});

mainTray.on("scroll-down", (event) => {
    console.log("Tray Scrolled DOWN!");
});

mainTray.on("double-click", () => {
    console.log("Main Tray double-clicked!");
});


mainTray.setContextMenu([
    {
        text: "Exit Main",
        action: function () {
            mainTray.destroy();
            secondaryTray.destroy();
            app.exit();
        }
    }
]);

// Secondary tray event handlers
secondaryTray.on("click", (event) => logEvent("Secondary Click", event));
secondaryTray.on("right-click", (event) => logEvent("Secondary Right-Click", event));
secondaryTray.setContextMenu([
    {
        text: "Close Secondary",
        action: function () {
            secondaryTray.destroy();
        }
    },
    {
        text: "Exit App",
        action: function () {
            mainTray.destroy();
            secondaryTray.destroy();
            app.exit();
        }
    }
]);
