import * as utils from "../common/utils.js";
import { app, widgetWindow } from "novadesk";
import * as system from "system";
var system_Widget = null;
var system_Timer = null;

function loadSystemWidget() {
    if (system_Widget) {
        return; // Widget already registered
    }

    system_Widget = new widgetWindow({
        id: 'system_Window',
        script: './src/systemWidget/ui/widget.ui.js',
        width: 212,
        height: 122,
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        var metrics = system.displayMetrics.get();
        system_Widget.setProperties({
            x: ((metrics.primary.screenArea.width - 212) - 10),
            y: 92,
            show: true
        });
    }
    
    // Set up context menu
    system_Widget.setContextMenu([
        {
            text: "Refresh",
            action: function() {
                system_Widget.refresh();
            }
        },
        { type: "separator" },
        {
            text: "Close",
            action: function() {
                utils.setJsonValue('system_Widget_Active', false);
                unloadSystemWidget();
                if (typeof __refreshTrayMenu === "function") {
                    __refreshTrayMenu();
                }
            }
        }
    ]);
    
    registerIPC();
}

function registerIPC() {
    function systemInfo() {
        var cpuUsage = Math.round(system.cpu.usage());
        var memoryUsage = system.memory.usagePercent();
        ipcMain.send('system-stats', {
            cpu: cpuUsage,
            memory: memoryUsage
        });
    }

    systemInfo();
    system_Timer = setInterval(systemInfo, 1000);
}

function unloadSystemWidget() {
    if (system_Timer) {
        clearInterval(system_Timer);
        system_Timer = null;
    }
    
    if (system_Widget) {
        system_Widget.close();
        system_Widget = null;
    }
}

export { loadSystemWidget, unloadSystemWidget };
