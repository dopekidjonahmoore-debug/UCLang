# ── UCLang 3D Collect ──────────────────────────────────────
#  WASD to move  •  Mouse to orbit  •  Collect 10 orbs
# ──────────────────────────────────────────────────────────

Window.open_3d("UCLang 3D Collect", 1024, 768).
Overlay.font("assets/font.ttf", 28).

Mesh.load_primitive("player", "cube").
Mesh.load_primitive("coin", "sphere").
Mesh.load_primitive("ground", "plane").
Mesh.load_primitive("wall", "cube").
Mesh.load_primitive("pillar", "cube").

Light.ambient(0.25, 0.3, 0.45, 0.25).
Light.directional(-3, -5, -4, 0.9, 0.9, 1.0, 0.8).

Camera3D.set_fov(55).

px == 0. pz == 0.
camAngle == 0.  camHeight == 4.  camDist == 8.
score == 0.  total == 10.

coins == [].
i == 0.
While(i < 10 then(
  coins == list_push(coins, Math.random(-7, 7)).
  coins == list_push(coins, 0.5 + Math.random(0, 2)).
  coins == list_push(coins, Math.random(-7, 7)).
  i == i + 1.
). ).

GameLoop(
  update(
    dt == time.delta().
    spd == 5.0 * dt.

    camAngle == camAngle + input.mouse_dx() * 0.003.
    camHeight == camHeight - input.mouse_dy() * 0.005.
    If(camHeight < 2   then(camHeight == 2.). ).
    If(camHeight > 10  then(camHeight == 10.). ).

    cx == Math.cos(camAngle). sz == Math.sin(camAngle).
    fx == 0 - sz. fz == cx.

    dx == 0. dz == 0.
    If(input.key("w") then(dx == fx * spd. dz == fz * spd.). ).
    If(input.key("s") then(dx == -fx * spd. dz == -fz * spd.). ).
    rcx == Math.cos(camAngle + 1.57).
    rsz == Math.sin(camAngle + 1.57).
    If(input.key("a") then(dx == rcx * spd. dz == rsz * spd.). ).
    If(input.key("d") then(dx == -rcx * spd. dz == -rsz * spd.). ).

    px == px + dx. pz == pz + dz.
    If(px < -8.5 then(px == -8.5.). ). If(px > 8.5 then(px == 8.5.). ).
    If(pz < -8.5 then(pz == -8.5.). ). If(pz > 8.5 then(pz == 8.5.). ).

    Camera3D.set_pos(px + cx * camDist, camHeight, pz + sz * camDist).
    Camera3D.set_target(px, 0, pz).

    i == 0.
    While(i < list_len(coins) then(
      cx2 == list_get(coins, i). cy == list_get(coins, i + 1). cz == list_get(coins, i + 2).
      ddx == px - cx2. ddz == pz - cz.
      If(Math.sqrt(ddx * ddx + ddz * ddz) < 1.2 then(
        coins == list_set(coins, i,   Math.random(-7, 7)).
        coins == list_set(coins, i + 1, 0.5 + Math.random(0, 2)).
        coins == list_set(coins, i + 2, Math.random(-7, 7)).
        score == score + 1.
      ). ).
      i == i + 3.
    ). ).
  ).
  draw(
    Draw3D.begin().
    Draw3D.mesh("ground", "default", 0, -0.5, 0, 0, 0, 0, 20, 1, 20).
    Draw3D.mesh("player", "default", px, 0.5, pz, 0, camAngle * 57.3, 0, 0.8, 0.8, 0.8).
    i == 0.
    While(i < list_len(coins) then(
      cx == list_get(coins, i). cy == list_get(coins, i + 1). cz == list_get(coins, i + 2).
      bob == Math.sin(time.elapsed() * 2.5 + i) * 0.3.
      spin == time.elapsed() * (40 + i * 10).
      sz2 == 0.3 + Math.sin(i) * 0.1.
      Draw3D.mesh("coin", "default", cx, cy + bob, cz, 0, spin, 0, sz2, sz2, sz2).
      i == i + 3.
    ). ).

    Overlay.begin().
    s == int_to_str(score). t == int_to_str(total).
    Overlay.rect(0, 0, 1024, 86, "#00000077").
    Overlay.text(20, 20, s, "#ffffff", 28).
    Overlay.text(20, 56, t, "#88ff88", 16).
    Overlay.text(700, 740, "WASD: move  Mouse: orbit", "#888899", 14).
  ).
).
