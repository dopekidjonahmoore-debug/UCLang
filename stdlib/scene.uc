scenes == [].
active_scene == "".

def scene_register(name, on_init, on_update, on_draw.
  scenes == list_push(scenes, [name, on_init, on_update, on_draw]).
).

def scene_go(name.
  active_scene == name.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = name then(
      run.call_func(list_get(s, 1)).
    ). ).
    i == i + 1.
  ). ).
).

def scene_update(.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = active_scene then(
      run.call_func(list_get(s, 2)).
    ). ).
    i == i + 1.
  ). ).
).

def scene_draw(.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = active_scene then(
      run.call_func(list_get(s, 3)).
    ). ).
    i == i + 1.
  ). ).
).
