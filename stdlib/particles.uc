particle_create(x, y, vx, vy, lifetime, color.
  response([x, y, vx, vy, lifetime, lifetime, color]).
).

particle_update(p, dt.
  px == list_get(p, 0) + list_get(p, 2) * dt.
  py == list_get(p, 1) + list_get(p, 3) * dt.
  p == list_set(p, 0, px).
  p == list_set(p, 1, py).
  remaining == list_get(p, 4) - dt.
  p == list_set(p, 4, remaining).
  response(p).
).

particle_draw(p.
  alpha == Math.clamp(list_get(p, 4) / list_get(p, 5), 0, 1).
  color == list_get(p, 6).
  draw.circle(list_get(p, 0), list_get(p, 1), 3, color).
).

particle_alive(p.
  response(list_get(p, 4) > 0).
).

particle_burst(x, y, count, speed, lifetime, color.
  result == [].
  i == 0.
  While(i < count then(
    angle == Math.random(0, 6.28318).
    vx == Math.cos(angle) * Math.random(speed * 0.5, speed).
    vy == Math.sin(angle) * Math.random(speed * 0.5, speed).
    p == run.particle_create(x, y, vx, vy, lifetime, color).
    result == list_push(result, p).
    i == i + 1.
  ). ).
  response(result).
).

particle_system_new(maxParticles.
  response([maxParticles, []]).
).

particle_system_update(ps, dt.
  particles == list_get(ps, 1).
  i == 0.
  count == list_len(particles).
  While(i < count then(
    p == list_get(particles, i).
    p == run.particle_update(p, dt).
    If(run.particle_alive(p) then(
      particles == list_set(particles, i, p).
      i == i + 1.
    ). else(
      particles == list_remove(particles, i).
      count == count - 1.
    ). ).
  ). ).
  ps == list_set(ps, 1, particles).
  response(ps).
).

particle_system_draw(ps.
  particles == list_get(ps, 1).
  i == 0.
  While(i < list_len(particles) then(
    run.particle_draw(list_get(particles, i)).
    i == i + 1.
  ). ).
).

particle_system_emit(ps, count, x, y, speed, lifetime, color.
  particles == list_get(ps, 1).
  maxP == list_get(ps, 0).
  burst == run.particle_burst(x, y, count, speed, lifetime, color).
  i == 0.
  While(i < list_len(burst) and list_len(particles) < maxP then(
    particles == list_push(particles, list_get(burst, i)).
    i == i + 1.
  ). ).
  ps == list_set(ps, 1, particles).
  response(ps).
).
