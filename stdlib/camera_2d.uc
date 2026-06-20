cam2d_x == 0.0.
cam2d_y == 0.0.
cam2d_zoom == 1.0.
cam2d_rotation == 0.0.
cam2d_follow_eid == -1.
cam2d_follow_offset_x == 0.0.
cam2d_follow_offset_y == 0.0.
cam2d_shake_magnitude == 0.0.
cam2d_shake_duration == 0.0.
cam2d_shake_elapsed == 0.0.

def cam2d_set_pos(x, y.
  cam2d_x == x.
  cam2d_y == y.
).

def cam2d_get_pos(.
  response([cam2d_x, cam2d_y]).
).

def cam2d_set_zoom(zoom.
  If(zoom < 0.01 then(zoom == 0.01. ). ).
  cam2d_zoom == zoom.
).

def cam2d_get_zoom(.
  response(cam2d_zoom).
).

def cam2d_follow(eid, offset_x, offset_y.
  cam2d_follow_eid == eid.
  cam2d_follow_offset_x == offset_x.
  cam2d_follow_offset_y == offset_y.
).

def cam2d_stop_follow(.
  cam2d_follow_eid == -1.
).

def cam2d_shake(magnitude, duration.
  cam2d_shake_magnitude == magnitude.
  cam2d_shake_duration == duration.
  cam2d_shake_elapsed == 0.0.
).

def cam2d_update(dt.
  If(cam2d_follow_eid >= 0 then(
    If(entity_exists(cam2d_follow_eid) then(
      pos == entity_get(cam2d_follow_eid, "position").
      cam2d_x == list_get(pos, 0) + cam2d_follow_offset_x.
      cam2d_y == list_get(pos, 1) + cam2d_follow_offset_y.
    ). ).
  ). ).
  If(cam2d_shake_duration > 0 then(
    cam2d_shake_elapsed == cam2d_shake_elapsed + dt.
    If(cam2d_shake_elapsed >= cam2d_shake_duration then(
      cam2d_shake_magnitude == 0.0.
      cam2d_shake_duration == 0.0.
    ). else(
      progress == cam2d_shake_elapsed / cam2d_shake_duration.
      decay == 1.0 - progress.
      sx == (Math.random() - 0.5) * 2.0 * cam2d_shake_magnitude * decay.
      sy == (Math.random() - 0.5) * 2.0 * cam2d_shake_magnitude * decay.
      cam2d_x == cam2d_x + sx.
      cam2d_y == cam2d_y + sy.
    ). ).
  ). ).
).

def cam2d_world_to_screen(wx, wy, sw, sh.
  sx == (wx - cam2d_x) * cam2d_zoom + sw / 2.
  sy == (wy - cam2d_y) * cam2d_zoom + sh / 2.
  response([sx, sy]).
).

def cam2d_screen_to_world(sx, sy, sw, sh.
  wx == (sx - sw / 2) / cam2d_zoom + cam2d_x.
  wy == (sy - sh / 2) / cam2d_zoom + cam2d_y.
  response([wx, wy]).
).
