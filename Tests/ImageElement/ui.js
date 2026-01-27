win.addImage({
    id:"pic-demo",
    path: "../assets/pic.png",
    x: 10,
    y: 10,
    width: 100,
    height: 100
});

win.addText({
    id:"txt-demo",
    text: "X=10\nY=10\nW=100\nH=100",
    x: 30,
    y: 120,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)"
});

win.addImage({
    id:"pic-demo2",
    path: "../assets/pic.png",
    x: 120,
    y: 10,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)"
});

win.addText({
    id:"txt-demo2",
    text: "X=120\nY=10\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)",
    x: 150,
    y: 120,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)"
});

win.addImage({
    id:"pic-demo3",
    path: "../assets/pic.png",
    x: 340,
    y: 10,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)",
    solidColorRadius: 10
});

win.addText({
    id:"txt-demo3",
    text: "X=340\nY=10\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)\nSolid Color Radius=10",
    x: 350,
    y: 120,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)"
});


win.addImage({
    id:"pic-demo4",
    path: "../assets/pic.png",
    x: 520,
    y: 10,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)",
    solidColorRadius: 10,
    rotate:45
});

win.addText({
    id:"txt-demo4",
    text: "X=520\nY=10\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)\nSolid Color Radius=10\nRotate=45",
    x: 540,
    y: 150,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)"
});

win.addImage({
    id:"pic-demo5",
    path: "../assets/pic.png",
    x: 10,
    y: 280,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)",
    solidColorRadius: 10,
    rotate:45,
    antiAlias: false
});

win.addText({
    id:"txt-demo5",
    text: "X=10\nY=280\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)\nSolid Color Radius=10\nRotate=45\nAnti-Alias=false",
    x: 30,
    y: 420,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)",
});

win.addImage({
    id:"pic-demo6",
    path: "../assets/pic.png",
    x: 280,
    y: 280,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)",
    solidColorRadius: 10,
    rotate:45,
    antiAlias: false,
    bevelType:"raised",
    bevelWidth:2,
    bevelColor: "rgb(13, 244, 40)", 
    bevelColor2: "rgb(0, 0, 255)"

});

win.addText({
    id:"txt-demo6",
    text: "X=280\nY=280\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)\nSolid Color Radius=10\nRotate=45\nAnti-Alias=false\nbevelType:\"raised\"\nbevelWidth:2\nbevelColor: \"rgb(13, 244, 40)\"\nbevelColor2: \"rgb(0, 0, 255)\"",
    x: 300,
    y: 420,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)",
});

win.addImage({
    id:"pic-demo7",
    path: "../assets/pic.png",
    x: 580,
    y: 280,
    width: 100,
    height: 100,
    solidColor: "rgb(255, 0, 0)",
    solidColorRadius: 10,
    rotate:45,
    antiAlias: false,
    bevelType:"sunken",
    bevelWidth:2,
    bevelColor: "rgb(13, 244, 40)", 
    bevelColor2: "rgb(0, 0, 255)"

});

win.addText({
    id:"txt-demo7",
    text: "X=580\nY=280\nW=100\nH=100\nSolid Color=rgb(255, 0, 0)\nSolid Color Radius=10\nRotate=45\nAnti-Alias=false\nbevelType:\"sunken\"\nbevelWidth:2\nbevelColor: \"rgb(13, 244, 40)\"\nbevelColor2: \"rgb(0, 0, 255)\"",
    x: 600,
    y: 420,
    fontSize: 15,
    fontColor: "rgb(255, 255, 255)",
});