const padUtil = require('./src/utils');

var variables_Path = path.join(app.getAppDataPath(), 'config.json');

console.log('Variables Path: ' + variables_Path);

var variables_Data = system.readJson(variables_Path);

console.log('Variables Data: ' + JSON.stringify(variables_Data));

clock_Widget = new widgetWindow({
  id: 'clock_Window',
  script: 'clock/clock.js',
  width: 212,
  height: 72,
  zPos: "ontop",
  show: !app.isFirstRun()
})

// console.log('Is First Run: ' + app.isFirstRun());

function formatTime(dateObj, use12h) {
  var h = dateObj.getHours();
  var m = dateObj.getMinutes();
  var s = dateObj.getSeconds();

  if (use12h) {
    var h12 = h % 12;
    if (h12 === 0) h12 = 12;
    var ampm = h >= 12 ? "PM" : "AM";
    return padUtil.pad2(h12) + ":" + padUtil.pad2(m) + ":" + padUtil.pad2(s) + " " + ampm;
  }

  return padUtil.pad2(h) + ":" + padUtil.pad2(m) + ":" + padUtil.pad2(s);
}

var use12h = false;
var now = new Date();
var timeStr = formatTime(now, use12h);
console.log("Current Time: " + timeStr);