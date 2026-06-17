# ── UCLang 3D Collect ──────────────────────────────────────
#  WASD  •  Mouse orbit  •  Collect 10 floating orbs
# ──────────────────────────────────────────────────────────

Window.open_3d("UCLang 3D Collect", 1024, 768).

Overlay.font("assets/font.ttf", 24).
input.mouse_lock(true).

Mesh.load_primitive("player", "cube").
Mesh.load_primitive("coin",   "sphere").
Mesh.load_primitive("beam",   "cylinder").
Mesh.load_primitive("ground", "plane").
Mesh.load_primitive("wall",   "cube").
Mesh.load_primitive("pillar", "cube").
Mesh.load_primitive("tile",   "cube").

Light.ambient(0.20, 0.25, 0.35, 0.30).
Light.directional(-3, -6, -4, 0.8, 0.85, 1.0, 0.75).

Camera3D.set_fov(55).

px == 0. pz == 0.
camAngle == 0.  camHeight == 6.  camDist == 10.
score == 0.  total == 10.  pRotY == 0.

coins == [].
i == 0.
While(i < 10 then(
  coins == list_push(coins, Math.random(-6, 6)).
  coins == list_push(coins, 0.5 + Math.random(0, 2)).
  coins == list_push(coins, Math.random(-6, 6)).
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

    If(dx * dx + dz * dz > 0.0001 then(
      pRotY == Math.atan2(dx, dz).
    ). ).

    Camera3D.set_pos(px + cx * camDist, camHeight, pz + sn * camDist).
    Camera3D.set_target(px, 0, pz).

    i == 0.
    While(i < list_len(coins) then(
      cx2 == list_get(coins, i).
      cy2 == list_get(coins, i + 1).
      cz2 == list_get(coins, i + 2).
      ddx == px - cx2. ddz == pz - cz2.
      If(Math.sqrt(ddx * ddx + ddz * ddz) < 1.2 then(
        coins == list_set(coins, i,   Math.random(-6, 6)).
        coins == list_set(coins, i + 1, 0.5 + Math.random(0, 2)).
        coins == list_set(coins, i + 2, Math.random(-6, 6)).
        score == score + 1.
      ). ).
      i == i + 3.
    ). ).
  ).
  draw(
    Draw3D.begin().

    Draw3D.mesh("ground", "default", 0, -0.5, 0, 0, 0, 0, 20, 1, 20).

    i == 0.
    While(i < 4 then(
      rad == 2.0 + i * 2.0.
      Draw3D.mesh("pillar", "default", 0, -0.2, 0, 90, 0, 0, rad * 2, 0.06, rad * 2).
      i == i + 1.
    ). ).

    Draw3D.mesh("wall", "default",  0, 2, -9,  0, 0, 0, 18, 4, 0.5).
    Draw3D.mesh("wall", "default",  0, 2,  9,  0, 0, 0, 18, 4, 0.5).
    Draw3D.mesh("wall", "default", -9, 2,  0,  0, 0, 0, 0.5, 4, 18).
    Draw3D.mesh("wall", "default",  9, 2,  0,  0, 0, 0, 0.5, 4, 18).

    Draw3D.mesh("pillar", "default", -7, 1.5, -7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default",  7, 1.5, -7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default", -7, 1.5,  7, 0, 0, 0, 0.6, 3, 0.6).
    Draw3D.mesh("pillar", "default",  7, 1.5,  7, 0, 0, 0, 0.6, 3, 0.6).

    Draw3D.mesh("player", "default", px, 0.5, pz, 0, pRotY, 0, 0.7, 0.7, 0.7).

    i == 0.
    While(i < list_len(coins) then(
      cx2 == list_get(coins, i).
      cy2 == list_get(coins, i + 1).
      cz2 == list_get(coins, i + 2).
      bob == Math.sin(time.elapsed() * 2.5 + i) * 0.3.
      spin == time.elapsed() * (40 + i * 10).
      sz2 == 0.3 + Math.sin(i) * 0.1.
      Draw3D.mesh("beam", "default", cx2, 0.05, cz2, 0, 0, 0, 0.06, cy2 + bob, 0.06).
      Draw3D.mesh("coin", "default", cx2, cy2 + bob, cz2, 0, spin, 0, sz2, sz2, sz2).
      i == i + 3.
    ). ).

    Overlay.begin().
    Overlay.rect(16, 14, 220, 56, "#0a0e24cc").
    Overlay.rect(16, 68, 220, 2, "#3b82f6ff").
    Overlay.text(24, 18, "COLLECT", "#64748b", 11).
    Overlay.text(24, 32, int_to_str(score), "#ffffff", 38).
    Overlay.text(24, 54, int_to_str(total) + " remaining", "#22d3ee", 12).

    Overlay.rect(788, 14, 220, 56, "#0a0e24cc").
    Overlay.rect(788, 68, 220, 2, "#3b82f6ff").
    Overlay.text(796, 18, "ORBIT", "#64748b", 11).
    Overlay.text(796, 32, "W A S D", "#c4b5fd", 26).
    Overlay.text(796, 54, "Mouse  |  ESC to quit", "#64748b", 11).

    Overlay.rect(256, 720, 512, 36, "#0a0e24cc").
    Overlay.rect(256, 714, 512, 2, "#3b82f6ff").
    pct == score * 10.
    barW == pct * 4.8.
    Overlay.rect(264, 724, barW, 24, "#22d3eecc").
    Overlay.text(512, 726, int_to_str(score) + " / " + int_to_str(total), "#ffffff", 16).
  ).
).
