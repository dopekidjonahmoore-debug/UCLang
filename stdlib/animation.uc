tween_create(startVal, endVal, dur, easeType.
  response([startVal, endVal, dur, easeType, 0.0, startVal]).
).

tween_update(t, dt.
  elapsed == list_get(t, 4) + dt.
  t == list_set(t, 4, elapsed).
  dur == list_get(t, 2).
  If(elapsed >= dur then(
    elapsed == dur.
    t == list_set(t, 4, dur).
  ). ).
  progress == elapsed / dur.
  val == list_get(t, 0) + (list_get(t, 1) - list_get(t, 0)) * ease_apply(list_get(t, 3), progress).
  t == list_set(t, 5, val).
  response(t).
).

tween_value(t.
  response(list_get(t, 5)).
).

tween_done(t.
  response(list_get(t, 4) >= list_get(t, 2)).
).

tween_reset(t.
  t == list_set(t, 4, 0.0).
  t == list_set(t, 5, list_get(t, 0)).
  response(t).
).

ease_apply(type, t.
  If(type = "linear" then(
    response(ease_linear(t)).
  ). ).
  If(type = "quad_in" then(
    response(ease_quad_in(t)).
  ). ).
  If(type = "quad_out" then(
    response(ease_quad_out(t)).
  ). ).
  If(type = "cubic_in" then(
    response(ease_cubic_in(t)).
  ). ).
  If(type = "cubic_out" then(
    response(ease_cubic_out(t)).
  ). ).
  If(type = "sine_in" then(
    response(ease_sine_in(t)).
  ). ).
  If(type = "sine_out" then(
    response(ease_sine_out(t)).
  ). ).
  If(type = "elastic_out" then(
    response(ease_elastic_out(t)).
  ). ).
  If(type = "bounce_out" then(
    response(ease_bounce_out(t)).
  ). ).
  response(t).
).

ease_linear(t.
  response(t).
).

ease_quad_in(t.
  response(t * t).
).

ease_quad_out(t.
  response(t * (2 - t)).
).

ease_cubic_in(t.
  response(t * t * t).
).

ease_cubic_out(t.
  t == t - 1.
  response(t * t * t + 1).
).

ease_sine_in(t.
  response(1 - Math.cos(t * 3.14159 / 2)).
).

ease_sine_out(t.
  response(Math.sin(t * 3.14159 / 2)).
).

ease_elastic_out(t.
  If(t = 0 or t = 1 then(
    response(t).
  ). ).
  response(Math.pow(2, -10 * t) * Math.sin((t * 10 - 0.75) * 6.283185) + 1).
).

ease_bounce_out(t.
  If(t < 1 / 2.75 then(
    response(7.5625 * t * t).
  ). ).
  If(t < 2 / 2.75 then(
    t == t - 1.5 / 2.75.
    response(7.5625 * t * t + 0.75).
  ). ).
  If(t < 2.5 / 2.75 then(
    t == t - 2.25 / 2.75.
    response(7.5625 * t * t + 0.9375).
  ). ).
  t == t - 2.625 / 2.75.
  response(7.5625 * t * t + 0.984375).
).
