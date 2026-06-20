run_file("stdlib/engine.uc").
print("step 1: engine loaded").
Window.open_3d("test", 1024, 768).
print("step 2: window open").
Camera3D.set_pos(0.0, 5.0, 8.0).
Camera3D.set_target(0.0, 0.0, 0.0).
light.directional(0.5, -1.0, 0.3, 1.0, 1.0, 1.0, 1.0).
light.ambient(0.3, 0.3, 0.4, 0.5).
print("step 3: 3d setup").
physics2d_set_gravity(-12.0).
time_set_scale(1.0).
cam2d_set_zoom(1.0).
cam2d_set_pos(0.0, 0.0).
print("step 4: physics/cam setup").
main_canvas == ui_canvas_create().
print("step 5: main canvas").
game_canvas == ui_canvas_create().
print("step 6: game canvas").
ui_canvas_set_active(game_canvas, 0).
print("step 7").
game_state == "menu".
score == 0.
high_score == 0.
Mesh.load_primitive("floor", "plane").
Mesh.load_primitive("cube", "cube").
print("step 8: meshes").
player == gameobject_create("Player", 0.0, 3.0).
print("step 9: player").
Window.close().
print("step 10: done").
