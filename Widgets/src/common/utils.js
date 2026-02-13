var config_Path = path.join(app.getAppDataPath(), 'config.json');

var config_Data = system.readJson(config_Path);


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
    return config_Data[key];
}

function setJsonValue(key, value) {
    config_Data[key] = value;
    system.writeJson(config_Path, config_Data);
}

module.exports = {
    formatTime: formatTime,
    formatDay: formatDay,
    formatDate: formatDate,
    getJsonValue: getJsonValue,
    setJsonValue: setJsonValue,
    pad2: pad2
};
