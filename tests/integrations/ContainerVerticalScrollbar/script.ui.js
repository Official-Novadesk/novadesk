console.log("=== ContainerVerticalScrollbar UI Ready ===");

const CONTAINER_X = 40;
const CONTAINER_Y = 90;
const CONTAINER_W = 420;
const CONTAINER_H = 540;

const TRACK_X = CONTAINER_X + CONTAINER_W + 16;
const TRACK_Y = CONTAINER_Y;
const TRACK_W = 12;
const TRACK_H = CONTAINER_H;

const ITEM_X = 14;
const ITEM_Y = 14;
const ITEM_W = 388;
const ITEM_H = 90;
const ITEM_GAP = 12;
const ITEM_COUNT = 16;

const contentHeight = (ITEM_COUNT * ITEM_H) + ((ITEM_COUNT - 1) * ITEM_GAP) + (ITEM_Y * 2);
const maxScroll = Math.max(0, contentHeight - CONTAINER_H);
const thumbHeight = maxScroll > 0
  ? Math.max(56, Math.floor((CONTAINER_H * CONTAINER_H) / contentHeight))
  : TRACK_H;
const maxThumbTravel = Math.max(0, TRACK_H - thumbHeight);

let scrollY = 0;
let dragStartClientY = 0;
let dragStartThumbY = TRACK_Y;
let isDraggingThumb = false;

function clamp(v, min, max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

function toInt(v) {
  return Math.round(v);
}

function getThumbYFromScroll() {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return TRACK_Y;
  return TRACK_Y + (scrollY / maxScroll) * maxThumbTravel;
}

function getScrollFromThumbY(thumbY) {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return 0;
  const t = (thumbY - TRACK_Y) / maxThumbTravel;
  return clamp(t * maxScroll, 0, maxScroll);
}

function updateItemsAndScrollbar() {
  const thumbY = toInt(getThumbYFromScroll());

  ui.beginUpdate();
  for (let i = 0; i < ITEM_COUNT; i++) {
    const baseY = ITEM_Y + (i * (ITEM_H + ITEM_GAP));
    ui.setElementProperties("v_item_" + i, {
      y: toInt(baseY - scrollY)
    });
    ui.setElementProperties("v_item_title_" + i, {
      y: toInt(baseY - scrollY + 12)
    });
    ui.setElementProperties("v_item_sub_" + i, {
      y: toInt(baseY - scrollY + 46)
    });
  }

  ui.setElementProperties("v_scroll_thumb", {
    y: thumbY,
    height: thumbHeight
  });

  ui.setElementProperties("v_scroll_info", {
    text: "Scroll: " + toInt(scrollY) + " / " + toInt(maxScroll)
  });
  ui.endUpdate();
}

function scrollBy(delta) {
  scrollY = clamp(scrollY + delta, 0, maxScroll);
  updateItemsAndScrollbar();
}

function onThumbDragStart(e) {
  isDraggingThumb = true;
  dragStartClientY = (e && typeof e.__clientY === "number") ? e.__clientY : TRACK_Y;
  dragStartThumbY = getThumbYFromScroll();
}

function onThumbDrag(e) {
  if (!isDraggingThumb) return;
  const cy = (e && typeof e.__clientY === "number") ? e.__clientY : dragStartClientY;
  const deltaY = cy - dragStartClientY;
  const newThumbY = clamp(dragStartThumbY + deltaY, TRACK_Y, TRACK_Y + maxThumbTravel);
  scrollY = getScrollFromThumbY(newThumbY);
  updateItemsAndScrollbar();
}

function onThumbDragEnd(e) {
  onThumbDrag(e);
  isDraggingThumb = false;
}

ui.beginUpdate();

ui.addText({
  id: "v_title",
  x: 24,
  y: 20,
  width: 500,
  height: 34,
  text: "Container Vertical Rectangles + Scrollbar",
  fontSize: 26,
  fontWeight: "bold",
  fontColor: "rgb(232,236,255)"
});

ui.addText({
  id: "v_sub",
  x: 24,
  y: 52,
  width: 500,
  height: 24,
  text: "Use mouse wheel over container or drag the vertical thumb",
  fontSize: 14,
  fontColor: "rgb(164,176,220)"
});

ui.addShape({
  id: "v_container_box",
  type: "rectangle",
  x: CONTAINER_X,
  y: CONTAINER_Y,
  width: CONTAINER_W,
  height: CONTAINER_H,
  radius: 12,
  fillColor: "linearGradient(90, rgba(28,32,52,0.92), rgba(22,26,44,0.92))",
  strokeColor: "rgba(116,132,190,0.40)",
  strokeWidth: 1,
  onScrollUp: function () { scrollBy(-48); },
  onScrollDown: function () { scrollBy(48); }
});

for (let i = 0; i < ITEM_COUNT; i++) {
  const y = ITEM_Y + (i * (ITEM_H + ITEM_GAP));
  const hueA = 120 + ((i * 9) % 120);
  const hueB = 180 + ((i * 5) % 70);

  ui.addShape({
    id: "v_item_" + i,
    type: "rectangle",
    container: "v_container_box",
    x: ITEM_X,
    y: y,
    width: ITEM_W,
    height: ITEM_H,
    radius: 12,
    fillColor: "linearGradient(0, rgba(" + hueA + ",170,255,0.95), rgba(" + hueB + ",120,255,0.95))",
    strokeColor: "rgba(255,255,255,0.24)",
    strokeWidth: 1
  });

  ui.addText({
    id: "v_item_title_" + i,
    container: "v_container_box",
    x: ITEM_X + 14,
    y: y + 12,
    width: ITEM_W - 24,
    height: 28,
    text: "Vertical Card " + (i + 1),
    fontSize: 20,
    fontWeight: "bold",
    fontColor: "rgb(18,24,46)"
  });

  ui.addText({
    id: "v_item_sub_" + i,
    container: "v_container_box",
    x: ITEM_X + 14,
    y: y + 46,
    width: ITEM_W - 24,
    height: 24,
    text: "Rectangle inside container",
    fontSize: 14,
    fontColor: "rgba(18,24,46,0.80)"
  });
}

ui.addShape({
  id: "v_scroll_track",
  type: "rectangle",
  x: TRACK_X,
  y: TRACK_Y,
  width: TRACK_W,
  height: TRACK_H,
  radius: 6,
  fillColor: "rgba(64,76,120,0.45)",
  strokeColor: "rgba(162,178,236,0.24)",
  strokeWidth: 1
});

ui.addShape({
  id: "v_scroll_thumb",
  type: "rectangle",
  x: TRACK_X,
  y: TRACK_Y,
  width: TRACK_W,
  height: thumbHeight,
  radius: 6,
  fillColor: "linearGradient(90, rgb(92,188,255), rgb(100,132,255))",
  strokeColor: "rgba(234,242,255,0.60)",
  strokeWidth: 1,
  onDragStart: function (e) { onThumbDragStart(e); },
  onDrag: function (e) { onThumbDrag(e); },
  onDragEnd: function (e) { onThumbDragEnd(e); }
});

ui.addText({
  id: "v_scroll_info",
  x: 40,
  y: 640,
  width: 460,
  height: 24,
  text: "Scroll: 0 / " + toInt(maxScroll),
  fontSize: 14,
  fontColor: "rgb(174,186,232)"
});

ui.endUpdate();

updateItemsAndScrollbar();
