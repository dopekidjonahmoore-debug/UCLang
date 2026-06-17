# ── UCLang 3D Collect ──────────────────────────────────────
#  WASD  •  Mouse orbit  •  Collect 10 floating orbs
# ──────────────────────────────────────────────────────────

Window.open_3d("UCLang 3D Collect", 1024, 768).
Overlay.font("assets/font.ttf", 24).

Mesh.load_primitive("player", "cube").
Mesh.load_primitive("coin",   "sphere").
Mesh.load_primitive("ground", "plane").
Mesh.load_primitive("wall",   "cube").
Mesh.load_primitive("pillar", "cube").

Light.ambient(0.25, 0.3, 0.45, 0.25).
Light.directional(-3, -5, -4, 0.9, 0.9, 1.0, 0.8).

Camera3D.set_fov(55).

px == 0. pz == 0.
camAngle == 0.  camHeight == 5.  camDist == 9.
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

    camAngle == camAngle + input.mouse_dx() * 0.002.
    camHeight == camHeight - input.mouse_dy() * 0.004.
    If(camHeight < 2  then(camHeight == 2.). ).
    If(camHeight > 12 then(camHeight == 12.). ).

    cx == Math.cos(camAngle). sn == Math.sin(camAngle).

    dx == 0. dz == 0.
    If(input.key("w") then(dx == 0 - cx * spd. dz == 0 - sn * spd.). ).
    If(input.key("s") then(dx == cx * spd. dz == sn * spd.). ).
    If(input.key("a") then(dx == sn * spd. dz == 0 - cx * spd.). ).
    If(input.key("d") then(dx == 0 - sn * spd. dz == cx * spd.). ).

    px == px + dx. pz == pz + dz.
    If(px < -8.5 then(px == -8.5.). ).
    If(px > 8.5 then(px == 8.5.). ).
    If(pz < -8.5 then(pz == -8.5.). ).
    If(pz > 8.5 then(pz == 8.5.). ).

    Camera3D.set_pos(px + cx * camDist, camHeight, pz + sn * camDist).
    Camera3D.set_target(px, 0, pz).

    i == 0.
    While(i < list_len(coins) then(
      cx2 == list_get(coins, i).
      cy2 == list_get(coins, i + 1).
      cz2 == list_get(coins, i + 2).
      ddx == px - cx2. ddz == pz - cz2.
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

    Draw3D.mesh("wall", "default",  0, 2, -9,  0, 0, 0, 18, 4, 0.5).
    Draw3D.mesh("wall", "default",  0, 2,  9,  0, 0, 0, 18, 4, 0.5).
    Draw3D.mesh("wall", "default", -9, 2,  0,  0, 0, 0, 0.5, 4, 18).
    Draw3D.mesh("wall", "default",  9, 2,  0,  0, 0, 0, 0.5, 4, 18).

    Draw3D.mesh("pillar", "default", -7, 1.5, -7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default",  7, 1.5, -7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default", -7, 1.5,  7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default",  7, 1.5,  7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default", -3, 1, -3, 0, 0, 0, 0.4, 2, 0.4).
    Draw3D.mesh("pillar", "default",  3, 1,  3, 0, 0, 0, 0.4, 2, 0.4).

    Draw3D.mesh("player", "default", px, 0.5, pz, 0, 0, 0, 0.8, 0.8, 0.8).

    i == 0.
    While(i < list_len(coins) then(
      cx2 == list_get(coins, i). cy2 == list_get(coins, i + 1). cz2 == list_get(coins, i + 2).
      bob == Math.sin(time.elapsed() * 2.5 + i) * 0.3.
      spin == time.elapsed() * (40 + i * 10).
      sz2 == 0.3 + Math.sin(i) * 0.1.
      Draw3D.mesh("coin", "default", cx2, cy2 + bob, cz2, 0, spin, 0, sz2, sz2, sz2).
      i == i + 3.
    ). ).

    Overlay.begin().
    Overlay.rect(0, 0, 1024, 86, "#00000099").
    Overlay.rect(0, 86, 1024, 2, "#e94560ff").
    Overlay.text(20, 14, "SCORE", "#888899", 14).
    Overlay.text(20, 34, int_to_str(score), "#ffffff", 32).
    Overlay.text(20, 56, int_to_str(total) + " remaining", "#88ff88", 16).
    Overlay.text(700, 740, "WASD move  |  Mouse orbit", "#666680", 15).
    Overlay.text(20, 740, "Score: " + int_to_str(score), "#ffffff", 15).
  ).
).
