def graphics_draw_rect(x, y, w, h, color.
  Overlay.rect(x, y, w, h, color).
).

def graphics_draw_circle(cx, cy, radius, color.
  Overlay.circle(cx, cy, radius, color).
).

def graphics_draw_text(x, y, text, color, size.
  Overlay.text(x, y, text, color, size).
).

def graphics_draw_sprite(eid.
  If(entity_has(eid, "position") and entity_has(eid, "sprite") then(
    pos == entity_get(eid, "position").
    sprite == entity_get(eid, "sprite").
    px == list_get(pos, 0).
    py == list_get(pos, 1).
    sw == list_get(sprite, 0).
    sh == list_get(sprite, 1).
    color == list_get(sprite, 2).
    ox == px - sw / 2.
    oy == py - sh / 2.
    Overlay.rect(ox, oy, sw, sh, color).
  ). ).
).
