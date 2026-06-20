def input_get_key(name.
  response(input.key_down(name)).
).

def input_get_key_down(name.
  response(input.key_just_pressed(name)).
).

def input_get_key_up(name.
  response(input.key_just_released(name)).
).

def input_get_mouse_button(button.
  response(input.mouse(button)).
).

def input_get_mouse_button_down(button.
  response(input.mouse_just_pressed(button)).
).

def input_get_mouse_button_up(button.
  response(input.mouse_just_released(button)).
).

def input_get_axis(axis.
  If(axis = "horizontal" then(
    right == input.key_down("d") or input.key_down("right").
    left == input.key_down("a") or input.key_down("left").
    If(right and left then(response(0.0).). ).
    If(right then(response(1.0).). ).
    If(left then(response(-1.0).). ).
    response(0.0).
  ). ).
  If(axis = "vertical" then(
    up == input.key_down("w") or input.key_down("up").
    down == input.key_down("s") or input.key_down("down").
    If(up and down then(response(0.0).). ).
    If(up then(response(-1.0).). ).
    If(down then(response(1.0).). ).
    response(0.0).
  ). ).
  response(0.0).
).

def input_get_mouse_x(.
  response(input.mouse_x()).
).

def input_get_mouse_y(.
  response(input.mouse_y()).
).

def input_get_mouse_delta(.
  response([input.mouse_dx(), input.mouse_dy()]).
).

def input_lock_mouse(locked.
  input.mouse_lock(locked).
).
