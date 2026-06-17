Window.open("2D Test", 800, 600).
x == 200.
y == 300.
vx == 200.
vy == 150.
r == 30.
GameLoop(
  update(
    dt == time.delta().
    x == x + vx * dt.
    y == y + vy * dt.
    If(x + r >= 800 or x - r <= 0 then(vx == -vx.).).
    If(y + r >= 600 or y - r <= 0 then(vy == -vy.).).
  ).
  draw(
    Draw.clear(30, 30, 40).
    Draw.circle(x, y, r, "#ff4488").
    Draw.text("FPS: " + int_to_str(time.fps()), 10, 10, "#ffffff", 24).
    Draw.text("Pos: " + int_to_str(x) + ", " + int_to_str(y), 10, 36, "#aaaaaa", 18).
    Draw.present().
  ).
).
