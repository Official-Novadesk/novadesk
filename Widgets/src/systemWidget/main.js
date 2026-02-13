var system_Widget = null;
var cpuMonitor = null;
var memoryMonitor = null;



function loadSystemWidget() {

    if (system_Widget) {
        return; // Widget already registered
    }

    system_Widget = new widgetWindow({
        id: 'system_Window',
        script: path.join(__dirname, 'ui/ui.js'),
        width: 212,
        height: 122,
        zPos: "ontop",
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {

        system_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 212) - 10),
            y: 10,
            show: true
        });
    }
    registerIPC()
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
        ipc.send('cpu-usage', cpuUsage);
        ipc.send('memory-usage', memoryUsage);

    }

    systemInfo()
    setInterval(systemInfo, 1000);
}
module.exports = {
    loadSystemWidget
}
