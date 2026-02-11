// Require Test (index.js only)

const a = require("./moduleA");
console.log("RequireTest: a.name =", a.name);
console.log("RequireTest: a.bName =", a.bName);

const sum = require("./sum");
console.log("RequireTest: sum(2, 3) =", sum(2, 3));
