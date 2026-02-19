// JSON API Test
// Tests system.readJson and system.writeJson

console.log("=== JSON API Test Started ===");

// Test 1: Read existing JSON file
console.log("Test 1: Read existing JSON file");
var jsonData = system.readJson("test_config.json");
console.log("Read result: " + (jsonData ? "Success" : "Failed"));

if (jsonData) {
    console.log("Data: " + JSON.stringify(jsonData));
}

// Test 2: Read non-existent JSON file
console.log("Test 2: Read non-existent JSON file");
var jsonData2 = system.readJson("nonexistent.json");
console.log("Non-existent read result: " + jsonData2);

// Test 3: Write JSON data
console.log("Test 3: Write JSON data");
var testData = {
    name: "Test Config",
    version: 1.0,
    enabled: true,
    settings: {
        debug: false,
        timeout: 5000
    }
};

var writeResult = system.writeJson("output_test.json", testData);
console.log("Write result: " + writeResult);

// Test 4: Write and read back
console.log("Test 4: Write and read verification");
var verifyData = {test: "verification", number: 42};
system.writeJson("verify.json", verifyData);
var readBack = system.readJson("verify.json");
console.log("Read back result: " + (readBack ? "Success" : "Failed"));

if (readBack) {
    console.log("Verification data matches: " + JSON.stringify(readBack));
}

// Test 5: Deep merge with comment preservation
console.log("Test 5: Deep merge with comment preservation");

var patchData = {
    obj: { b: 20, d: 4 },
    arr: [9, 8],
    newKey: true
};

var mergeResult = system.writeJson("merge_base.json", patchData);
console.log("Merge write result: " + mergeResult);

system.fetch("merge_base.json", function(data) {
    console.log("Merged text:\n" + data);
    console.log("Preserved top comment: " + (data.indexOf("// top comment") !== -1));
    console.log("Preserved inner comment: " + (data.indexOf("// inner comment") !== -1));
});

var mergedJson = system.readJson("merge_base.json");
if (mergedJson) {
    console.log("Merged obj.b == 20: " + (mergedJson.obj && mergedJson.obj.b === 20));
    console.log("Merged obj.c == 3: " + (mergedJson.obj && mergedJson.obj.c === 3));
    console.log("Merged obj.d == 4: " + (mergedJson.obj && mergedJson.obj.d === 4));
    console.log("Merged arr length == 2: " + (mergedJson.arr && mergedJson.arr.length === 2));
    console.log("Merged newKey == true: " + (mergedJson.newKey === true));
}

console.log("=== JSON API Test Completed ===");
