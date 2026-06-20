run_file("stdlib/engine.uc").

Window.open_3d("Game Engine Demo", 1024, 768).

Camera3D.set_pos(0.0, 5.0, 8.0).
Camera3D.set_target(0.0, 0.0, 0.0).

light.directional(0.5, -1.0, 0.3, 1.0, 1.0, 1.0, 1.0).
light.ambient(0.3, 0.3, 0.4, 0.5).

Mesh.load_primitive("player", "cube").
Mesh.load_primitive("coin", "sphere").
Mesh.load_primitive("floor", "plane").

player_eid == entity_create().
entity_set(player_eid, "position", 0.0, 1.0, 0.0).
entity_set(player_eid, "velocity", 0.0, 0.0, 0.0).
entity_set(player_eid, "gravity", -9.8).
entity_set(player_eid, "ground_y", 0.0).

coin_eid == entity_create().
entity_set(coin_eid, "position", 3.0, 1.5, -3.0).
entity_set(coin_eid, "velocity", 0.0, 0.0, 0.0).

def game_init(.
  Overlay.font("assets/font.ttf", 24).
).

def game_update(.
  dt == time_delta().

  physics2d_update(dt).

  pos == entity_get(player_eid, "position").
  px == list_get(pos, 0).
  py == list_get(pos, 1).
  pz == list_get(pos, 2).

  speed == 4.0.
  If(input_get_key("w") then(pz == pz - speed * dt. ). ).
  If(input_get_key("s") then(pz == pz + speed * dt. ). ).
  If(input_get_key("a") then(px == px - speed * dt. ). ).
  If(input_get_key("d") then(px == px + speed * dt. ). ).

  If(input_get_key_down("space") and py = 0.0 then(
    vel == entity_get(player_eid, "velocity").
    vel == list_set(vel, 1, 6.0).
    entity_set(player_eid, "velocity", list_get(vel, 0), list_get(vel, 1), list_get(vel, 2)).
  ). ).

  entity_set(player_eid, "position", px, py, pz).

  cp == entity_get(coin_eid, "position").
  cpx == list_get(cp, 0).
  cpy == list_get(cp, 1).
  cpz == list_get(cp, 2).
  dx == cpx - px.
  dz == cpz - pz.
  dist == Math.sqrt(dx * dx + dz * dz).
  If(dist < 1.5 then(
    entity_set(coin_eid, "position", Math.random() * 6.0 - 3.0, 1.5, Math.random() * 6.0 - 3.0).
  ). ).
).

def game_draw(.
  Draw3D.begin().

  Draw3D.mesh("floor", "default", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 1.0, 10.0).

  pp == entity_get(player_eid, "position").
  Draw3D.mesh("player", "default", list_get(pp, 0), list_get(pp, 1), list_get(pp, 2), 0.0, 0.0, 0.0, 0.5, 0.5, 0.5).

  cp == entity_get(coin_eid, "position").
  angle == time_elapsed() * 90.0.
  Draw3D.mesh("coin", "default", list_get(cp, 0), list_get(cp, 1), list_get(cp, 2), 0.0, angle, 0.0, 0.4, 0.4, 0.4).

  Overlay.begin().
  Overlay.text(10, 10, "Use WASD to move, SPACE to jump", "#ffffff", 24).
  Overlay.text(10, 40, "Collect the spinning coin!", "#ffffff", 24).
  Overlay.circle(500, 300, 50, "#ffaa0066").
).

scene_register("game", "game_init", "game_update", "game_draw").
scene_go("game").

GameLoop(
  update(.
    scene_update().
  ).
  draw(.
    scene_draw().
  ).
).
