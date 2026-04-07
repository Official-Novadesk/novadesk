import * as utils from "../common/utils.js";
import { app, widgetWindow } from "novadesk";
import * as system from "system";

var clock_Widget = null;
var clock_Timer = null;
var use12h = false;

function loadClockWidget() {
    if (clock_Widget) {
        return; // Widget already registered
    }

    clock_Widget = new widgetWindow({
        id: 'clock_Window',
        script: './src/clockWidget/ui/widget.ui.js',
        width: 212,
        height: 72, 
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        var metrics = system.displayMetrics.get();
        clock_Widget.setProperties({
            x: ((metrics.primary.screenArea.width - 212) - 10),
            y: 10,
            show: true
        });
    }

    // Set up context menu
    clock_Widget.setContextMenu([
        {
            text: "Refresh",
            action: function () {
                clock_Widget.refresh();
            }
        },
        { type: "separator" },
        {
            text: "Close",
            action: function () {
                utils.setJsonValue('clock_Widget_Active', false);
                unloadClockWidget();
                if (typeof __refreshTrayMenu === "function") {
                    __refreshTrayMenu();
                }
            }
        }
    ]);

    registerIPC();
}

function registerIPC() {
    function publishTime() {
        var now = new Date();
        var timeStr = utils.formatTime(now, use12h);
        var dayStr = utils.formatDay(now);
        var dateStr = utils.formatDate(now);
        ipcMain.send("timeUpdate", timeStr);
        ipcMain.send("dayUpdate", dayStr);
        ipcMain.send("dateUpdate", dateStr);
    }

    publishTime();
    clock_Timer = setInterval(publishTime, 1000);
}

function toggleTimeFormat() {
    use12h = !use12h;
}

function unloadClockWidget() {
    if (clock_Timer) {
        clearInterval(clock_Timer);
        clock_Timer = null;
    }

    if (clock_Widget) {
        clock_Widget.close();
        clock_Widget = null;
    }
}

export { loadClockWidget, unloadClockWidget };
