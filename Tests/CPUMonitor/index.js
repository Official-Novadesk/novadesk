// Create CPU monitors
var overallCPU = new system.cpu();
var core1CPU = new system.cpu({ processor: 1 });

// Store the interval ID
var intervalId = setInterval(function () {
    var overallUsage = overallCPU.usage();
    var core1Usage = core1CPU.usage();
    console.log("Overall CPU: " + overallUsage + "%");
    console.log("Core 1 CPU: " + core1Usage + "%");
}, 1000);

// Destroy monitors and clean up the interval when done
setTimeout(function () {
    clearInterval(intervalId);
    overallCPU.destroy();
    core1CPU.destroy();
    console.log("CPU Monitors Destroyed");
}, 5000);