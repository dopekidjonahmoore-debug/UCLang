run_file("stdlib/engine.uc").
print("A: engine loaded").

Window.open_3d("UCLang Engine", 1024, 768).
Camera3D.set_pos(0.0, 5.0, 8.0).
Camera3D.set_target(0.0, 0.0, 0.0).
light.directional(0.5, -1.0, 0.3, 1.0, 1.0, 1.0, 1.0).
light.ambient(0.3, 0.3, 0.4, 0.5).
print("B: 3d setup").

physics2d_set_gravity(-12.0).
time_set_scale(1.0).
cam2d_set_zoom(1.0).
cam2d_set_pos(0.0, 0.0).
print("C: physics").

main_canvas == ui_canvas_create().
game_canvas == ui_canvas_create().
ui_canvas_set_active(game_canvas, 0).
print("D: canvases").

game_state == "menu".
score == 0.
high_score == 0.
Mesh.load_primitive("floor", "plane").
Mesh.load_primitive("cube", "cube").
print("E: meshes").

player == gameobject_create("Player", 0.0, 3.0).
entity_set(player, "velocity", 0.0, 0.0, 0.0).
entity_set(player, "gravity", -12.0).
entity_set(player, "ground_y", -3.0).
entity_set(player, "sprite", 32.0, 32.0, "#44ddff").
print("F: player").

jump_count == 0.
max_jumps == 2.
coins == [].
coin_timer == 0.0.
print("G: vars").

def spawn_coin(.
  cx2 == Math.random() * 10.0 - 5.0.
  cy2 == Math.random() * 4.0 + 1.0.
  c2 == gameobject_create("Coin", cx2, cy2).
  entity_set(c2, "sprite", 20.0, 20.0, "#ffdd44").
  coins == list_push(coins, c2).
).

def reset_game(.
  score == 0.
  transform_set_pos(player, 0.0, 3.0).
  entity_set(player, "velocity", 0.0, 0.0, 0.0).
  jump_count == 0.
  coins == [].
  coin_timer == 0.0.
  spawn_coin().
  spawn_coin().
  spawn_coin().
).

print("H: defs done").

menu_title == ui_text(main_canvas, 320, 100, "UCLang Engine", 48, "#ffdd44").
print("I: menu_title").
menu_subtitle == ui_text(main_canvas, 340, 160, "Unity-inspired 2D + 3D", 20, "#aaaacc").
print("J: subtitle").
btn_play == ui_button(main_canvas, 312, 260, 200, 50, "Play", "on_play").
print("K: play btn").

def on_play(.
  game_state == "game".
  ui_canvas_set_active(main_canvas, 0).
  ui_canvas_set_active(game_canvas, 1).
  reset_game().
).

def on_3d(.
  game_state == "3d".
  ui_canvas_set_active(main_canvas, 0).
  ui_canvas_set_active(game_canvas, 1).
  reset_game().
).

def on_quit(.
  debug_log("Close window manually").
).

print("L: callbacks").

hud_score == ui_text(game_canvas, 20, 20, "Score: 0", 24, "#ffffff").
print("M: hud score").
hud_coins == ui_text(game_canvas, 20, 50, "Coins: 0", 18, "#ffdd44").
hud_mode == ui_text(game_canvas, 20, 720, "MODE: 2D", 14, "#44ff66").
hud_info == ui_text(game_canvas, 350, 720, "WASD Move | SPACE Jump x2 | ESC Menu", 14, "#888888").
print("N: hud done").

def scene_init(.
  Overlay.font("assets/font.ttf", 24).
  reset_game().
).

