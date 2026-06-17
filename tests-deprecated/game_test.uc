Window.open("UCLang Game", 800, 600).

x == 200.
y == 200.
dx == 3.
dy == 2.
boxSize == 50.

GameLoop(
    update(
        x == x + dx.
        y == y + dy.
        If(x => 750 or x <= 0 then(
            dx == dx * -1.
        ).).
        If(y => 550 or y <= 0 then(
            dy == dy * -1.
        ).).
    ).
    draw(
        Draw.clear("#1e1e1e").
        Draw.rect(x, y, boxSize, boxSize, "#007acc").
        Draw.text("UCLang!", 10, 10, "#ffffff", 24).
    ).
).

Window.close().
