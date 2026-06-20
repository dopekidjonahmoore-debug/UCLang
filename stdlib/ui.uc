def button_create(x, y, w, h, label.
  response([x, y, w, h, label, 0]).
).

def button_update(b.
  mx == input.mouse_x().
  my == input.mouse_y().
  bx == list_get(b, 0).
  by == list_get(b, 1).
  bw == list_get(b, 2).
  bh == list_get(b, 3).
  hover == mx >= bx and mx <= bx + bw and my >= by and my <= by + bh.
  b == list_set(b, 5, hover).
  response([b, hover and input.mouse_just_pressed(1)]).
).

def button_draw(b.
  x == list_get(b, 0).
  y == list_get(b, 1).
  w == list_get(b, 2).
  h == list_get(b, 3).
  label == list_get(b, 4).
  hover == list_get(b, 5).
  If(hover then(
    Overlay.rect(x, y, w, h, "#4488ff").
  ). else(
    Overlay.rect(x, y, w, h, "#3366cc").
  ). ).
  Overlay.rect(x, y, w, h, "#ffffff99").
  Overlay.text(x + 4, y + 4, label, "#ffffff", 16).
).

def label_create(x, y, text, color.
  response([x, y, text, color]).
).

def label_draw(l.
  Overlay.text(list_get(l, 0), list_get(l, 1), list_get(l, 2), "#ffffff", 16).
).

def panel_create(x, y, w, h, color.
  response([x, y, w, h, color]).
).

def panel_draw(p.
  Overlay.rect(list_get(p, 0), list_get(p, 1), list_get(p, 2), list_get(p, 3), list_get(p, 4)).
).

def textbox_create(x, y, w, h.
  response([x, y, w, h, "", 0]).
).

def textbox_update(tb.
  mx == input.mouse_x().
  my == input.mouse_y().
  bx == list_get(tb, 0).
  by == list_get(tb, 1).
  bw == list_get(tb, 2).
  bh == list_get(tb, 3).
  focused == mx >= bx and mx <= bx + bw and my >= by and my <= by + bh and input.mouse_just_pressed(1).
  tb == list_set(tb, 5, focused).
  response(tb).
).

def textbox_draw(tb.
  x == list_get(tb, 0).
  y == list_get(tb, 1).
  w == list_get(tb, 2).
  h == list_get(tb, 3).
  text == list_get(tb, 4).
  focused == list_get(tb, 5).
  If(focused then(
    Overlay.rect(x, y, w, h, "#444466").
  ). else(
    Overlay.rect(x, y, w, h, "#333344").
  ). ).
  Overlay.rect(x, y, w, h, "#888899").
  Overlay.text(x + 4, y + 4, text, "#ffffff", 16).
).
