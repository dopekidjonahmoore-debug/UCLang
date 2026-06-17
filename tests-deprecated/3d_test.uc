Window.open_3d("3D Test", 800, 600).
Mesh.load_primitive("cube", "cube").
Texture.load("white", "assets/white.png").
camera3d.set_pos(0, 2, 5).
camera3d.set_target(0, 0, 0).
lighting.ambient(0.2, 0.2, 0.2).
lighting.directional(1, 1, -1, 1, 1, 1, 0.8).
angle == 0.
GameLoop(
  update(
    angle == angle + 50 * time.delta().
  ).
  draw(
    Draw3D.begin(camera3d.get_view(), camera3d.get_proj()).
    Draw3D.mesh("cube", "default", 0, 0, 0, angle, angle * 0.5, 0, 1, 1, 1).
    Draw3D.end().
  ).
).
