const clock_Widget = require('./src/clockWidget/main');
const utils = require('./src/common/utils');
const system_Widget = require('./src/systemWidget/main');
const network_Widget = require('./src/networkWidget/main');
const welcome_Widget = require('./src/welcomeWidget/main');

var isFirstRun = !!app.isFirstRun();

// First run: force-show all default widgets and persist enabled state.
if (isFirstRun) {
    clock_Widget.loadClockWidget();
    system_Widget.loadSystemWidget();
    network_Widget.loadNetworkWidget();
    welcome_Widget.loadWelcomeWidget();

    utils.setJsonValue('clock_Widget_Active', true);
    utils.setJsonValue('system_Widget_Active', true);
    utils.setJsonValue('network_Widget_Active', true);
    utils.setJsonValue('welcome_Widget_Active', true);
} else {
    // Load widgets based on saved state
    if (utils.getJsonValue('clock_Widget_Active')) {
        clock_Widget.loadClockWidget();
        utils.setJsonValue('clock_Widget_Active', true);
    }

    if (utils.getJsonValue('system_Widget_Active') !== false) {
        system_Widget.loadSystemWidget();
        utils.setJsonValue('system_Widget_Active', true);
    }

    if (utils.getJsonValue('network_Widget_Active') !== false) {
        network_Widget.loadNetworkWidget();
        utils.setJsonValue('network_Widget_Active', true);
    }

    if (utils.getJsonValue('welcome_Widget_Active') !== false) {
        welcome_Widget.loadWelcomeWidget();
        utils.setJsonValue('welcome_Widget_Active', true);
    }
}

// Set up tray menu
function setupTrayMenu() {
    app.setTrayMenu([
        {
            text: "Website",
            action: function() {
                system.execute("https://novadesk.pages.dev/");
            }
        },
        {
            text: "Docs",
            action: function() {
                system.execute("https://novadesk-docs.pages.dev/");
            }
        },
        { type: "separator" },
        {
            text: "Widgets",
            items: [
                {
                    text: "Clock Widget",
                    checked: !!utils.getJsonValue('clock_Widget_Active'),
                    action: function() {
                        var isActive = !!utils.getJsonValue('clock_Widget_Active');
                        if (isActive) {
                            // Unload widget
                            if (typeof clock_Widget.unloadClockWidget === 'function') {
                                clock_Widget.unloadClockWidget();
                            }
                            utils.setJsonValue('clock_Widget_Active', false);
                        } else {
                            // Load widget
                            clock_Widget.loadClockWidget();
                            utils.setJsonValue('clock_Widget_Active', true);
                        }
                        // Refresh tray menu to update checkmark
                        setupTrayMenu();
                    }
                },
                {
                    text: "System Widget",
                    checked: !!utils.getJsonValue('system_Widget_Active'),
                    action: function() {
                        var isActive = !!utils.getJsonValue('system_Widget_Active');
                        if (isActive) {
                            // Unload widget
                            if (typeof system_Widget.unloadSystemWidget === 'function') {
                                system_Widget.unloadSystemWidget();
                            }
                            utils.setJsonValue('system_Widget_Active', false);
                        } else {
                            // Load widget
                            system_Widget.loadSystemWidget();
                            utils.setJsonValue('system_Widget_Active', true);
                        }
                        setupTrayMenu();
                    }
                },
                {
                    text: "Network Widget",
                    checked: !!utils.getJsonValue('network_Widget_Active'),
                    action: function() {
                        var isActive = !!utils.getJsonValue('network_Widget_Active');
                        if (isActive) {
                            // Unload widget
                            if (typeof network_Widget.unloadNetworkWidget === 'function') {
                                network_Widget.unloadNetworkWidget();
                            }
                            utils.setJsonValue('network_Widget_Active', false);
                        } else {
                            // Load widget
                            network_Widget.loadNetworkWidget();
                            utils.setJsonValue('network_Widget_Active', true);
                        }
                        setupTrayMenu();
                    }
                },
                {
                    text: "Welcome Widget",
                    checked: !!utils.getJsonValue('welcome_Widget_Active'),
                    action: function() {
                        var isActive = !!utils.getJsonValue('welcome_Widget_Active');
                        if (isActive) {
                            // Unload widget
                            if (typeof welcome_Widget.unloadWelcomeWidget === 'function') {
                                welcome_Widget.unloadWelcomeWidget();
                            }
                            utils.setJsonValue('welcome_Widget_Active', false);
                        } else {
                            // Load widget
                            welcome_Widget.loadWelcomeWidget();
                            utils.setJsonValue('welcome_Widget_Active', true);
                        }
                        setupTrayMenu();
                    }
                }
            ]
        },
        { type: "separator" },
        {
            text: "Exit",
            action: function() {
                app.exit();
            }
        }
    ]);
}

globalThis.__refreshTrayMenu = setupTrayMenu;

// Initialize tray menu
setupTrayMenu();
