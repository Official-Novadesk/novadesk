var config_Path = path.join(app.getAppDataPath(), 'config.json');

// Default configuration values
var default_Config = {
    clock_Widget_Active: true,
    system_Widget_Active: true,
    network_Widget_Active: true,
    welcome_Widget_Active: true
};

// Try to read config, if failed use default
var config_Data = system.readJson(config_Path);

// If config is invalid (null, undefined, or not an object), use default
if (!config_Data || typeof config_Data !== 'object') {
    config_Data = default_Config;
    system.writeJson(config_Path, config_Data);
}


function pad2(n) {
    return (n < 10 ? "0" : "") + n;
}

function formatTime(dateObj, use12h) {
    var h = dateObj.getHours();
    var m = dateObj.getMinutes();
    var s = dateObj.getSeconds();

    if (use12h) {
        var h12 = h % 12;
        if (h12 === 0) h12 = 12;
        var ampm = h >= 12 ? "PM" : "AM";
        return pad2(h12) + ":" + pad2(m) + ":" + pad2(s) + " " + ampm;
    }

    return pad2(h) + ":" + pad2(m) + ":" + pad2(s);
}

function formatDay(dateObj) {
    var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    return days[dateObj.getDay()];
}

function formatDate(dateObj) {
    var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    var monthName = months[dateObj.getMonth()];
    var date = dateObj.getDate();
    var year = dateObj.getFullYear();
    return monthName + " " + date + ", " + year;
}

function getJsonValue(key) {
    // Return value if exists, otherwise return default value
    if (config_Data && typeof config_Data === 'object') {
        if (key in config_Data) {
            return config_Data[key];
        }
    }
    // Return default value if key doesn't exist
    if (default_Config && key in default_Config) {
        return default_Config[key];
    }
    return null;
}

function setJsonValue(key, value) {
    if (!config_Data || typeof config_Data !== 'object') {
        config_Data = {};
    }
    config_Data[key] = value;
    system.writeJson(config_Path, config_Data);
}

function kelvinToCelsius(kelvin) {
    return Math.round(kelvin - 273.15);
}

function celsiusToFahrenheit(celsius) {
    return Math.round((celsius * 9/5) + 32);
}

module.exports = {
    formatTime: formatTime,
    formatDay: formatDay,
    formatDate: formatDate,
    getJsonValue: getJsonValue,
    setJsonValue: setJsonValue,
    kelvinToCelsius: kelvinToCelsius,
    celsiusToFahrenheit: celsiusToFahrenheit,
    pad2: pad2
};