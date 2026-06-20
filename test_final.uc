run_file("stdlib/engine.uc").
Window.open_3d("test", 1024, 768).
Camera3D.set_pos(0.0, 5.0, 8.0).
Camera3D.set_target(0.0, 0.0, 0.0).
light.directional(0.5, -1.0, 0.3, 1.0, 1.0, 1.0, 1.0).
light.ambient(0.3, 0.3, 0.4, 0.5).
physics2d_set_gravity(-12.0).
time_set_scale(1.0).
cam2d_set_zoom(1.0).
cam2d_set_pos(0.0, 0.0).

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

menu_title == ui_text(main_canvas, 320, 100, "UCLang Engine", 48, "#ffdd44").
menu_subtitle == ui_text(main_canvas, 340, 160, "test", 20, "#aaaacc").
btn_play == ui_button(main_canvas, 312, 260, 200, 50, "Play", "on_play").

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

hud_score == ui_text(game_canvas, 20, 20, "Score: 0", 24, "#ffffff").
hud_coins == ui_text(game_canvas, 20, 50, "Coins: 0", 18, "#ffdd44").
hud_mode == ui_text(game_canvas, 20, 720, "MODE: 2D", 14, "#44ff66").
hud_info == ui_text(game_canvas, 350, 720, "test", 14, "#888888").

def scene_init(.
  Overlay.font("assets/font.ttf", 24).
  reset_game().
).
def scene_update(.
).
def scene_draw(.
  Draw3D.begin().
  ui_draw_all().
  debug_draw_all().
).

scene_register("engine_demo", "scene_init", "scene_update", "scene_draw").
print("about to scene_go").
scene_go("engine_demo").
print("after scene_go").
Window.close().
