console.log("=== SetWallpaper API Test Started ===");

// Use an existing local image path for your machine.
// Example below points to current test asset.
var ok = system.setWallpaper("C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\pics\\2.png");
console.log("setWallpaper result:", ok);
var okStretch = system.setWallpaper("C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\pics\\2.png", "stretch");
console.log("setWallpaper stretch result:", okStretch);

console.log("=== SetWallpaper API Test Completed ===");
