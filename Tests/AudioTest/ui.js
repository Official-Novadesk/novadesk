
var bandCount = 20;
var barWidth = 10;
var barGap = 2;
var totalBarWidth = barWidth + barGap;
var centerX = 300;
var centerY = 200;
var maxBarHeight = 150;

// We need 2 sets of bars for X symmetry (High <- Low -> High)
var barsLU = []; // Left Up
var barsRU = []; // Right Up

// Start from Center (Low Freq) and go Outwards (High Freq)
for (var i = 0; i < bandCount; i++) {
    // Current band corresponds to i (0=Low, 19=High)
    // Distance from center
    var xOffset = barGap + i * totalBarWidth; 
    
    // Right Position: Center + offset
    var xRight = centerX + xOffset;
    
    // Left Position: Center - offset - width
    var xLeft = centerX - xOffset - barWidth;

    // --- Right Side (Low -> High) ---
    // Right Up (Grows Upwards from Center)
    win.addBar({
        id: "ru_" + i,
        x: xRight,
        y: centerY, // Starts at center
        width: barWidth,
        height: 1,  // Starts small
        barColor: "rgba(0, 255, 255, 0.8)",
        backgroundColor: "transparent",
        value: 1 // Always full, we change size
    });
    barsRU.push("ru_" + i);

    // --- Left Side (High <- Low) ---
    // Left Up (Grows Upwards)
    win.addBar({
        id: "lu_" + i,
        x: xLeft,
        y: centerY,
        width: barWidth,
        height: 1,
        barColor: "rgba(0, 255, 255, 0.8)",
        backgroundColor: "transparent",
        value: 1
    });
    barsLU.push("lu_" + i);
}

win.addText({
    id: "rms_l",
    x: 20, y: 20,
    text: "L: 0.00",
    fontColor: "rgba(0, 255, 255, 0.8)",
    fontSize: 14
});

win.addText({
    id: "rms_r",
    x: 520, y: 20,
    text: "R: 0.00",
    fontColor: "rgba(0, 255, 255, 0.8)",
    fontSize: 14
});

ipc.on("audio-data", function(data) {
    if (data.rms) {
        win.setElementProperties("rms_l", { text: "L: " + data.rms[0].toFixed(2) });
        win.setElementProperties("rms_r", { text: "R: " + data.rms[1].toFixed(2) });
    }

    if (data.bands) {
        if (win.beginUpdate) win.beginUpdate();
        
        for (var i = 0; i < bandCount; i++) {
            if (i < data.bands.length) {
                var val = data.bands[i];
                var h = Math.max(1, val * maxBarHeight);

                // Right Up: Grows UP. y decreases, height increases.
                win.setElementProperties(barsRU[i], { height: h, y: centerY - h });
                
                // Left Up: Grows UP.
                win.setElementProperties(barsLU[i], { height: h, y: centerY - h });
            }
        }
        
        if (win.endUpdate) win.endUpdate();
    }
});
