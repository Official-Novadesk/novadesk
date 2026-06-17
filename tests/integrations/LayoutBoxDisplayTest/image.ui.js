// Test 3: display:list-item with listStyleType:disc (default filled circle marker)
ui.addLayoutBox({
    id: "list-item-disc",
    display: "list-item",
    listStyleType: "disc",
    x: "50",
    y: "230",
    flexDirection: "column",
    padding: 10,
    // gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            // elementType: "image",
            // id: "list-item1",
            //  path: "../assets/pic.png",
                        elementType: "text",
            id: "list-item1",
             text: "Test",
             fontSize:50
        }
    ]
})
