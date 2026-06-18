Window.open_3d("Simple 3D", 1024, 768).
Overlay.font("assets/font.ttf", 24).

Mesh.load_primitive("cube", "cube").
Camera3D.set_fov(60).

rot == 0.0.

GameLoop(
  update(
    dt == time.delta().
    rot == rot + 30.0 * dt.
    Camera3D.set_pos(3.0, 2.0, 3.0).
    Camera3D.set_target(0.0, 0.0, 0.0).
  ).
  draw(
    Draw3D.begin().
    Draw3D.mesh("cube", "default", 0.0, 0.0, 0.0, rot, rot * 0.5, 0.0, 1.0, 1.0, 1.0).

    Overlay.begin().
    Overlay.rect(50, 50, 300, 40, "#ff000088").
    Overlay.text(60, 58, "HELLO WORLD", "#ffffff", 20).
    Overlay.rect(50, 100, 300, 40, "#00ff0088").
    Overlay.text(60, 108, "Score: 42", "#ffff00", 20).
    Overlay.circle(400, 60, 20, "#ff880088").
    Overlay.text(430, 52, "Circle test", "#ffffff", 16).
  ).
).
