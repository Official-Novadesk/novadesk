ui.beginUpdate();

// Test 1: display:flex with explicit width/height
ui.addLayoutBox({
    id: "flex-type",
    display: "flex",
    x: "10",
    y: "10",
    padding: 10,
    gap: 10,
    width: 500,
    height: 200,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "shape",
            id: "flexChild1",
            type: "rectangle",
            width: 60,
            height: 50,
            fillColor: "rgba(255, 100, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild2",
            type: "rectangle",
            width: 60,
            height: 60,
            fillColor: "rgba(100, 255, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild3",
            type: "rectangle",
            width: 60,
            height: 40,
            fillColor: "rgba(100, 200, 255, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild4",
            type: "rectangle",
            width: 60,
            height: 70,
            fillColor: "rgba(255, 200, 100, 1)",
            borderRadius: 4
        }
    ]
})

// Test 2: display:none (should be hidden)
ui.addLayoutBox({
    id: "none-type",
    display: "none",
    x: "10",
    y: "230",
    padding: 10,
    gap: 10,
    width: 500,
    height: 200,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "shape",
            id: "noneChild1",
            type: "rectangle",
            width: 60,
            height: 50,
            fillColor: "rgba(255, 100, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "noneChild2",
            type: "rectangle",
            width: 60,
            height: 60,
            fillColor: "rgba(100, 255, 100, 1)",
            borderRadius: 4
        }
    ]
})

// Test 3: display:list-item with listStyleType:disc (default filled circle marker)
ui.addLayoutBox({
    id: "list-item-disc",
    display: "list-item",
    listStyleType: "disc",
    x: "20",
    y: "230",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText1",
            text: "List item with disc marker (filled circle) \n New line goes here",
            x: 20,
            y: 10,
            fontSize: 56,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 4: display:list-item with listStyleType:circle (hollow circle marker)
ui.addLayoutBox({
    id: "list-item-circle",
    display: "list-item",
    listStyleType: "circle",
    x: "20",
    y: "330",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText2",
            text: "List item with circle marker (hollow circle)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 5: display:list-item with listStyleType:square (filled square marker)
ui.addLayoutBox({
    id: "list-item-square",
    display: "list-item",
    listStyleType: "square",
    x: "20",
    y: "430",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText3",
            text: "List item with square marker (filled square)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 6: display:list-item with listStyleType:none (no marker)
ui.addLayoutBox({
    id: "list-item-none",
    display: "list-item",
    listStyleType: "none",
    x: "20",
    y: "530",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText4",
            text: "List item with no marker (listStyleType: none)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 7: display:list-item with listStyleType:upperroman (Roman numerals - auto-indexed)
ui.addLayoutBox({
    id: "list-item-roman1",
    display: "list-item",
    listStyleType: "upper-roman",
    x: "20",
    y: "630",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText5",
            text: "First list item with Roman numeral (auto I.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.addLayoutBox({
    id: "list-item-roman2",
    display: "list-item",
    listStyleType: "upper-roman",
    x: "20",
    y: "730",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 120,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText6",
            text: "Second list item with Roman numeral (auto II.)",
            // NO x/y - let flex layout position it automatically
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        },
        {
            elementType: "text",
            id: "list-itemText65",
            text: "Third list item with Roman numeral (auto III.)",
            // NO x/y - let flex layout position it automatically
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.addLayoutBox({
    id: "list-item-roman3",
    display: "list-item",
    listStyleType: "upper-roman",
    x: "20",
    y: "870",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText67",
            text: "Fourth list item with Roman numeral (auto IV.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.addLayoutBox({
    id: "list-item-roman4",
    display: "list-item",
    listStyleType: "upper-roman",
    x: "20",
    y: "970",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText7",
            text: "Fifth list item with Roman numeral (auto V.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 8: display:list-item with listStyleType:lower-roman (lowercase Roman numerals - auto-indexed)
ui.addLayoutBox({
    id: "list-item-lowerroman1",
    display: "list-item",
    listStyleType: "lower-roman",
    x: "20",
    y: "1060",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText8",
            text: "First list item with lower Roman numeral (auto i.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.addLayoutBox({
    id: "list-item-lowerroman2",
    display: "list-item",
    listStyleType: "lower-roman",
    x: "20",
    y: "1150",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText9",
            text: "Second list item with lower Roman numeral (auto ii.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.addLayoutBox({
    id: "list-item-lowerroman56",
    display: "list-item",
    listStyleType: "upper-alpha",
    x: "20",
    y: "1350",
    flexDirection: "column",
    padding: 10,
    gap: 10,
    width: 500,
    height: 80,
    backgroundColor: "rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "list-itemText9",
            text: "Second list item with lower Roman numeral (auto ii.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        },
                {
            elementType: "text",
            id: "list-itemText95454",
            text: "Second list item with lower Roman numeral (auto ii.)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.endUpdate();
