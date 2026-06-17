button_create(x, y, w, h, label.
  response([x, y, w, h, label, 0]).
).

button_update(b.
  mx == Input.mouse_x().
  my == Input.mouse_y().
  bx == list_get(b, 0).
  by == list_get(b, 1).
  bw == list_get(b, 2).
  bh == list_get(b, 3).
  hover == Rect.contains(bx, by, bw, bh, mx, my).
  b == list_set(b, 5, hover).
  clicked == hover and Input.mouse_just(1).
  response([b, clicked]).
).

button_draw(b.
  x == list_get(b, 0).
  y == list_get(b, 1).
  w == list_get(b, 2).
  h == list_get(b, 3).
  label == list_get(b, 4).
  hover == list_get(b, 5).
  If(hover then(
    Draw.rect(x, y, w, h, "#4488ff").
  ). else(
    Draw.rect(x, y, w, h, "#3366cc").
  ). ).
  Draw.rect_outline(x, y, w, h, "#ffffff").
  tx == x + Math.floor(w / 2) - Math.floor(list_len(label) * 4).
  ty == y + Math.floor(h / 2) - 6.
  Draw.text(tx, ty, label, "#ffffff").
).

label_create(x, y, text, color.
  response([x, y, text, color]).
).

label_draw(l.
  Draw.text(list_get(l, 0), list_get(l, 1), list_get(l, 2), list_get(l, 3)).
).

panel_create(x, y, w, h, color.
  response([x, y, w, h, color]).
).

panel_draw(p.
  Draw.rect(list_get(p, 0), list_get(p, 1), list_get(p, 2), list_get(p, 3), list_get(p, 4)).
).

textbox_create(x, y, w, h.
  response([x, y, w, h, "", 0]).
).

textbox_update(tb.
  mx == Input.mouse_x().
  my == Input.mouse_y().
  focused == Rect.contains(list_get(tb, 0), list_get(tb, 1), list_get(tb, 2), list_get(tb, 3), mx, my) and Input.mouse_just(1).
  tb == list_set(tb, 5, focused).
  response(tb).
).

textbox_draw(tb.
  x == list_get(tb, 0).
  y == list_get(tb, 1).
  w == list_get(tb, 2).
  h == list_get(tb, 3).
  text == list_get(tb, 4).
  focused == list_get(tb, 5).
  If(focused then(
    Draw.rect(x, y, w, h, "#444466").
  ). else(
    Draw.rect(x, y, w, h, "#333344").
  ). ).
  Draw.rect_outline(x, y, w, h, "#888899").
  Draw.text(x + 4, y + 4, text, "#ffffff").
).
