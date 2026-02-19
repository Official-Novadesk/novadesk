// File Icon Extraction API Test
// Tests system.extractFileIcon(filePath, outPath?, size?)

console.log("=== Icon Extract API Test Started ===");

// Test 1: Default output path (temp cache), default size
var icon1 = system.extractFileIcon("C:\\Windows\\System32\\notepad.exe");
console.log("Default extract => " + icon1);

// Test 2: Explicit output path and size 48
var icon2 = system.extractFileIcon(
    "C:\\Windows\\System32\\calc.exe",
    "calc_48.ico",
    48
);
console.log("Explicit extract (48) => " + icon2);

// Test 3: Invalid file path should return null
var icon4 = system.extractFileIcon("C:\\this\\path\\does-not-exist.exe");
console.log("Invalid extract => " + icon4);

console.log("=== Icon Extract API Test Completed ===");

