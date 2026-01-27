app.enableDebugging(true);

console.log("Testing console.log");
console.warn("Testing console.warn");
console.error("Testing console.error");
console.debug("Testing console.debug");

// Keep it open for a bit to see the output if needed, or just exit
setTimeout(function() {
    console.log("Exiting test script");
    app.exit();
}, 2000);
