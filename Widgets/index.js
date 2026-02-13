const clock_Widget = require('./src/clockWidget/main');
const utils = require('./src/common/utils');
const system_Widget = require('./src/systemWidget/main');

if (utils.getJsonValue('clock_Widget_Active')) {
    clock_Widget.loadClockWidget();
}
system_Widget.loadSystemWidget();
