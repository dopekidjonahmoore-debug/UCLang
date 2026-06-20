run_file("stdlib/engine.uc").
Window.open_3d("test", 1024, 768).
def my_init(. print("init!"). ).
def my_update(. print("update!"). ).
def my_draw(. print("draw!"). ).
scene_register("test", "my_init", "my_update", "my_draw").
print("registered").
scene_go("test").
print("scene active").
Window.close().