def scene_update(.
  dt3 == time_delta().
  If(game_state = "game" then(
    physics2d_update(dt3).
    px3 == transform_get_x(player).
    py3 == transform_get_y(player).
    h3 == input_get_axis("horizontal").
    If(h3 != 0 then(transform_translate(player, h3 * 5.0 * dt3, 0.0). ). ).
    grounded == physics2d_is_grounded(player).
    If(grounded then(jump_count == 0. ). ).
    If(input_get_key_down("space") and jump_count < max_jumps then(
      vel3 == entity_get(player, "velocity").
      vel3 == list_set(vel3, 1, 7.0).
      entity_set(player, "velocity", list_get(vel3, 0), list_get(vel3, 1), list_get(vel3, 2)).
      jump_count == jump_count + 1.
    ). ).
    If(px3 < -10.0 then(transform_set_pos(player, -10.0, py3). ). ).
    If(px3 > 10.0 then(transform_set_pos(player, 10.0, py3). ). ).
    cam2d_set_pos(px3, py3).
    ci3 == 0.
    While(ci3 < list_len(coins) then(
      cid3 == list_get(coins, ci3).
      If(entity_exists(cid3) then(
        cx3 == transform_get_x(cid3).
        cy3 == transform_get_y(cid3).
        dist3 == mathf_distance(px3, py3, cx3, cy3).
        If(dist3 < 24.0 then(
          entity_destroy(cid3).
          coins == list_remove(coins, ci3).
          score == score + 1.
          ui_set_text(hud_score, "Score: " + int_to_str(score)).
        ). else(ci3 == ci3 + 1. ). ).
      ). else(coins == list_remove(coins, ci3). ). ).
    ). ).
    coin_timer3 == coin_timer + dt3.
    If(coin_timer3 > 2.0 and list_len(coins) < 8 then(
      spawn_coin().
      coin_timer == 0.0.
    ). else(coin_timer == coin_timer3. ). ).
    If(py3 < -10.0 then(
      If(score > high_score then(high_score == score. ). ).
      game_state == "menu".
      ui_canvas_set_active(main_canvas, 1).
      ui_canvas_set_active(game_canvas, 0).
      ui_set_text(menu_title, "Game Over!").
      ui_set_text(menu_subtitle, "Score: " + int_to_str(score) + "  Best: " + int_to_str(high_score)).
    ). ).
    ui_set_text(hud_coins, "Coins: " + int_to_str(list_len(coins))).
  ). ).
  If(game_state = "3d" then(
    physics2d_update(dt3).
    px3a == transform_get_x(player).
    py3a == transform_get_y(player).
    h3a == input_get_axis("horizontal").
    If(h3a != 0 then(transform_translate(player, h3a * 4.0 * dt3, 0.0). ). ).
    If(input_get_key_down("space") then(
      vel3a == entity_get(player, "velocity").
      vel3a == list_set(vel3a, 1, 6.0).
      entity_set(player, "velocity", list_get(vel3a, 0), list_get(vel3a, 1), list_get(vel3a, 2)).
    ). ).
    cam2d_set_pos(px3a, py3a).
    ui_set_text(hud_mode, "MODE: 3D SANDBOX (WIP)").
  ). ).
  If(input_get_key_down("escape") then(
    If(game_state != "menu" then(
      If(score > high_score then(high_score == score. ). ).
      game_state == "menu".
      ui_canvas_set_active(main_canvas, 1).
      ui_canvas_set_active(game_canvas, 0).
      ui_set_text(menu_title, "UCLang Engine").
      ui_set_text(menu_subtitle, "Unity-inspired 2D + 3D").
    ). ).
  ). ).
).

def scene_draw(.
  Draw3D.begin().
  Draw3D.mesh("floor", "default", 0.0, -3.0, 0.0, 0.0, 0.0, 0.0, 16.0, 1.0, 16.0).
  px3b == transform_get_x(player).
  py3b == transform_get_y(player).
  Draw3D.mesh("cube", "default", px3b / 10.0, py3b / 10.0 - 0.5, 0.0, 0.0, 0.0, 0.0, 0.4, 0.4, 0.4).
  ci3b == 0.
  While(ci3b < list_len(coins) then(
    cid3b == list_get(coins, ci3b).
    If(entity_exists(cid3b) then(
      cx3b == transform_get_x(cid3b) / 10.0.
      cy3b == transform_get_y(cid3b) / 10.0.
      Draw3D.mesh("cube", "default", cx3b, cy3b - 0.3, 0.0, 0.0, time_elapsed() * 90.0, 0.0, 0.3, 0.3, 0.3).
    ). ).
    ci3b == ci3b + 1.
  ). ).
  ui_draw_all().
  debug_draw_all().
).

scene_register("engine_demo", "scene_init", "scene_update", "scene_draw").
scene_go("engine_demo").
print("O: scene init done").

GameLoop(
  update(.
    scene_update().
  ).
  draw(.
    scene_draw().
  ).
).
