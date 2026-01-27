var hotkeyId = system.registerHotkey("Win+D", function() {
    console.log("Win+D pressed");
});

var hotkeyId2 = system.registerHotkey("Space", {
    onKeyDown: function() {
        console.log("SPACE DOWN (Global)");
    },
    onKeyUp: function() {
        console.log("SPACE UP (Global)");
    }
});

setTimeout(function() {
    // Later, unregister the hotkey
    system.unregisterHotkey(hotkeyId);
    console.log("Hotkey Id unregistered");
    system.unregisterHotkey(hotkeyId2);
    console.log("Hotkey Id2 unregistered");
}, 5000);