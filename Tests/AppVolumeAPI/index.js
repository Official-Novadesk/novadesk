// App Volume API Test
// Tests listAppVolumes/getAppVolume/setAppVolume/getAppMute/setAppMute

console.log("=== App Volume API Test Started ===");

var sessions = system.listAppVolumes();
console.log("Sessions count: " + (sessions ? sessions.length : 0));

if (!sessions || !sessions.length) {
    console.log("No active sessions found, skipping mutation tests.");
    console.log("=== App Volume API Test Completed ===");
} else {
    var first = sessions[0];
    console.log("Using first session => pid: " + first.pid + ", process: " + first.process + ", fileName: " + first.fileName + ", filePath: " + first.filePath + ", volume: " + first.volume + ", peak: " + first.peak + ", muted: " + first.muted);

    var selector = { pid: first.pid };

    var volBefore = system.getAppVolume(selector);
    var peakNow = system.getAppPeak(selector);
    var iconPath = first.iconPath;
    var muteBefore = system.getAppMute(selector);
    console.log("Before => volume: " + volBefore + ", peak: " + peakNow + ", icon: " + iconPath + ", mute: " + muteBefore);

    var targetVol = (volBefore >= 90) ? 70 : 90;
    var setVolOk = system.setAppVolume(selector, targetVol);
    var volAfterSet = system.getAppVolume(selector);
    console.log("Set volume to " + targetVol + " => " + setVolOk + ", now: " + volAfterSet);

    // Keep mute unchanged during volume testing.
    // var setMuteOk = system.setAppMute(selector, !muteBefore);
    // var muteAfterSet = system.getAppMute(selector);
    // console.log("Toggle mute => " + setMuteOk + ", now: " + muteAfterSet);

    // Keep changed state so result is visible after script exits.
    // Uncomment to restore automatically:
    // system.setAppVolume(selector, volBefore);
    // system.setAppMute(selector, muteBefore);
    // console.log("State restored.");

    console.log("=== App Volume API Test Completed ===");
}
