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
print("a: player created").
entity_set(player, "velocity", 0.0, 0.0, 0.0).
print("b: vel set").
entity_set(player, "gravity", -12.0).
print("c: grav set").
entity_set(player, "ground_y", -3.0).
print("d: ground_y set").
entity_set(player, "sprite", 32.0, 32.0, "#44ddff").
print("e: sprite set").
jump_count == 0.
max_jumps == 2.
coins == [].
coin_timer == 0.0.
print("f: vars init").
Window.close().
print("g: done").
