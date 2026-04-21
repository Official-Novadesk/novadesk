import {widgetWindow} from "novadesk"

var widget = new widgetWindow({
    id: "TooltipDisabledTest",
    title: "Tooltip Disabled Test",
    width: 400,
    height: 400,
    script: "tooltipTest.ui.js",
    backgroundColor: "rgb(20,20,20)"
})
