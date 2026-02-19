// // Brightness API Test
// // Tests system.getBrightness and system.setBrightness

// console.log("=== Brightness API Test Started ===");

// // Test 1: Read brightness info for primary display
// console.log("Test 1: Get brightness");
// var info = system.getBrightness({ display: 0 });
// console.log("Brightness info: " + JSON.stringify(info));

// if (info && typeof info === "object") {
//     console.log("OK: info object returned");
//     console.log("supported: " + info.supported);
//     console.log("current: " + info.current + ", min: " + info.min + ", max: " + info.max + ", percent: " + info.percent);
// } else {
//     console.log("FAIL: invalid info object");
// }

// // Test 2: Try setting brightness to current percent (no-op style check)
// if (info && typeof info.percent === "number") {
//     console.log("Test 2: Set brightness to current percent = " + info.percent);
//     var setResultSame = system.setBrightness({ percent: info.percent, display: 0 });
//     console.log("Set same level result: " + setResultSame);
// } else {
//     console.log("Test 2 skipped: no percent value");
// }

// // Test 3: Clamp behavior expectation at API layer (native may reject unsupported displays)
// console.log("Test 3: Set brightness out of bounds");
// var setLow = system.setBrightness({ percent: -10, display: 0 });
// var setHigh = system.setBrightness({ percent: 150, display: 0 });
// console.log("Set -10 result: " + setLow);
// console.log("Set 150 result: " + setHigh);

// // Test 4: Optional display index argument
// console.log("Test 4: Get brightness without params");
// var infoDefault = system.getBrightness();
// console.log("Default brightness info: " + JSON.stringify(infoDefault));

// console.log("=== Brightness API Test Completed ===");

var info = system.getBrightness();
console.log("Brightness info: " + JSON.stringify(info));

var setHigh = system.setBrightness({ percent: 100 });

console.log("Set 100 result: " + setHigh);