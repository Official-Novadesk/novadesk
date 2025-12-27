!logToFile;

novadesk.log("--- Starting Environment Variable Fix Verification ---");

// Get all environment variables as an object
var allEnv = system.getEnv();
if (allEnv) {
    novadesk.log("Total environment variables: " + Object.keys(allEnv).length);

    // Iterate through all environment variables
    var count = 0;
    for (var key in allEnv) {
        if (allEnv.hasOwnProperty(key)) {
            novadesk.log(key + " = " + allEnv[key]);
            count++;
            if (count > 20) {
                novadesk.log("... and more (truncated for test)");
                break;
            }
        }
    }
} else {
    novadesk.log("Error: system.getEnv() returned nothing!");
}

novadesk.log("--- Verification Finished ---");
