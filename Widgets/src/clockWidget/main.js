var utils = require('../common/utils');

var clock_Widget = null;
var clock_Timer = null;
var use12h = false;

function loadClockWidget() {
    if (clock_Widget) {
        return; // Widget already registered
    }

    clock_Widget = new widgetWindow({
        id: 'clock_Window',
        script: 'ui/ui.js',
        width: 212,
        height: 72,
        zPos: "ontop",
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {

        clock_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 212) - 10),
            y: 10,
            show: true
        });
    }

    registerIPC();
}

function registerIPC() {
    function publishTime() {
        var now = new Date();
        var timeStr = utils.formatTime(now, use12h);
        var dayStr = utils.formatDay(now);
        var dateStr = utils.formatDate(now);
        ipc.send("timeUpdate", timeStr);
        ipc.send("dayUpdate", dayStr);
        ipc.send("dateUpdate", dateStr);
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

module.exports = {
    loadClockWidget,
    unloadClockWidget
}
