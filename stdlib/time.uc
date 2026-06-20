time_scale == 1.0.

def time_delta(.
  response(time.delta() * time_scale).
).

def time_unscaled_delta(.
  response(time.delta()).
).

def time_elapsed(.
  response(time.elapsed()).
).

def time_fps(.
  response(time.fps()).
).

def time_set_scale(scale.
  If(scale < 0 then(scale == 0. ). ).
  time_scale == scale.
).

def time_get_scale(.
  response(time_scale).
).
