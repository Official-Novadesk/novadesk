const clock_Widget = require('./src/clockWidget/main');
const utils = require('./src/common/utils');

if (utils.getJsonValue('clock_Widget_Active')) {
    clock_Widget.loadClockWidget();
}
