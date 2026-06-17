Window.open_3d("Simple 3D", 1024, 768).

Overlay.font("assets/font.ttf", 24).

Mesh.load_primitive("cube", "cube").

Camera3D.set_fov(60).

GameLoop(
  update(
    dt == time.delta().
    Camera3D.set_pos(3.0, 2.0, 3.0).
    Camera3D.set_target(0.0, 0.0, 0.0).
  ).
  draw(
    Draw3D.begin().
    Overlay.begin().
    Overlay.rect(0, 0, 200, 30, "#ff0000ff").
    Overlay.rect(0, 30, 200, 30, "#00ff00ff").
    Overlay.rect(0, 60, 200, 30, "#0000ffff").
    Overlay.debug().
  ).
).
