var cDrive = new system.disk({ drive: "C:" });
var allDisks = new system.disk();  // Monitor all drives

// Update every 1 second (disk info doesn't change as frequently)
var intervalId = setInterval(function () {
    var cStats = cDrive.stats();

    // Convert bytes to GB for display
    var cTotalGB = (cStats.total / (1024 * 1024 * 1024)).toFixed(2);
    var cUsedGB = (cStats.used / (1024 * 1024 * 1024)).toFixed(2);
    var cFreeGB = (cStats.free / (1024 * 1024 * 1024)).toFixed(2);

    console.log("C: Drive - Total: " + cTotalGB + "GB, Used: " + cUsedGB + "GB, Free: " + cFreeGB + "GB (" + cStats.percent + "%)");

    var allStats = allDisks.stats();
    for (var i = 0; i < allStats.length; i++) {
        var drive = allStats[i];
        var totalGB = (drive.total / (1024 * 1024 * 1024)).toFixed(2);
        var usedGB = (drive.used / (1024 * 1024 * 1024)).toFixed(2);
        var freeGB = (drive.free / (1024 * 1024 * 1024)).toFixed(2);

        console.log(drive.drive + " Drive - Total: " + totalGB + "GB, Used: " + usedGB + "GB, Free: " + freeGB + "GB (" + drive.percent + "%)");
    }
}, 1000);

setTimeout(function () {
    clearInterval(intervalId);
    cDrive.destroy();
    allDisks.destroy();
    console.log("Disk Monitor Destroyed");
}, 30000);