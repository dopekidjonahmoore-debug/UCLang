# ── UCLang Engine  —  3D Demo  ──────────────────────────────────
#  OpenGL 3.3  •  Phong lighting  •  Rotating primitives
# ────────────────────────────────────────────────────────────────

Window.open_3d("UCLang 3D Demo", 1024, 768).

Mesh.load_primitive("cube",   "cube").
Mesh.load_primitive("sphere", "sphere").
Mesh.load_primitive("plane",  "plane").

Light.ambient(0.25, 0.3, 0.4, 0.2).
Light.directional(-2, -3, -4,  1, 0.95, 0.9, 0.7).

Camera3D.set_pos(0, 2.5, 6).
Camera3D.set_target(0, 0, 0).
Camera3D.set_fov(50).

angle == 0.

GameLoop(
  update(
    dt == time.delta().
    angle == angle + 30 * dt.
    If(angle >= 360 then(angle == angle - 360.). ).
  ).

  draw(
    Draw3D.begin().

    # Rotating cube at center
    Draw3D.mesh("cube", "default",
      0, 0.8, 0,
      angle, angle * 0.7, 0,
      1, 1, 1).

    # Sphere orbiting
    ox == Math.sin(angle * 0.03) * 2.5.
    oz == Math.cos(angle * 0.03) * 2.5.
    Draw3D.mesh("sphere", "default",
      ox, 0.5, oz,
      0, angle * 2, 0,
      0.6, 0.6, 0.6).

    # Ground plane
    Draw3D.mesh("plane", "default",
      0, -0.5, 0,
      0, 0, 0,
      5, 5, 5).
  ).
).
