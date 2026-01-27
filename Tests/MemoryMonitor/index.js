// Create memory monitor
var memory = new system.memory();

// Update every 2 seconds
var intervalId = setInterval(function () {
    var stats = memory.stats();

    // Convert bytes to GB for display
    var totalGB = (stats.total / (1024 * 1024 * 1024)).toFixed(2);
    var usedGB = (stats.used / (1024 * 1024 * 1024)).toFixed(2);
    var availableGB = (stats.available / (1024 * 1024 * 1024)).toFixed(2);

    console.log("Memory - Total: " + totalGB + "GB, Used: " + usedGB + "GB, Available: " + availableGB + "GB");
    console.log("Memory Load: " + stats.percent + "%");
}, 1000);

setTimeout(function () {
    clearInterval(intervalId);
    memory.destroy();
    console.log("Memory Monitor Destroyed");
}, 5000)