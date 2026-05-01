console.log("=== ElementDragEvents UI Ready ===");
console.log("Drag inside the slider area to test onDragStart/onDrag/onDragEnd.");

const SLIDER_X = 40;
const SLIDER_Y = 90;
const SLIDER_W = 420;
const SLIDER_H = 24;
const THUMB_SIZE = 26;
const THUMB_RADIUS = Math.floor(THUMB_SIZE / 2);
const THUMB_Y = SLIDER_Y + Math.floor((SLIDER_H - THUMB_SIZE) / 2);
const THUMB_MIN_X = SLIDER_X;
const THUMB_MAX_X = SLIDER_X + SLIDER_W - THUMB_SIZE;
const THUMB_TRAVEL = THUMB_MAX_X - THUMB_MIN_X;

let sliderValue = 35;
let thumbGrabOffset = THUMB_RADIUS;

function clamp(v, min, max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

function toInt(v) {
  return Math.round(v);
}

function eventToPercent(e, source) {
  if (!e || typeof e.__clientX !== "number") {
    return sliderValue;
  }

  // Work in thumb-left coordinates so thumb stays fully inside track.
  let thumbLeft = e.__clientX - SLIDER_X;

  // For thumb drag keep the same grab point, so value doesn't jump at edges.
  if (source === "thumb") {
    thumbLeft = e.__clientX - SLIDER_X - thumbGrabOffset;
  }

  thumbLeft = clamp(thumbLeft, 0, THUMB_TRAVEL);
  return clamp((thumbLeft / THUMB_TRAVEL) * 100, 0, 100);
}

function updateThumbGrabOffset(e) {
  if (e && typeof e.__offsetX === "number") {
    thumbGrabOffset = clamp(e.__offsetX, 0, THUMB_SIZE);
  } else {
    thumbGrabOffset = THUMB_RADIUS;
  }
}

function updateSliderVisuals() {
  const thumbX = toInt(THUMB_MIN_X + (sliderValue / 100) * THUMB_TRAVEL);
  const fillWidth = toInt((thumbX - SLIDER_X) + THUMB_RADIUS);

  ui.beginUpdate();
  ui.setElementProperties("slider_fill", {
    width: fillWidth
  });
  ui.setElementProperties("slider_fill_highlight", {
    width: fillWidth
  });

  ui.setElementProperties("slider_thumb", {
    x: clamp(thumbX, THUMB_MIN_X, THUMB_MAX_X),
    y: THUMB_Y
  });
  ui.setElementProperties("slider_thumb_core", {
    x: clamp(thumbX + 4, THUMB_MIN_X + 4, THUMB_MAX_X + 4),
    y: THUMB_Y + 4
  });

  ui.setElementProperties("slider_value_text", {
    text: "Value: " + toInt(sliderValue) + "%"
  });
  ui.endUpdate();
}

function logDrag(eventName, source, e) {
  console.log(
    "[" + eventName + "(" + source + ")] value=" + toInt(sliderValue) +
      " clientX=" + (e && typeof e.__clientX === "number" ? e.__clientX : "n/a") +
      " offsetX=" + (e && typeof e.__offsetX === "number" ? e.__offsetX : "n/a") +
      " offsetXPercent=" + (e && typeof e.__offsetXPercent === "number" ? e.__offsetXPercent : "n/a")
  );
}

function setFromEvent(source, e) {
  sliderValue = clamp(eventToPercent(e, source), 0, 100);
}

function handleDragStart(source, e) {
  if (source === "thumb") {
    updateThumbGrabOffset(e);
  } else {
    thumbGrabOffset = THUMB_RADIUS;
  }
  setFromEvent(source, e);
  updateSliderVisuals();
  logDrag("drag-start", source, e);
}

function handleDragMove(source, e) {
  setFromEvent(source, e);
  updateSliderVisuals();
  logDrag("drag", source, e);
}

function handleDragEnd(source, e) {
  setFromEvent(source, e);
  updateSliderVisuals();
  logDrag("drag-end", source, e);
  thumbGrabOffset = THUMB_RADIUS;
}

function handleTrackDown(e) {
  thumbGrabOffset = THUMB_RADIUS;
  setFromEvent("track", e);
  updateSliderVisuals();
  logDrag("down", "track", e);
}

ui.addText({
  id: "title_text",
  x: 24,
  y: 20,
  width: 470,
  height: 28,
  text: "Element Drag Events Test",
  fontSize: 20,
  fontWeight: "bold",
  fontColor: "rgb(230,230,245)"
});

ui.addText({
  id: "hint_text",
  x: 24,
  y: 52,
  width: 470,
  height: 24,
  text: "Drag on slider track or thumb",
  fontSize: 14,
  fontColor: "rgb(170,170,190)"
});

ui.addShape({
  id: "slider_track_back",
  type: "rectangle",
  x: SLIDER_X,
  y: SLIDER_Y + 1,
  width: SLIDER_W,
  height: SLIDER_H,
  radius: 12,
  fillColor: "rgba(20,26,44,0.70)",
  strokeWidth: 0
});

ui.addShape({
  id: "slider_track",
  type: "rectangle",
  x: SLIDER_X,
  y: SLIDER_Y,
  width: SLIDER_W,
  height: SLIDER_H,
  radius: 12,
  fillColor: "linearGradient(0, rgba(68,78,108,0.55), rgba(58,66,96,0.55))",
  strokeColor: "rgba(190,200,235,0.22)",
  strokeWidth: 1
});

ui.addShape({
  id: "slider_fill",
  type: "rectangle",
  x: SLIDER_X,
  y: SLIDER_Y,
  width: 1,
  height: SLIDER_H,
  radius: 12,
  fillColor: "linearGradient(0, rgb(88,182,255), rgb(76,120,255))",
  strokeWidth: 0
});

ui.addShape({
  id: "slider_fill_highlight",
  type: "rectangle",
  x: SLIDER_X,
  y: SLIDER_Y + 2,
  width: 1,
  height: 8,
  radius: 6,
  fillColor: "linearGradient(0, rgba(255,255,255,0.35), rgba(255,255,255,0.06))",
  strokeWidth: 0
});

ui.addShape({
  id: "slider_hit",
  type: "rectangle",
  x: SLIDER_X,
  y: SLIDER_Y,
  width: SLIDER_W,
  height: SLIDER_H,
  radius: 10,
  fillColor: "rgba(0,0,0,0)",
  onDragStart: function (e) { handleDragStart("track", e); },
  onDrag: function (e) { handleDragMove("track", e); },
  onDragEnd: function (e) { handleDragEnd("track", e); },
  onLeftMouseDown: function (e) { handleTrackDown(e); }
});

ui.addShape({
  id: "slider_thumb",
  type: "rectangle",
  x: SLIDER_X,
  y: THUMB_Y,
  width: THUMB_SIZE,
  height: THUMB_SIZE,
  radius: THUMB_RADIUS,
  fillColor: "linearGradient(90, rgba(248,250,255,1), rgba(228,236,255,1))",
  strokeColor: "rgba(88,120,210,0.55)",
  strokeWidth: 1,
  onDragStart: function (e) { handleDragStart("thumb", e); },
  onDrag: function (e) { handleDragMove("thumb", e); },
  onDragEnd: function (e) { handleDragEnd("thumb", e); }
});

ui.addShape({
  id: "slider_thumb_core",
  type: "rectangle",
  x: SLIDER_X + 4,
  y: THUMB_Y + 4,
  width: THUMB_SIZE - 8,
  height: THUMB_SIZE - 8,
  radius: THUMB_RADIUS - 4,
  fillColor: "linearGradient(90, rgba(126,196,255,0.95), rgba(98,145,255,0.95))",
  strokeWidth: 0
});

ui.addText({
  id: "slider_value_text",
  x: 24,
  y: 142,
  width: 300,
  height: 32,
  text: "Value: " + toInt(sliderValue) + "%",
  fontSize: 22,
  fontWeight: "bold",
  fontColor: "rgb(214,224,255)"
});

updateSliderVisuals();
