var utils = require('../common/utils');

var welcome_Widget = null;

function loadWelcomeWidget() {
    if (welcome_Widget) {
        return; // Widget already registered
    }

    welcome_Widget = new widgetWindow({
        id: 'welcome_Window',
        script: 'ui/ui.js',
        width: 300,
        height: 300,
        zPos: "ontop",
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        welcome_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 300) / 2),
            y: ((system.getDisplayMetrics().primary.screenArea.height - 300) / 2),
            show: true
        });
    }
}

// IPC listeners for button clicks
ipc.on("openWebsite", function() {
    system.execute("https://novadesk.pages.dev/");
});

ipc.on("openDocs", function() {
    system.execute("https://novadesk-docs.pages.dev/");
});

function unloadWelcomeWidget() {
    if (welcome_Widget) {
        welcome_Widget.close();
        welcome_Widget = null;
    }
}

module.exports = {
    loadWelcomeWidget,
    unloadWelcomeWidget
}