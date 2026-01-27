var mouse = new system.mouse();

var intervalId = setInterval(function () {
    var pos = mouse.position();
    console.log("Mouse Position: X=" + pos.x + ", Y=" + pos.y);
}, 100);

setTimeout(function () {
    clearInterval(intervalId);
    mouse.destroy();
    console.log("Mouse Monitor Destroyed");
}, 5000);