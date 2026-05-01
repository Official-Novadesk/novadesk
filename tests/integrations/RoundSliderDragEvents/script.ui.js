console.log("=== RoundSliderDragEvents UI Ready ===");
console.log("RoundLine slider with thumb drag enabled.");

const CENTER_X = 260;
const CENTER_Y = 205;
const RING_RADIUS = 110;
const RING_SIZE = RING_RADIUS * 2;
const RING_X = CENTER_X - RING_RADIUS;
const RING_Y = CENTER_Y - RING_RADIUS;
const RING_THICKNESS = 16;

const ROUNDLINE_START_ANGLE = 0;
const TOTAL_ANGLE = 360;
const START_ANGLE = ROUNDLINE_START_ANGLE - 90;

const THUMB_SIZE = 28;
const THUMB_RADIUS = Math.floor(THUMB_SIZE / 2);

let sliderValue = 35;
let isThumbDragging = false;
let lastDragRawNorm = 0;
let dragAngleAccum = 0;

function clamp(v, min, max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

function toInt(v) {
  return Math.round(v);
}

function degToRad(deg) {
  return (deg * Math.PI) / 180;
}

function valueToAngle(value) {
  return START_ANGLE + (clamp(value, 0, 100) / 100) * TOTAL_ANGLE;
}

function normalizeDeg360(angle) {
  let a = angle % 360;
  if (a < 0) a += 360;
  return a;
}

function getRawNormFromEvent(e) {
  if (!e || typeof e.__clientX !== "number" || typeof e.__clientY !== "number") {
    return null;
  }

  const dx = e.__clientX - CENTER_X;
  const dy = e.__clientY - CENTER_Y;
  const rawAngle = (Math.atan2(dy, dx) * 180) / Math.PI;
  return normalizeDeg360(rawAngle);
}

function setFromEvent(e, phase) {
  const rawNorm = getRawNormFromEvent(e);
  if (rawNorm === null) {
    return;
  }

  // For full circle, track incremental angle delta to avoid seam jumps.
  if (TOTAL_ANGLE >= 360) {
    if (phase === "start") {
      isThumbDragging = true;
      lastDragRawNorm = rawNorm;
      dragAngleAccum = clamp((sliderValue / 100) * TOTAL_ANGLE, 0, TOTAL_ANGLE);
      return;
    }

    if (phase === "move" || phase === "end") {
      if (!isThumbDragging) {
        isThumbDragging = true;
        lastDragRawNorm = rawNorm;
        dragAngleAccum = clamp((sliderValue / 100) * TOTAL_ANGLE, 0, TOTAL_ANGLE);
      }

      let delta = rawNorm - lastDragRawNorm;
      if (delta > 180) delta -= 360;
      if (delta < -180) delta += 360;

      dragAngleAccum = clamp(dragAngleAccum + delta, 0, TOTAL_ANGLE);
      lastDragRawNorm = rawNorm;
      sliderValue = clamp((dragAngleAccum / TOTAL_ANGLE) * 100, 0, 100);

      if (phase === "end") {
        isThumbDragging = false;
      }
      return;
    }
  }

  // Fallback for partial arc sliders.
  const startNorm = normalizeDeg360(START_ANGLE);
  const clockwiseDelta = normalizeDeg360(rawNorm - startNorm);
  sliderValue = clamp((clockwiseDelta / TOTAL_ANGLE) * 100, 0, 100);
}

function updateSliderVisuals() {
  const angle = valueToAngle(sliderValue);
  const rad = degToRad(angle);
  const thumbCenterX = CENTER_X + Math.cos(rad) * RING_RADIUS;
  const thumbCenterY = CENTER_Y + Math.sin(rad) * RING_RADIUS;
  const thumbX = toInt(thumbCenterX - THUMB_RADIUS);
  const thumbY = toInt(thumbCenterY - THUMB_RADIUS);

  ui.beginUpdate();
  ui.setElementProperties("slider_ring", {
    value: clamp(sliderValue / 100, 0, 1)
  });
  ui.setElementProperties("slider_thumb", {
    x: thumbX,
    y: thumbY
  });
  ui.setElementProperties("slider_thumb_core", {
    x: thumbX + 4,
    y: thumbY + 4
  });
  ui.setElementProperties("slider_value_text", {
    text: "Value: " + toInt(sliderValue) + "%"
  });
  ui.setElementProperties("slider_angle_text", {
    text: "Angle: " + toInt(angle) + "deg"
  });
  ui.endUpdate();
}

function logDrag(eventName, e) {
  console.log(
    "[" + eventName + "] value=" + toInt(sliderValue) +
      " clientX=" + (e && typeof e.__clientX === "number" ? e.__clientX : "n/a") +
      " clientY=" + (e && typeof e.__clientY === "number" ? e.__clientY : "n/a")
  );
}

function handleDragStart(e) {
  setFromEvent(e, "start");
  updateSliderVisuals();
  logDrag("drag-start(thumb)", e);
}

function handleDragMove(e) {
  setFromEvent(e, "move");
  updateSliderVisuals();
  logDrag("drag(thumb)", e);
}

function handleDragEnd(e) {
  setFromEvent(e, "end");
  updateSliderVisuals();
  logDrag("drag-end(thumb)", e);
}

ui.addText({
  id: "title_text",
  x: 24,
  y: 20,
  width: 470,
  height: 30,
  text: "Round Slider (RoundLine)",
  fontSize: 28,
  fontWeight: "bold",
  fontColor: "rgb(230,230,245)"
});

ui.addText({
  id: "hint_text",
  x: 24,
  y: 54,
  width: 470,
  height: 24,
  text: "Drag thumb only",
  fontSize: 14,
  fontColor: "rgb(170,170,190)"
});

ui.addShape({
  id: "slider_glow",
  type: "ellipse",
  x: RING_X - 24,
  y: RING_Y - 24,
  width: RING_SIZE + 48,
  height: RING_SIZE + 48,
  fillColor: "radialGradient(circle, rgba(54,84,188,0.20), rgba(0,0,0,0))",
  strokeWidth: 0
});

ui.addRoundLine({
  id: "slider_ring",
  x: RING_X,
  y: RING_Y,
  width: RING_SIZE,
  height: RING_SIZE,
  radius: RING_RADIUS,
  thickness: RING_THICKNESS,
  startAngle: ROUNDLINE_START_ANGLE,
  totalAngle: TOTAL_ANGLE,
  clockwise: true,
  capType: "round",
  lineColorBg: "rgba(82,92,132,0.70)",
  lineColor: "linearGradient(0, rgb(90,188,255), rgb(104,126,255))",
  value: sliderValue / 100
});

ui.addShape({
  id: "slider_thumb",
  type: "ellipse",
  x: CENTER_X + RING_RADIUS - THUMB_RADIUS,
  y: CENTER_Y - THUMB_RADIUS,
  width: THUMB_SIZE,
  height: THUMB_SIZE,
  fillColor: "linearGradient(90, rgba(251,252,255,1), rgba(234,239,255,1))",
  strokeColor: "rgba(95,124,214,0.65)",
  strokeWidth: 1,
  onDragStart: function (e) { handleDragStart(e); },
  onDrag: function (e) { handleDragMove(e); },
  onDragEnd: function (e) { handleDragEnd(e); }
});

ui.addShape({
  id: "slider_thumb_core",
  type: "ellipse",
  x: CENTER_X + RING_RADIUS - THUMB_RADIUS + 4,
  y: CENTER_Y - THUMB_RADIUS + 4,
  width: THUMB_SIZE - 8,
  height: THUMB_SIZE - 8,
  fillColor: "linearGradient(90, rgba(130,206,255,0.98), rgba(100,148,255,0.98))",
  strokeWidth: 0,
  onDragStart: function (e) { handleDragStart(e); },
  onDrag: function (e) { handleDragMove(e); },
  onDragEnd: function (e) { handleDragEnd(e); }
});

ui.addText({
  id: "slider_value_text",
  x: 24,
  y: 336,
  width: 230,
  height: 36,
  text: "Value: " + toInt(sliderValue) + "%",
  fontSize: 28,
  fontWeight: "bold",
  fontColor: "rgb(214,224,255)"
});

ui.addText({
  id: "slider_angle_text",
  x: 260,
  y: 342,
  width: 230,
  height: 26,
  text: "Angle: " + toInt(valueToAngle(sliderValue)) + "deg",
  fontSize: 18,
  fontColor: "rgb(170,184,240)"
});

updateSliderVisuals();
