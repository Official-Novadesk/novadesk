// Get all environment variables as an object
var allEnv = system.getEnv();
console.log("Total environment variables: " + Object.keys(allEnv).length);

// Iterate through all environment variables
for (var key in allEnv) {
    if (allEnv.hasOwnProperty(key)) {
        console.log(key + " = " + allEnv[key]);
    }
}