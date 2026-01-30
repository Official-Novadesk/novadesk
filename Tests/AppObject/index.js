// Get the original Novadesk version
const novadeskVersion = app.getNovadeskVersion();
console.log("Original Novadesk Version: " + novadeskVersion);

// Get the current file version
const fileVersion = app.getFileVersion();
console.log("File Version: " + fileVersion);

// Get the current product version
const version = app.getProductVersion();
console.log("Product Version: " + version);

const appData = app.getAppDataPath();
console.log("AppData Path: " + appData);

const settingsPath = app.getSettingsFilePath();
console.log("Settings Path: " + settingsPath);

const logPath = app.getLogPath();
console.log("Log Path: " + logPath);