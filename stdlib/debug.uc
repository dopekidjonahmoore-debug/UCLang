debug_log_enabled == 1.

def debug_log(message.
  If(debug_log_enabled then(
    print(message).
  ). ).
).

def debug_log_value(name, val.
  If(debug_log_enabled then(
    print(name + ": " + val).
  ). ).
).

def debug_draw_circle(cx, cy, radius, color.
  Overlay.circle(cx, cy, radius, color).
).

def debug_draw_rect(x, y, w, h, color.
  Overlay.rect(x, y, w, h, color).
).

def debug_draw_text(x, y, text.
  Overlay.text(x, y, text, "#ffff00", 12).
).

def debug_draw_fps(.
  fps == time.fps().
  Overlay.text(10, 10, "FPS: " + int_to_str(fps), "#00ff00", 14).
).

def debug_draw_entity_count(.
  Overlay.text(10, 28, "Entities: " + int_to_str(entity_count()), "#00ff00", 14).
).

def debug_draw_all(.
  debug_draw_fps().
  debug_draw_entity_count().
).
