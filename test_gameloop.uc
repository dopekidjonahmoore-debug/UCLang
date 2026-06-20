run_file("stdlib/engine.uc").
Window.open_3d("test", 1024, 768).
count == 0.
GameLoop(
  update(.
    count == count + 1.
    If(count > 3 then(Window.close(). ). ).
  ).
  draw(.
    Overlay.begin().
    Overlay.text(10, 10, "hello", "#ffffff", 20).
    Overlay.end().
    Draw3D.begin().
  ).
).
