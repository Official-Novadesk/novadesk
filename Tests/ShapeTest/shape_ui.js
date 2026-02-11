
var startY = 20;
var spacing = 120;
var changeState = false;


win.addShape({
    id: "shape1",
    type: "rectangle",
    x: 20, y: startY,
    width: 100, height: 80,
    fillColor: "#f00",
    strokeColor: "#000000",
    strokeWidth: 4,
    onLeftMouseDown: function () {
        if (!changeState) {
            win.setElementProperties("shape1", {
                fillColor: "linearGradient(45, #00f, #0ff)",
            });

            console.log("change state is:", changeState)
        } else {
            win.setElementProperties("shape1", {
                fillColor: "#00ff00"
            });
            console.log("change state is:", changeState)
        }
        changeState = !changeState;
    }
});



win.addShape({
    id: "shape2",
    type: "rectangle",
    x: 20, y: startY + spacing,
    width: 100, height: 80,
    radius: 20,
    fillColor: "linearGradient(45, #00f, #0ff)",
    strokeColor: "#000000",
    strokeWidth: 4
});

win.addShape({
    id: "shape3",
    type: "rectangle",
    x: 20, y: startY + spacing * 2,
    width: 100, height: 80,
    radius: 20,
    fillColor: "radialGradient(circle, #ff0, #f00)",
    strokeColor: "#000000",
    strokeWidth: 4
});

win.addShape({
    id: "shape4",
    type: "rectangle",
    x: 20, y: startY + spacing * 3,
    width: 100, height: 80,
    radius: 20,
    fillColor: "radialGradient(ellipse, #ff0, #f00)",
    strokeColor: "#000000",
    strokeWidth: 4
});

win.addShape({
    id: "shape5",
    type: "rectangle",
    x: 20, y: startY + spacing * 4,
    width: 100, height: 80,
    radius: 20,
    fillColor: "#ff00a2",
    strokeColor: "linearGradient(45, #ff0000, #00ff00, #0000ff)",
    strokeWidth: 4,
    backgroundColor: "linearGradient(90, #59ff00, #ff0e46)",
    backgroundColorRadius: 10
});

win.addShape({
    id: "shape7",
    type: "rectangle",
    x: 20 + spacing, y: startY,
    width: 100, height: 80,
    radius: 20,
    fillColor: "radialGradient(ellipse, #ff0, #f00)",
    strokeColor: "#000000",
    strokeWidth: 4,
    rotate: 45,
    backgroundColor: "linearGradient(90, #59ff00, #ff0e46)",
    onLeftMouseDown: function () {
        win.setElementProperties("shape7", {
            rotate: 0
        });
    }
});

win.addShape({
    id: "shape8",
    type: "rectangle",
    x: 20 + spacing, y: startY + spacing,
    width: 100, height: 80,
    radius: 20,
    fillColor: "radialGradient(ellipse, #ff0, #f00)",
    strokeColor: "#000000",
    strokeWidth: 4,
    padding: 10,
    backgroundColor: "linearGradient(90, #59ff00, #ff0e46)",
    onLeftMouseDown: function () {
    }
});

win.addShape({
    id: "shape9",
    type: "rectangle",
    x: 20 + spacing, y: startY + spacing *2,
    width: 100, height: 80,
    radius: 20,
    fillColor: "radialGradient(ellipse, #ff0, #f00)",
    strokeColor: "#000000",
    strokeWidth: 4,
    bevelType:"raised",
    bevelColor:"#4cff0b",
    bevelColor2:"#1135ff",
});

var shape9_W = win.getElementProperty("shape9", "width");
console.log("Width of shape9:", shape9_W);