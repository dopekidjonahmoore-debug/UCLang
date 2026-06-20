run_file("stdlib/engine.uc").
print("stage 1: window open").
Window.open_3d("UCLang Engine", 1024, 768).
Camera3D.set_pos(0.0, 5.0, 8.0).
Camera3D.set_target(0.0, 0.0, 0.0).
light.directional(0.5, -1.0, 0.3, 1.0, 1.0, 1.0, 1.0).
light.ambient(0.3, 0.3, 0.4, 0.5).
physics2d_set_gravity(-12.0).
time_set_scale(1.0).
cam2d_set_zoom(1.0).
cam2d_set_pos(0.0, 0.0).
print("stage 2: setup").
main_canvas == ui_canvas_create().
game_canvas == ui_canvas_create().
ui_canvas_set_active(game_canvas, 0).
game_state == "menu".
score == 0.
high_score == 0.
Mesh.load_primitive("floor", "plane").
Mesh.load_primitive("cube", "cube").
player == gameobject_create("Player", 0.0, 3.0).
entity_set(player, "velocity", 0.0, 0.0, 0.0).
entity_set(player, "gravity", -12.0).
entity_set(player, "ground_y", -3.0).
entity_set(player, "sprite", 32.0, 32.0, "#44ddff").
jump_count == 0.
max_jumps == 2.
coins == [].
coin_timer == 0.0.
print("stage 3: defs").
def spawn_coin(.
  cx == Math.random() * 10.0 - 5.0.
  cy == Math.random() * 4.0 + 1.0.
  c == gameobject_create("Coin", cx, cy).
  entity_set(c, "sprite", 20.0, 20.0, "#ffdd44").
  coins == list_push(coins, c).
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
print("stage 4: menu").
menu_title == ui_text(main_canvas, 320, 100, "UCLang Engine", 48, "#ffdd44").
menu_subtitle == ui_text(main_canvas, 340, 160, "Unity-inspired 2D + 3D", 20, "#aaaacc").
btn_play == ui_button(main_canvas, 312, 260, 200, 50, "Play", "on_play").
btn_3d == ui_button(main_canvas, 312, 330, 200, 50, "3D Sandbox (WIP)", "on_3d").
btn_quit == ui_button(main_canvas, 312, 400, 200, 50, "Quit", "on_quit").
print("stage 5: on_play").
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
print("stage 6: hud").
hud_score == ui_text(game_canvas, 20, 20, "Score: 0", 24, "#ffffff").
hud_coins == ui_text(game_canvas, 20, 50, "Coins: 0", 18, "#ffdd44").
hud_mode == ui_text(game_canvas, 20, 720, "MODE: 2D", 14, "#44ff66").
hud_info == ui_text(game_canvas, 350, 720, "WASD Move | SPACE Jump x2 | ESC Menu", 14, "#888888").
print("stage 7: scene").
def scene_init(.
  Overlay.font("assets/font.ttf", 24).
  reset_game().
).
def scene_update(.
  dt == time_delta().
  If(game_state = "game" then(
    physics2d_update(dt).
    px == transform_get_x(player).
    py == transform_get_y(player).
    h == input_get_axis("horizontal").
    If(h != 0 then(transform_translate(player, h * 5.0 * dt, 0.0). ). ).
    grounded == physics2d_is_grounded(player).
    If(grounded then(jump_count == 0. ). ).
    If(input_get_key_down("space") and jump_count < max_jumps then(
      vel == entity_get(player, "velocity").
      vel == list_set(vel, 1, 7.0).
      entity_set(player, "velocity", list_get(vel, 0), list_get(vel, 1), list_get(vel, 2)).
      jump_count == jump_count + 1.
    ). ).
    If(px < -10.0 then(transform_set_pos(player, -10.0, py). ). ).
    If(px > 10.0 then(transform_set_pos(player, 10.0, py). ). ).
    cam2d_set_pos(px, py).
    ci == 0.
    While(ci < list_len(coins) then(
      cid == list_get(coins, ci).
      If(entity_exists(cid) then(
        cx2 == transform_get_x(cid).
        cy2 == transform_get_y(cid).
        dist == mathf_distance(px, py, cx2, cy2).
        If(dist < 24.0 then(
          entity_destroy(cid).
          coins == list_remove(coins, ci).
          score == score + 1.
          ui_set_text(hud_score, "Score: " + int_to_str(score)).
        ). else(ci == ci + 1. ). ).
      ). else(coins == list_remove(coins, ci). ). ).
    ). ).
    coin_timer == coin_timer + dt.
    If(coin_timer > 2.0 and list_len(coins) < 8 then(
      spawn_coin().
      coin_timer == 0.0.
    ). ).
    If(py < -10.0 then(
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
    physics2d_update(dt).
    px2 == transform_get_x(player).
    py2 == transform_get_y(player).
    h2 == input_get_axis("horizontal").
    If(h2 != 0 then(transform_translate(player, h2 * 4.0 * dt, 0.0). ). ).
    If(input_get_key_down("space") then(
      vel2 == entity_get(player, "velocity").
      vel2 == list_set(vel2, 1, 6.0).
      entity_set(player, "velocity", list_get(vel2, 0), list_get(vel2, 1), list_get(vel2, 2)).
    ). ).
    cam2d_set_pos(px2, py2).
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
  px3 == transform_get_x(player).
  py3 == transform_get_y(player).
  Draw3D.mesh("cube", "default", px3 / 10.0, py3 / 10.0 - 0.5, 0.0, 0.0, 0.0, 0.0, 0.4, 0.4, 0.4).
  ci2 == 0.
  While(ci2 < list_len(coins) then(
    cid2 == list_get(coins, ci2).
    If(entity_exists(cid2) then(
      cx3 == transform_get_x(cid2) / 10.0.
      cy3 == transform_get_y(cid2) / 10.0.
      Draw3D.mesh("cube", "default", cx3, cy3 - 0.3, 0.0, 0.0, time_elapsed() * 90.0, 0.0, 0.3, 0.3, 0.3).
    ). ).
    ci2 == ci2 + 1.
  ). ).
  ui_draw_all().
  debug_draw_all().
).
print("stage 8: register").
scene_register("engine_demo", "scene_init", "scene_update", "scene_draw").
print("stage 9: scene_go").
scene_go("engine_demo").
print("stage 10: done").
Window.close().
