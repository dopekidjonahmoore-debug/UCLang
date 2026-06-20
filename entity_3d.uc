# Entity-based 3D movement + physics demo
# Components: "transform" → [x,y,z, rx,ry,rz], "physics" → [mass, vx, vy, vz]

# ─── Player entity ───
player == entity_create().
entity_set(player, "transform", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0).
entity_set(player, "physics", 1.0, 0.0, 0.0, 0.0).

# ─── Coin entity ───
coin == entity_create().
entity_set(coin, "transform", 3.0, 1.5, -3.0, 0.0, 0.0, 0.0).

# ─── Window (init OpenGL first) ───
Window.open_3d("Entity 3D Demo", 800, 600).

# ─── Build scene meshes ───
Mesh.load_primitive("floor", "plane").
Mesh.load_primitive("player_mesh", "cube").
Mesh.load_primitive("coin_mesh", "sphere").

# ─── Camera ───
Camera3D.set_pos(0.0, 3.0, 6.0).
Camera3D.set_target(0.0, 0.0, 0.0).

# ─── Light ───
Light.ambient(0.2, 0.2, 0.3, 0.25).
Light.directional(-3.0, -6.0, -4.0, 0.8, 0.85, 1.0, 0.45).

# ─── Game loop ───
GameLoop(
  update(
    dt == time.delta().

    # ─── Physics step: apply gravity, integrate velocity ───
    i == 0.
    While(i < entity_count() then(
      eid == entity_all()[i].
      If(entity_has(eid, "physics") then(
        p == entity_get(eid, "physics").
        mass == p[0].
        vx == p[1].
        vy == p[2].
        vz == p[3].
        vy == vy - 9.8 * dt.
        entity_set(eid, "physics", mass, vx, vy, vz).
      ). ).

      If(entity_has(eid, "transform") then(
        t == entity_get(eid, "transform").
        x == t[0].
        y == t[1].
        z == t[2].
        rx == t[3].
        ry == t[4].
        rz == t[5].

        If(entity_has(eid, "physics") then(
          p == entity_get(eid, "physics").
          x == x + p[1] * dt.
          y == y + p[2] * dt.
          z == z + p[3] * dt.
          If(y < 0.0 then(
            y == 0.0.
            p2 == entity_get(eid, "physics").
            entity_set(eid, "physics", p2[0], p2[1] * 0.5, 0.0, p2[3] * 0.5).
          ). ).
        ). ).

        entity_set(eid, "transform", x, y, z, rx, ry, rz).
      ). ).
      i == i + 1.
    ). ).

    # ─── Player movement (WASD) ───
    speed == 5.0.
    tp == entity_get(player, "transform").
    px == tp[0].
    py == tp[1].
    pz == tp[2].

    If(input.key_down("w") then(pz == pz - speed * dt.). ).
    If(input.key_down("s") then(pz == pz + speed * dt.). ).
    If(input.key_down("a") then(px == px - speed * dt.). ).
    If(input.key_down("d") then(px == px + speed * dt.). ).

    # ─── Jump ───
    pp == entity_get(player, "physics").
    If(input.key_just_pressed("space") and py < 0.1 then(
      entity_set(player, "physics", pp[0], pp[1], 5.0, pp[3]).
    ). ).

    entity_set(player, "transform", px, py, pz, tp[3], tp[4], tp[5]).

    # ─── Exit ───
    If(input.key_just_pressed("escape") then(input.mouse_lock(false.). ). ).
  ).

  draw(
    Draw3D.begin().

    # ─── Draw floor ───
    Draw3D.mesh("floor", "default", 0.0, -0.5, 0.0, 0.0, 0.0, 0.0, 20.0, 1.0, 20.0).

    # ─── Draw player ───
    tp2 == entity_get(player, "transform").
    Draw3D.mesh("player_mesh", "default", tp2[0], tp2[1], tp2[2], tp2[3], tp2[4], tp2[5], 0.5, 1.0, 0.5).

    # ─── Draw coin ───
    If(entity_exists(coin) then(
      tc == entity_get(coin, "transform").
      Draw3D.mesh("coin_mesh", "default", tc[0], tc[1], tc[2], tc[3], tc[4], tc[5], 0.6, 0.6, 0.6).
    ). ).

    # ─── HUD ───
    Overlay.font("assets/font.ttf", 24).
    Overlay.begin().
    x_str == int_to_str(tp2[0]).
    y_str == int_to_str(tp2[1]).
    z_str == int_to_str(tp2[2]).
    Overlay.text(10, 10, "Pos: " + x_str + ", " + y_str + ", " + z_str, "#ffffff", 18).

    Draw3D.end().
  ).
).
