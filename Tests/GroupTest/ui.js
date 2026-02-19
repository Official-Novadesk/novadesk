console.log("=== Group Test Started ===");

win.addText({
    id: "g1_text_1",
    group: "groupA",
    x: 20,
    y: 20,
    text: "Group A - 1",
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addText({
    id: "g1_text_2",
    group: "groupA",
    x: 20,
    y: 50,
    text: "Group A - 2",
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

win.addText({
    id: "g2_text_1",
    group: "groupB",
    x: 20,
    y: 90,
    text: "Group B - 1",
    fontSize: 14,
    fontColor: "rgb(255,255,255)"
});

console.log("g1_text_1 group: " + win.getElementProperty("g1_text_1", "group"));
console.log("g2_text_1 group: " + win.getElementProperty("g2_text_1", "group"));

win.setElementPropertiesByGroup("groupA", {
    x: 260,
    fontColor: "rgb(80,220,120)"
});

console.log("After groupA update, g1_text_1 x: " + win.getElementProperty("g1_text_1", "x"));
console.log("After groupA update, g1_text_2 x: " + win.getElementProperty("g1_text_2", "x"));
console.log("After groupA update, g2_text_1 x: " + win.getElementProperty("g2_text_1", "x"));

win.removeElementsByGroup("groupA");

console.log("After remove groupA, g1_text_1 exists: " + (win.getElementProperty("g1_text_1", "text") !== null));
console.log("After remove groupA, g1_text_2 exists: " + (win.getElementProperty("g1_text_2", "text") !== null));
console.log("After remove groupA, g2_text_1 exists: " + (win.getElementProperty("g2_text_1", "text") !== null));

console.log("=== Group Test Completed ===");
