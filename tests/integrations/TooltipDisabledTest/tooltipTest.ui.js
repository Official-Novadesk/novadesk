// Case 1: Standard tooltip (should show)
ui.addText({
    id: "label1",
    x: 10, y: 10,
    text: "Hover here (Tooltip ENABLED)",
    fontColor: "white",
    fontSize: 24,
    tooltipText: "This tooltip SHOULD show up",
    tooltipTitle: "Tooltip Title",
    tooltipIcon: "error",
    // tooltipBalloon: true,
    onMouseOver: () => {
        console.log("Mouse Overded");
    },
    onLeftMouseDown: function () {
        console.log("Mouse Overded");
    }
});

// Case 2: Disabled tooltip (should NOT show)
ui.addText({
    id: "label2",
    x: 10, y: 50,
    text: "Hover here (Tooltip DISABLED)",
    fontColor: "white",
    fontSize: 24,
    tooltipText: "This tooltip should NOT show up",
    tooltipDisabled: true
});

// Case 3: Verify dynamic updates (getter/setter)
ui.addText({
    id: "label3",
    x: 10, y: 100,
    text: "Click to toggle tooltip",
    fontColor: "cyan",
    fontSize: 24,
    tooltipText: "I am togglable",
    onLeftMouseDown: () => {
        const current = ui.getElementProperty("label3", "tooltipDisabled");
        ui.setElementProperty("label3", { tooltipDisabled: !current });
        console.log("label3 tooltipDisabled is now:", !current);
    }
});