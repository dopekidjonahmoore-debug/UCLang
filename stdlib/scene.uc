scenes == [].
activeScene == "".

scene_register(name, onInit, onUpdate, onDraw.
  scenes == list_push(scenes, [name, onInit, onUpdate, onDraw]).
).

scene_go(name.
  activeScene == name.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = name then(
      run.call_func(list_get(s, 1)).
    ). ).
    i == i + 1.
  ). ).
).

scene_update(.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = activeScene then(
      run.call_func(list_get(s, 2)).
    ). ).
    i == i + 1.
  ). ).
).

scene_draw(.
  i == 0.
  While(i < list_len(scenes) then(
    s == list_get(scenes, i).
    If(list_get(s, 0) = activeScene then(
      run.call_func(list_get(s, 3)).
    ). ).
    i == i + 1.
  ). ).
).
