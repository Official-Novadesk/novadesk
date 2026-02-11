const a = require("./moduleA");

module.exports = {
    name: "moduleB",
    aName: a.name || "partial"
};
