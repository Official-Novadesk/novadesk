var utils = require('../common/utils');

var network_Widget = null;
var network_Monitor = null;
var network_Timer = null;

function loadNetworkWidget() {
    if (network_Widget) {
        return; // Widget already registered
    }

    // Create network monitor
    network_Monitor = new system.network();

    network_Widget = new widgetWindow({
        id: 'network_Window',
        script: 'ui/ui.js',
        width: 212,
        height: 122,
        zPos: "ontop",
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        network_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 212) - 10),
            y: 10,
            show: true
        });
    }

    registerIPC();
}

function registerIPC() {
    function publishNetworkStats() {
        if (!network_Monitor) return;
        
        var stats = network_Monitor.stats();
        
        // Convert bytes to appropriate units
        var netInKB = stats.netIn / 1024;
        var netOutKB = stats.netOut / 1024;
        
        // Format download rate with appropriate units
        var downloadRate, downloadUnit;
        if (netInKB >= 1024) {
            downloadRate = (netInKB / 1024).toFixed(2);
            downloadUnit = "MB/s";
        } else {
            downloadRate = netInKB.toFixed(2);
            downloadUnit = "KB/s";
        }
        
        // Format upload rate with appropriate units
        var uploadRate, uploadUnit;
        if (netOutKB >= 1024) {
            uploadRate = (netOutKB / 1024).toFixed(2);
            uploadUnit = "MB/s";
        } else {
            uploadRate = netOutKB.toFixed(2);
            uploadUnit = "KB/s";
        }
        
        // Convert total bytes to MB for display
        var totalInMB = (stats.totalIn / (1024 * 1024)).toFixed(2);
        var totalOutMB = (stats.totalOut / (1024 * 1024)).toFixed(2);
        
        // Send to UI
        ipc.send("networkStats", {
            downloadRate: downloadRate,
            downloadUnit: downloadUnit,
            uploadRate: uploadRate,
            uploadUnit: uploadUnit,
            totalDownload: totalInMB,
            totalUpload: totalOutMB
        });
    }

    publishNetworkStats();
    network_Timer = setInterval(publishNetworkStats, 1000);
}

function unloadNetworkWidget() {
    if (network_Timer) {
        clearInterval(network_Timer);
        network_Timer = null;
    }
    
    if (network_Monitor) {
        network_Monitor.destroy();
        network_Monitor = null;
    }
    
    if (network_Widget) {
        network_Widget.close();
        network_Widget = null;
    }
}

module.exports = {
    loadNetworkWidget,
    unloadNetworkWidget
}