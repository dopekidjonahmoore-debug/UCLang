run.run_file("stdlib/animation.uc").
run.run_file("stdlib/particles.uc").

Window.open("UCLang Engine Demo", 1024, 768).

px == 400. py == 300.
ps == run.particle_system_new(500).
t == tween_create(0, 100, 2, "quad_out").
trailX == []. trailY == [].

GameLoop(
  update(
    dt == time.delta().
    speed == 300 * dt.
    dx == 0. dy == 0.

    If(input.key("a") or input.key("left")  then(dx == -speed.). ).
    If(input.key("d") or input.key("right") then(dx == speed.). ).
    If(input.key("w") or input.key("up")    then(dy == -speed.). ).
    If(input.key("s") or input.key("down")  then(dy == speed.). ).

    If(dx != 0 and dy != 0 then(
      dx == dx * 0.7071. dy == dy * 0.7071.
    ). ).

    px == px + dx.
    py == py + dy.
    If(px < 0   then(px == 1024.). ).
    If(px > 1024 then(px == 0.). ).
    If(py < 0   then(py == 768.). ).
    If(py > 768 then(py == 0.). ).

    trailX == list_push(trailX, px).
    trailY == list_push(trailY, py).
    If(list_len(trailX) > 30 then(
      trailX == list_remove(trailX, 0).
      trailY == list_remove(trailY, 0).
    ). ).

    t == tween_update(t, dt).
    If(tween_done(t) then(
      t == tween_create(0, 100, 2, "quad_out").
    ). ).

    mx == input.mouse_x(). my == input.mouse_y().
    If(input.mouse_down("left") then(
      ps == run.particle_system_emit(ps, 2, mx, my, 80, 1.5, "#ffaa44").
    ). ).
    If(input.key_just("space") then(
      ps == run.particle_system_emit(ps, 25, px, py, 120, 2.0, "#44ddff").
    ). ).

    ps == run.particle_system_update(ps, dt).
  ).

  draw(
    Draw.clear("#0f0f23").

    i == 0.
    While(i < 1024 then(
      Draw.line(i, 0, i, 768, "#1a1a3e").
      i == i + 48.
    ). ).
    j == 0.
    While(j < 768 then(
      Draw.line(0, j, 1024, j, "#1a1a3e").
      j == j + 48.
    ). ).

    i == 0.
    While(i < list_len(trailX) then(
      tx == list_get(trailX, i). ty == list_get(trailY, i).
      Draw.rect(tx - 4, ty - 4, 8, 8, "#e94560").
      i == i + 1.
    ). ).

    run.particle_system_draw(ps).

    r == tween_value(t) / 5 + 15.
    Draw.circle(150, 100, r, "#0f6490").

    Draw.rect(px - 15, py - 15, 30, 30, "#e94560").

    mx == input.mouse_x(). my == input.mouse_y().
    Draw.circle(mx, my, 4, "#ffffff").

    Draw.text("UCLang Engine Demo", 20, 20, "#ffffff", 28).
    Draw.text("FPS: " + int_to_str(time.fps()), 20, 52, "#88ff88", 18).
    Draw.text("Pos: " + int_to_str(px) + ", " + int_to_str(py), 20, 74, "#aaaaaa", 16).
    Draw.text("Particles: " + int_to_str(list_len(list_get(ps, 1))), 20, 94, "#ffaa44", 16).
    Draw.text("WASD/Arrows: move | Space: burst | Click: paint", 20, 734, "#666688", 14).
  ).
).
