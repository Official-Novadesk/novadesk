const b = require("./moduleB");

module.exports = {
    name: "moduleA",
    bName: b.name
};
