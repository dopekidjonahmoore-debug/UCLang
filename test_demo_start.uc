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
