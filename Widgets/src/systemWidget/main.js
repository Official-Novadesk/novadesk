var utils = require('../common/utils');
var system_Widget = null;
var cpuMonitor = null;
var memoryMonitor = null;
var system_Timer = null;

function loadSystemWidget() {
    if (system_Widget) {
        return; // Widget already registered
    }

    system_Widget = new widgetWindow({
        id: 'system_Window',
        script: path.join(__dirname, 'ui/ui.js'),
        width: 212,
        height: 122,
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        system_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 212) - 10),
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
    if (!cpuMonitor) {
        cpuMonitor = new system.cpu();
    }

    if (!memoryMonitor) {
        memoryMonitor = new system.memory();
    }

    function systemInfo() {
        var cpuUsage = cpuMonitor.usage();
        var memoryUsage = memoryMonitor.stats().percent;
        ipc.send('system-stats', {
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
    
    if (cpuMonitor) {
        // cpuMonitor.destroy(); // Check if destroy method exists
        cpuMonitor = null;
    }
    
    if (memoryMonitor) {
        // memoryMonitor.destroy(); // Check if destroy method exists
        memoryMonitor = null;
    }
    
    if (system_Widget) {
        system_Widget.close();
        system_Widget = null;
    }
}

module.exports = {
    loadSystemWidget,
    unloadSystemWidget
}
