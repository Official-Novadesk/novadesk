import * as clock_Widget from "./src/clockWidget/main.js";
import * as utils from "./src/common/utils.js";
import * as system_Widget from "./src/systemWidget/main.js";
import * as network_Widget from "./src/networkWidget/main.js";
import * as welcome_Widget from "./src/welcomeWidget/main.js";
import { app, tray as Tray } from "novadesk";
import * as system from "system";

var isFirstRun = !!app.isFirstRun();
const trayIcon = new Tray("src/assets/tray.ico");

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
    trayIcon.setContextMenu([
        {
            text: "Clock Widget",
            checked: !!utils.getJsonValue('clock_Widget_Active'),
            action: function () {
                var isActive = !!utils.getJsonValue('clock_Widget_Active');
                if (isActive) {
                    if (typeof clock_Widget.unloadClockWidget === 'function') {
                        clock_Widget.unloadClockWidget();
                    }
                    utils.setJsonValue('clock_Widget_Active', false);
                } else {
                    clock_Widget.loadClockWidget();
                    utils.setJsonValue('clock_Widget_Active', true);
                }
                setupTrayMenu();
            }
        },
        {
            text: "System Widget",
            checked: !!utils.getJsonValue('system_Widget_Active'),
            action: function () {
                var isActive = !!utils.getJsonValue('system_Widget_Active');
                if (isActive) {
                    if (typeof system_Widget.unloadSystemWidget === 'function') {
                        system_Widget.unloadSystemWidget();
                    }
                    utils.setJsonValue('system_Widget_Active', false);
                } else {
                    system_Widget.loadSystemWidget();
                    utils.setJsonValue('system_Widget_Active', true);
                }
                setupTrayMenu();
            }
        },
        {
            text: "Network Widget",
            checked: !!utils.getJsonValue('network_Widget_Active'),
            action: function () {
                var isActive = !!utils.getJsonValue('network_Widget_Active');
                if (isActive) {
                    if (typeof network_Widget.unloadNetworkWidget === 'function') {
                        network_Widget.unloadNetworkWidget();
                    }
                    utils.setJsonValue('network_Widget_Active', false);
                } else {
                    network_Widget.loadNetworkWidget();
                    utils.setJsonValue('network_Widget_Active', true);
                }
                setupTrayMenu();
            }
        },
        {
            text: "Welcome Widget",
            checked: !!utils.getJsonValue('welcome_Widget_Active'),
            action: function () {
                var isActive = !!utils.getJsonValue('welcome_Widget_Active');
                if (isActive) {
                    if (typeof welcome_Widget.unloadWelcomeWidget === 'function') {
                        welcome_Widget.unloadWelcomeWidget();
                    }
                    utils.setJsonValue('welcome_Widget_Active', false);
                } else {
                    welcome_Widget.loadWelcomeWidget();
                    utils.setJsonValue('welcome_Widget_Active', true);
                }
                setupTrayMenu();
            }
        }
    ]);
}

globalThis.__refreshTrayMenu = setupTrayMenu;

// Initialize tray menu
setupTrayMenu();
