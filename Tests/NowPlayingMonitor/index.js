var nowPlaying = new system.nowPlaying();

var intervalId = setInterval(function () {
    var stats = nowPlaying.stats();
    console.log("NowPlaying:", JSON.stringify(stats));
}, 1000);

// setTimeout(function () {
//     clearInterval(intervalId);
//     nowPlaying.destroy();
//     console.log("NowPlaying monitor destroyed");
// }, 10000);

setTimeout(function () {
    nowPlaying.setPosition(50)
    // clearInterval(intervalId);
    // nowPlaying.destroy();
    console.log("NowPlaying monitor position set to 50");
}, 5000);
