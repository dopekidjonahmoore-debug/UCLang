# ── UCLang 3D Collect ──────────────────────────────────────
#  WASD to move  •  Collect floating orbs  •  Third-person
# ──────────────────────────────────────────────────────────

Window.open_3d("UCLang 3D Collect", 1024, 768).

Mesh.load_primitive("player", "cube").
Mesh.load_primitive("coin",   "sphere").
Mesh.load_primitive("ground", "plane").

Light.ambient(0.25, 0.3, 0.4, 0.2).
Light.directional(-3, -5, -4, 0.9, 0.9, 1.0, 0.8).

Camera3D.set_fov(55).

px == 0. pz == 0.
score == 0.

coins == [].
i == 0.
While(i < 10 then(
  coins == list_push(coins, Math.random(-7, 7)).
  coins == list_push(coins, 1.0).
  coins == list_push(coins, Math.random(-7, 7)).
  i == i + 1.
). ).

GameLoop(
  update(
    dt == time.delta().
    spd == 5.0 * dt.
    dx == 0. dz == 0.
    If(input.key("w") then(dz == -spd.). ).
    If(input.key("s") then(dz == spd.). ).
    If(input.key("a") then(dx == -spd.). ).
    If(input.key("d") then(dx == spd.). ).
    px == px + dx. pz == pz + dz.
    If(px < -9 then(px == -9.). ). If(px > 9 then(px == 9.). ).
    If(pz < -9 then(pz == -9.). ). If(pz > 9 then(pz == 9.). ).
    Camera3D.set_pos(px, 5, pz + 7).
    Camera3D.set_target(px, 0, pz).
    i == 0.
    While(i < list_len(coins) then(
      cx == list_get(coins, i).
      cy == list_get(coins, i + 1).
      cz == list_get(coins, i + 2).
      ddx == px - cx. ddz == pz - cz.
      dist == Math.sqrt(ddx * ddx + ddz * ddz).
      If(dist < 1.2 then(
        coins == list_set(coins, i,   Math.random(-7, 7)).
        coins == list_set(coins, i + 1, 1.0).
        coins == list_set(coins, i + 2, Math.random(-7, 7)).
        score == score + 1.
        print("Score: " + int_to_str(score)).
      ). ).
      i == i + 3.
    ). ).
  ).
  draw(
    Draw3D.begin().
    Draw3D.mesh("ground", "default", 0, -0.5, 0, 0, 0, 0, 20, 1, 20).
    Draw3D.mesh("player", "default", px, 0.5, pz, 0, 0, 0, 0.8, 0.8, 0.8).
    i == 0.
    While(i < list_len(coins) then(
      cx == list_get(coins, i).
      cy == list_get(coins, i + 1).
      cz == list_get(coins, i + 2).
      bob == Math.sin(time.elapsed() * 2.5 + i) * 0.4.
      spin == time.elapsed() * 60.
      Draw3D.mesh("coin", "default", cx, cy + bob, cz, 0, spin, 0, 0.35, 0.35, 0.35).
      i == i + 3.
    ). ).
  ).
).
