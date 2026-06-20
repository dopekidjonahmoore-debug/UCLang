def mathf_clamp(val, min_val, max_val.
  If(val < min_val then(response(min_val).). ).
  If(val > max_val then(response(max_val).). ).
  response(val).
).

def mathf_clamp01(val.
  If(val < 0.0 then(response(0.0).). ).
  If(val > 1.0 then(response(1.0).). ).
  response(val).
).

def mathf_lerp(a, b, t.
  response(a + (b - a) * mathf_clamp01(t)).
).

def mathf_lerp_unclamped(a, b, t.
  response(a + (b - a) * t).
).

def mathf_inverse_lerp(a, b, val.
  If(a = b then(response(0.0).). ).
  response((val - a) / (b - a)).
).

def mathf_move_towards(current, target, max_delta.
  diff == target - current.
  If(Math.abs(diff) <= max_delta then(response(target).). ).
  response(current + Math.sign(diff) * max_delta).
).

def mathf_smooth_step(a, b, t.
  t == mathf_clamp01(t).
  t == t * t * (3.0 - 2.0 * t).
  response(a + (b - a) * t).
).

def mathf_ping_pong(t, length.
  t == t / length.
  t == t - Math.floor(t / 2.0) * 2.0.
  If(t > 1.0 then(response(2.0 - t).). ).
  response(t).
).

def mathf_remap(val, from_low, from_high, to_low, to_high.
  t == mathf_inverse_lerp(from_low, from_high, val).
  response(mathf_lerp(to_low, to_high, t)).
).

def mathf_sign(val.
  If(val > 0 then(response(1).). ).
  If(val < 0 then(response(-1).). ).
  response(0).
).

def mathf_distance(x1, y1, x2, y2.
  dx == x2 - x1.
  dy == y2 - y1.
  response(Math.sqrt(dx * dx + dy * dy)).
).

def mathf_angle(x1, y1, x2, y2.
  dx == x2 - x1.
  dy == y2 - y1.
  response(Math.atan2(dy, dx) * 57.2958).
).

def mathf_direction(from_x, from_y, to_x, to_y.
  dx == to_x - from_x.
  dy == to_y - from_y.
  dist == Math.sqrt(dx * dx + dy * dy).
  If(dist = 0 then(response([0.0, 0.0]).). ).
  response([dx / dist, dy / dist]).
).
