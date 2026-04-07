import * as utils from "../common/utils.js";
import { app, widgetWindow } from "novadesk";
import * as system from "system";

var network_Widget = null;
var network_Timer = null;

function loadNetworkWidget() {
    if (network_Widget) {
        return; // Widget already registered
    }

    network_Widget = new widgetWindow({
        id: 'network_Window',
        script: './src/networkWidget/ui/widget.ui.js',
        width: 212,
        height: 122, 
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        var metrics = system.displayMetrics.get();
        network_Widget.setProperties({
            x: ((metrics.primary.screenArea.width - 212) - 10),
            y: 224,
            show: true
        });
    }

    // Set up context menu
    network_Widget.setContextMenu([
        {
            text: "Refresh",
            action: function () {
                network_Widget.refresh();
            }
        },
        { type: "separator" },
        {
            text: "Close",
            action: function () {
                utils.setJsonValue('network_Widget_Active', false);
                unloadNetworkWidget();
                if (typeof __refreshTrayMenu === "function") {
                    __refreshTrayMenu();
                }
            }
        }
    ]);

    registerIPC();
}

function registerIPC() {
    function publishNetworkStats() {
        var stats = {
            netIn: system.network.rxSpeed(),
            netOut: system.network.txSpeed(),
            totalIn: system.network.bytesReceived(),
            totalOut: system.network.bytesSent()
        };

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
        ipcMain.send("networkStats", {
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

    if (network_Widget) {
        network_Widget.close();
        network_Widget = null;
    }
}

export { loadNetworkWidget, unloadNetworkWidget };
