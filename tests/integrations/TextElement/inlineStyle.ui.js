var startY = 20;
var spacing = 45;

ui.addText({
    id: "basic-styles",
    x: 20, y: startY,
    text: "Basic: <b>Bold</b>, <i>Italic</i>, \n <u>Underline</u>, <s>Strike</s>",
    fontSize: 18,
    fontColor: "#FFFFFF",
    textSelection: true,
    // selectionTextColor:"blue",
    // selectionBackgroundColor:"#eb232332"
});

ui.addText({
    id: "nested-styles",
    x: 20, y: startY + spacing,
    text: "Nested: <b><i>Bold Italic <u>Underlined</u></i></b>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "solid-color",
    x: 20, y: startY + spacing * 2,
    text: "Colors: <color=#FF0000>Red</color>, <color=#00FF00>Green</color>, <color=#0000FF>Blue</color>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "linear-gradient",
    x: 20, y: startY + spacing * 3,
    text: "Linear: <color=linearGradient(toRight, #FF0000, #FFFF00)>Red to Yellow Gradient</color>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "radial-gradient",
    x: 20, y: startY + spacing * 4,
    text: "Radial: <color=radialGradient(circle, #00FFFF, #0000FF)>Cyan to Blue Circle</color>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "size-font",
    x: 20, y: startY + spacing * 5,
    text: "Mixed: <size=30>Big</size> <size=12>Small</size> <font=Consolas>Consolas</font> <font=Times New Roman>Times</font>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "case-test",
    x: 20, y: startY + spacing * 6.5,
    text: "Case: <case=upper>upper</case>, <case=lower>LOWER</case>, <case=capitalize>capitalize me</case>, <case=sentence>THIS IS A SENTENCE.</case>",
    fontSize: 18,
    fontColor: "#FFFFFF"
});

ui.addText({
    id: "complex-combo",
    x: 20, y: startY + spacing * 8,
    text: "<size=24><b><color=linearGradient(toBottom, #FFF, #555)>STYLIZED TITLE</color></b></size>\n<color=#AAA>Subtitle with <i>italics</i> and <u>underline</u></color>",
    fontSize: 14,
    fontColor: "#FFFFFF",
    width: 360,
    textClip: "wrap"
});

ui.addText({
    id: "icon-test",
    x: 20, y: startY + spacing * 9,
    // \uE700 is the 'Settings' gear icon in Segoe MDL2 Assets
    text: "<font=Segoe MDL2 Assets>\uE700</font> Settings",
    fontSize: 24,
    fontColor: "#FFFFFF"
});

console.log("Inline styling test widget script loaded.");
