console.log("=== ContainerHorizontalScrollbar UI Ready ===");

const CONTAINER_X = 40;
const CONTAINER_Y = 80;
const CONTAINER_W = 680;
const CONTAINER_H = 150;

const TRACK_X = CONTAINER_X;
const TRACK_Y = CONTAINER_Y + CONTAINER_H + 16;
const TRACK_W = CONTAINER_W;
const TRACK_H = 12;

const ITEM_W = 120;
const ITEM_H = 110;
const ITEM_GAP = 14;
const ITEM_Y = 20;
const MIN_ITEMS = 1;
const MAX_ITEMS = 40;
let itemCount = 14;
let nextItemId = itemCount;

let contentWidth = 0;
let maxScroll = 0;
let thumbWidth = TRACK_W;
let maxThumbTravel = 0;

let scrollX = 0;
let dragStartClientX = 0;
let dragStartThumbX = TRACK_X;
let isDraggingThumb = false;

function clamp(v, min, max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

function toInt(v) {
  return Math.round(v);
}

function recalcScrollMetrics() {
  contentWidth = (itemCount * ITEM_W) + ((itemCount - 1) * ITEM_GAP) + 20;
  maxScroll = Math.max(0, contentWidth - CONTAINER_W);
  thumbWidth = maxScroll > 0
    ? Math.max(56, Math.floor((CONTAINER_W * CONTAINER_W) / contentWidth))
    : TRACK_W;
  maxThumbTravel = Math.max(0, TRACK_W - thumbWidth);
  scrollX = clamp(scrollX, 0, maxScroll);
}

function addRectangle(id) {
  const x = 10 + (id * (ITEM_W + ITEM_GAP));
  const hueA = 120 + ((id * 7) % 120);
  const hueB = 180 + ((id * 4) % 70);
  ui.addShape({
    id: "item_" + id,
    type: "rectangle",
    container: "container_box",
    x: x,
    y: ITEM_Y,
    width: ITEM_W,
    height: ITEM_H,
    radius: 10,
    fillColor: "linearGradient(0, rgba(" + hueA + ",170,255,0.95), rgba(" + hueB + ",120,255,0.95))",
    strokeColor: "rgba(255,255,255,0.22)",
    strokeWidth: 1
  });

  ui.addText({
    id: "item_label_" + id,
    container: "container_box",
    x: x + 12,
    y: ITEM_Y + 42,
    width: ITEM_W - 24,
    height: 24,
    text: "Card " + (id + 1),
    fontSize: 18,
    fontWeight: "bold",
    fontColor: "rgb(16,22,44)"
  });
}

function getThumbXFromScroll() {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return TRACK_X;
  return TRACK_X + (scrollX / maxScroll) * maxThumbTravel;
}

function getScrollFromThumbX(thumbX) {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return 0;
  const t = (thumbX - TRACK_X) / maxThumbTravel;
  return clamp(t * maxScroll, 0, maxScroll);
}

function updateItemsAndScrollbar() {
  recalcScrollMetrics();
  const thumbX = toInt(getThumbXFromScroll());

  ui.beginUpdate();
  for (let i = 0; i < itemCount; i++) {
    const baseX = 10 + (i * (ITEM_W + ITEM_GAP));
    ui.setElementProperties("item_" + i, {
      x: toInt(baseX - scrollX)
    });
    ui.setElementProperties("item_label_" + i, {
      x: toInt(baseX - scrollX + 12)
    });
  }

  ui.setElementProperties("scroll_thumb", {
    x: thumbX,
    width: thumbWidth
  });

  ui.setElementProperties("scroll_info", {
    text: "Scroll: " + toInt(scrollX) + " / " + toInt(maxScroll) + "   Items: " + itemCount
  });
  ui.endUpdate();
}

function scrollBy(delta) {
  scrollX = clamp(scrollX + delta, 0, maxScroll);
  updateItemsAndScrollbar();
}

function onThumbDragStart(e) {
  isDraggingThumb = true;
  dragStartClientX = (e && typeof e.__clientX === "number") ? e.__clientX : TRACK_X;
  dragStartThumbX = getThumbXFromScroll();
}

function onThumbDrag(e) {
  if (!isDraggingThumb) return;
  const cx = (e && typeof e.__clientX === "number") ? e.__clientX : dragStartClientX;
  const deltaX = cx - dragStartClientX;
  const newThumbX = clamp(dragStartThumbX + deltaX, TRACK_X, TRACK_X + maxThumbTravel);
  scrollX = getScrollFromThumbX(newThumbX);
  updateItemsAndScrollbar();
}

function onThumbDragEnd(e) {
  onThumbDrag(e);
  isDraggingThumb = false;
}

function onAddRectangle() {
  if (itemCount >= MAX_ITEMS) return;
  addRectangle(nextItemId);
  nextItemId += 1;
  itemCount += 1;
  updateItemsAndScrollbar();
}

function onRemoveRectangle() {
  if (itemCount <= MIN_ITEMS) return;
  const id = itemCount - 1;
  ui.removeElements(["item_" + id, "item_label_" + id]);
  itemCount -= 1;
  nextItemId = itemCount;
  updateItemsAndScrollbar();
}

ui.beginUpdate();
ui.addText({
  id: "title",
  x: 24,
  y: 20,
  width: 700,
  height: 34,
  text: "Container Horizontal Rectangles + Scrollbar",
  fontSize: 28,
  fontWeight: "bold",
  fontColor: "rgb(232,236,255)"
});

ui.addText({
  id: "sub",
  x: 24,
  y: 50,
  width: 700,
  height: 24,
  text: "Drag scrollbar thumb or use mouse wheel over container",
  fontSize: 14,
  fontColor: "rgb(164,176,220)"
});

ui.addShape({
  id: "btn_add",
  type: "rectangle",
  x: 574,
  y: 22,
  width: 70,
  height: 30,
  radius: 8,
  fillColor: "linearGradient(0, rgb(82,198,255), rgb(86,142,255))",
  strokeColor: "rgba(255,255,255,0.35)",
  strokeWidth: 1,
  onLeftMouseUp: function () { onAddRectangle(); }
});

ui.addText({
  id: "btn_add_label",
  x: 598,
  y: 29,
  width: 30,
  height: 18,
  text: "Add",
  fontSize: 13,
  fontWeight: "bold",
  fontColor: "rgb(20,28,48)"
});

ui.addShape({
  id: "btn_remove",
  type: "rectangle",
  x: 652,
  y: 22,
  width: 70,
  height: 30,
  radius: 8,
  fillColor: "linearGradient(0, rgb(255,162,162), rgb(255,124,124))",
  strokeColor: "rgba(255,255,255,0.35)",
  strokeWidth: 1,
  onLeftMouseUp: function () { onRemoveRectangle(); }
});

ui.addText({
  id: "btn_remove_label",
  x: 667,
  y: 29,
  width: 48,
  height: 18,
  text: "Remove",
  fontSize: 13,
  fontWeight: "bold",
  fontColor: "rgb(56,18,18)"
});

ui.addShape({
  id: "container_box",
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

for (let i = 0; i < itemCount; i++) {
  addRectangle(i);
}

ui.addShape({
  id: "scroll_track",
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
  id: "scroll_thumb",
  type: "rectangle",
  x: TRACK_X,
  y: TRACK_Y,
  width: thumbWidth,
  height: TRACK_H,
  radius: 6,
  fillColor: "linearGradient(0, rgb(92,188,255), rgb(100,132,255))",
  strokeColor: "rgba(234,242,255,0.60)",
  strokeWidth: 1,
  onDragStart: function (e) { onThumbDragStart(e); },
  onDrag: function (e) { onThumbDrag(e); },
  onDragEnd: function (e) { onThumbDragEnd(e); }
});

ui.addText({
  id: "scroll_info",
  x: CONTAINER_X,
  y: TRACK_Y + 20,
  width: 320,
  height: 24,
  text: "Scroll: 0 / " + toInt(maxScroll),
  fontSize: 14,
  fontColor: "rgb(174,186,232)"
});

ui.endUpdate();
updateItemsAndScrollbar();
