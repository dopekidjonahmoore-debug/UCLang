run_file("stdlib/engine.uc").
Window.open_3d("test", 1024, 768).
main_canvas == ui_canvas_create().
game_canvas == ui_canvas_create().
print("canvases created").
ui_text(main_canvas, 320, 100, "UCLang Engine", 48, "#ffdd44").
print("ui_text ok").
ui_button(main_canvas, 312, 260, 200, 50, "Play", "on_play").
print("ui_button ok").
ui_canvas_set_active(game_canvas, 0).
print("ui_canvas_set_active ok").
hud_score == ui_text(game_canvas, 20, 20, "Score: 0", 24, "#ffffff").
print("hud_score ok").
Window.close().
