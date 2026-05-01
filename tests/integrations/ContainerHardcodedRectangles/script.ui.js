console.log("=== ContainerHardcodedRectangles UI Ready ===");

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
const ITEM_COUNT = 10;

const contentWidth = (ITEM_COUNT * ITEM_W) + ((ITEM_COUNT - 1) * ITEM_GAP) + 20;
const maxScroll = Math.max(0, contentWidth - CONTAINER_W);
const thumbWidth = maxScroll > 0
  ? Math.max(56, Math.floor((CONTAINER_W * CONTAINER_W) / contentWidth))
  : TRACK_W;
const maxThumbTravel = Math.max(0, TRACK_W - thumbWidth);

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

function getThumbXFromScroll() {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return TRACK_X;
  return TRACK_X + (scrollX / maxScroll) * maxThumbTravel;
}

function getScrollFromThumbX(thumbX) {
  if (maxScroll <= 0 || maxThumbTravel <= 0) return 0;
  const t = (thumbX - TRACK_X) / maxThumbTravel;
  return clamp(t * maxScroll, 0, maxScroll);
}

function setCardPosition(index, baseX) {
  ui.setElementProperties("item_" + index, { x: toInt(baseX - scrollX) });
  ui.setElementProperties("item_label_" + index, { x: toInt(baseX - scrollX + 12) });
}

function updateItemsAndScrollbar() {
  const thumbX = toInt(getThumbXFromScroll());

  ui.beginUpdate();
  setCardPosition(0, 10 + (0 * (ITEM_W + ITEM_GAP)));
  setCardPosition(1, 10 + (1 * (ITEM_W + ITEM_GAP)));
  setCardPosition(2, 10 + (2 * (ITEM_W + ITEM_GAP)));
  setCardPosition(3, 10 + (3 * (ITEM_W + ITEM_GAP)));
  setCardPosition(4, 10 + (4 * (ITEM_W + ITEM_GAP)));
  setCardPosition(5, 10 + (5 * (ITEM_W + ITEM_GAP)));
  setCardPosition(6, 10 + (6 * (ITEM_W + ITEM_GAP)));
  setCardPosition(7, 10 + (7 * (ITEM_W + ITEM_GAP)));
  setCardPosition(8, 10 + (8 * (ITEM_W + ITEM_GAP)));
  setCardPosition(9, 10 + (9 * (ITEM_W + ITEM_GAP)));

  ui.setElementProperties("scroll_thumb", {
    x: thumbX,
    width: thumbWidth
  });

  ui.setElementProperties("scroll_info", {
    text: "Scroll: " + toInt(scrollX) + " / " + toInt(maxScroll) + "   (Hardcoded cards)"
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

ui.beginUpdate();

ui.addText({
  id: "title",
  x: 24,
  y: 20,
  width: 700,
  height: 34,
  text: "Hardcoded Rectangles (No Loop Creation)",
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

ui.addShape({ id: "item_0", type: "rectangle", container: "container_box", x: 10, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(120,170,255,0.95), rgba(180,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_0", container: "container_box", x: 22, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 1", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_1", type: "rectangle", container: "container_box", x: 144, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(130,170,255,0.95), rgba(184,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_1", container: "container_box", x: 156, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 2", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_2", type: "rectangle", container: "container_box", x: 278, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(140,170,255,0.95), rgba(188,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_2", container: "container_box", x: 290, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 3", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_3", type: "rectangle", container: "container_box", x: 412, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(150,170,255,0.95), rgba(192,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_3", container: "container_box", x: 424, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 4", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_4", type: "rectangle", container: "container_box", x: 546, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(160,170,255,0.95), rgba(196,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_4", container: "container_box", x: 558, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 5", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_5", type: "rectangle", container: "container_box", x: 680, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(170,170,255,0.95), rgba(200,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_5", container: "container_box", x: 692, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 6", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_6", type: "rectangle", container: "container_box", x: 814, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(180,170,255,0.95), rgba(204,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_6", container: "container_box", x: 826, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 7", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_7", type: "rectangle", container: "container_box", x: 948, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(190,170,255,0.95), rgba(208,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_7", container: "container_box", x: 960, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 8", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_8", type: "rectangle", container: "container_box", x: 1082, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(200,170,255,0.95), rgba(212,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_8", container: "container_box", x: 1094, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 9", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

ui.addShape({ id: "item_9", type: "rectangle", container: "container_box", x: 1216, y: ITEM_Y, width: ITEM_W, height: ITEM_H, radius: 10, fillColor: "linearGradient(0, rgba(210,170,255,0.95), rgba(216,120,255,0.95))", strokeColor: "rgba(255,255,255,0.22)", strokeWidth: 1 });
ui.addText({ id: "item_label_9", container: "container_box", x: 1228, y: ITEM_Y + 42, width: ITEM_W - 24, height: 24, text: "Card 10", fontSize: 18, fontWeight: "bold", fontColor: "rgb(16,22,44)" });

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
  width: 420,
  height: 24,
  text: "Scroll: 0 / " + toInt(maxScroll),
  fontSize: 14,
  fontColor: "rgb(174,186,232)"
});

ui.endUpdate();

updateItemsAndScrollbar();
